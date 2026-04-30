#include "Common.h"
#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstring>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 5)
    {
        if (rank == 0)
            std::cerr << "Use: mpirun -np <P> ./cannon <M> <FileA> <FileB> <FileC> [noThreads] [parallelRead]\n";
        MPI_Finalize();
        return 1;
    }

    size_t M = std::stoul(argv[1]);
    std::string fileA = argv[2];
    std::string fileB = argv[3];
    std::string fileC = argv[4];
    size_t numThreads = (argc > 5) ? std::stoul(argv[5]) : 10;
    bool parallelRead = (argc > 6 && std::strcmp(argv[6], "1") == 0);

    int gridSize = static_cast<int>(std::sqrt(size));

    // create 2D grid
    int dims[2] = {gridSize, gridSize};
    int periods[2] = {1, 1};
    MPI_Comm gridComm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &gridComm);

    int coords[2];
    MPI_Cart_coords(gridComm, rank, 2, coords);
    int myRow = coords[0];
    int myCol = coords[1];

    MPI_Comm rowComm, colComm;
    int remainRow[2] = {0, 1};
    int remainCol[2] = {1, 0};
    MPI_Cart_sub(gridComm, remainRow, &rowComm);
    MPI_Cart_sub(gridComm, remainCol, &colComm);

    size_t blockSize = M / gridSize;

    std::vector<double> localA(blockSize * blockSize);
    std::vector<double> localB(blockSize * blockSize);
    std::vector<double> localC(blockSize * blockSize, 0.0);

    Timer timer;
    Timer totalTimer;

    MPI_Barrier(gridComm);
    totalTimer.start();

    // reading
    timer.start();

    if (parallelRead)
    {
        // each process reads its own block directly from file
        std::ifstream infA(fileA, std::ios::binary);
        std::ifstream infB(fileB, std::ios::binary);

        for (size_t i = 0; i < blockSize; ++i)
        {
            size_t globalRow = myRow * blockSize + i;
            size_t globalCol = myCol * blockSize;
            size_t offset = (globalRow * M + globalCol) * sizeof(double);

            infA.seekg(offset);
            infA.read(reinterpret_cast<char*>(localA.data() + i * blockSize), blockSize * sizeof(double));

            infB.seekg(offset);
            infB.read(reinterpret_cast<char*>(localB.data() + i * blockSize), blockSize * sizeof(double));
        }
    }
    else
    {
        // process 0 reads and distributes
        if (rank == 0)
        {
            Matrix A = ReadMatrixBinary(fileA, M);
            Matrix B = ReadMatrixBinary(fileB, M);

            for (int p = 0; p < size; ++p)
            {
                int pCoords[2];
                MPI_Cart_coords(gridComm, p, 2, pCoords);
                int pi = pCoords[0];
                int pj = pCoords[1];

                std::vector<double> blockA(blockSize * blockSize);
                std::vector<double> blockB(blockSize * blockSize);

                for (size_t i = 0; i < blockSize; ++i)
                    for (size_t j = 0; j < blockSize; ++j)
                    {
                        blockA[i * blockSize + j] = A[(pi * blockSize + i) * M + (pj * blockSize + j)];
                        blockB[i * blockSize + j] = B[(pi * blockSize + i) * M + (pj * blockSize + j)];
                    }

                if (p == 0)
                {
                    localA = blockA;
                    localB = blockB;
                }
                else
                {
                    MPI_Send(blockA.data(), blockSize * blockSize, MPI_DOUBLE, p, 0, gridComm);
                    MPI_Send(blockB.data(), blockSize * blockSize, MPI_DOUBLE, p, 1, gridComm);
                }
            }
        }
        else
        {
            MPI_Recv(localA.data(), blockSize * blockSize, MPI_DOUBLE, 0, 0, gridComm, MPI_STATUS_IGNORE);
            MPI_Recv(localB.data(), blockSize * blockSize, MPI_DOUBLE, 0, 1, gridComm, MPI_STATUS_IGNORE);
        }
    }

    double t_reading = timer.stop();

    // initial skew
    timer.start();

    int srcA, dstA, srcB, dstB;

    // shift A left by myRow
    MPI_Cart_shift(rowComm, 0, -myRow, &srcA, &dstA);
    MPI_Sendrecv_replace(localA.data(), blockSize * blockSize, MPI_DOUBLE, dstA, 0, srcA, 0, rowComm, MPI_STATUS_IGNORE);

    // shift B up by myCol
    MPI_Cart_shift(colComm, 0, -myCol, &srcB, &dstB);
    MPI_Sendrecv_replace(localB.data(), blockSize * blockSize, MPI_DOUBLE, dstB, 0, srcB, 0, colComm, MPI_STATUS_IGNORE);

    // main loop
    for (int step = 0; step < gridSize; ++step)
    {
        // multiply local blocks
        #pragma omp parallel for num_threads(numThreads) schedule(static)
        for (size_t i = 0; i < blockSize; ++i)
            for (size_t k = 0; k < blockSize; ++k)
            {
                double a_ik = localA[i * blockSize + k];
                for (size_t j = 0; j < blockSize; ++j)
                    localC[i * blockSize + j] += a_ik * localB[k * blockSize + j];
            }

        // shift A left by 1
        MPI_Cart_shift(rowComm, 0, -1, &srcA, &dstA);
        MPI_Sendrecv_replace(localA.data(), blockSize * blockSize, MPI_DOUBLE, dstA, 0, srcA, 0, rowComm, MPI_STATUS_IGNORE);

        // shift B up by 1
        MPI_Cart_shift(colComm, 0, -1, &srcB, &dstB);
        MPI_Sendrecv_replace(localB.data(), blockSize * blockSize, MPI_DOUBLE, dstB, 0, srcB, 0, colComm, MPI_STATUS_IGNORE);
    }

    double t_multiply = timer.stop();

    // gather and write
    timer.start();

    if (rank == 0)
    {
        Matrix C(M * M, 0.0);

        for (size_t i = 0; i < blockSize; ++i)
            for (size_t j = 0; j < blockSize; ++j)
                C[i * M + j] = localC[i * blockSize + j];

        for (int p = 1; p < size; ++p)
        {
            int pCoords[2];
            MPI_Cart_coords(gridComm, p, 2, pCoords);
            int pi = pCoords[0];
            int pj = pCoords[1];

            std::vector<double> block(blockSize * blockSize);
            MPI_Recv(block.data(), blockSize * blockSize, MPI_DOUBLE, p, 2, gridComm, MPI_STATUS_IGNORE);

            for (size_t i = 0; i < blockSize; ++i)
                for (size_t j = 0; j < blockSize; ++j)
                    C[(pi * blockSize + i) * M + (pj * blockSize + j)] = block[i * blockSize + j];
        }

        WriteMatrixBinary(fileC, C, M);
    }
    else
    {
        MPI_Send(localC.data(), blockSize * blockSize, MPI_DOUBLE, 0, 2, gridComm);
    }

    double t_writing = timer.stop();
    double t_total = totalTimer.stop();

    if (rank == 0)
    {
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "t_reading: " << t_reading * 1000 << " ms\n"
            << "t_multiply: " << t_multiply * 1000 << " ms\n"
            << "t_writing: " << t_writing * 1000 << " ms\n"
            << "t_total: " << t_total * 1000 << " ms\n";
    }

    MPI_Comm_free(&rowComm);
    MPI_Comm_free(&colComm);
    MPI_Comm_free(&gridComm);
    MPI_Finalize();
    return 0;
}
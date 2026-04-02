#include "Common.h"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <omp.h>

void MultiplyParallelOMP(Matrix const& A, Matrix const& B, Matrix& C, size_t M, size_t numThreads, size_t block)
{
    double const* __restrict__ a = A.data();
    double const* __restrict__ b = B.data();
    double* __restrict__ c = C.data();

    #pragma omp parallel for num_threads(numThreads) schedule(static)
    for (size_t i0 = 0; i0 < M; i0 += block)
        for (size_t k0 = 0; k0 < M; k0 += block)
            for (size_t j0 = 0; j0 < M; j0 += block)
            {
                size_t iMax = std::min(i0 + block, M);
                size_t kMax = std::min(k0 + block, M);
                size_t jMax = std::min(j0 + block, M);

                for (size_t i = i0; i < iMax; ++i)
                    for (size_t k = k0; k < kMax; ++k)
                    {
                        double a_ik = a[i * M + k];
                        for (size_t j = j0; j < jMax; ++j)
                            c[i * M + j] += a_ik * b[k * M + j];
                    }
            }
}

void ReadMatrixBinaryParallelOMP(std::string const& filename, Matrix& matrix, size_t M, size_t numThreads)
{
    #pragma omp parallel num_threads(numThreads)
    {
        int tid = omp_get_thread_num();
        int total = omp_get_num_threads();

        size_t rowsPerThread = M / total;
        size_t remainder = M % total;
        size_t startRow = tid * rowsPerThread + std::min(static_cast<size_t>(tid), remainder);
        size_t endRow = startRow + rowsPerThread + (static_cast<size_t>(tid) < remainder ? 1 : 0);

        std::ifstream inf(filename, std::ios::binary);
        size_t offset = startRow * M * sizeof(double);
        inf.seekg(offset);

        size_t count = (endRow - startRow) * M;
        inf.read(reinterpret_cast<char*>(matrix.data() + startRow * M), count * sizeof(double));
    }
}

int main(int argc, char** argv)
{
    if (argc < 7)
    {
        std::cerr << "Use: " << argv[0] << " <M> <FileA> <FileB> <FileC> <numThreads> <parallelRead> [block]\n";
        return 1;
    }

    size_t M = std::stoul(argv[1]);
    std::string fileA = argv[2];
    std::string fileB = argv[3];
    std::string fileC = argv[4];
    size_t numThreads = std::stoul(argv[5]);
    bool parallelRead = (std::strcmp(argv[6], "1") == 0);
    size_t block = (argc > 7) ? std::stoul(argv[7]) : 48;

    Timer timer;
    Timer totalTimer;
    totalTimer.start();

    timer.start();
    Matrix A(M * M);
    Matrix B(M * M);
    if (parallelRead)
    {
        ReadMatrixBinaryParallelOMP(fileA, A, M, numThreads);
        ReadMatrixBinaryParallelOMP(fileB, B, M, numThreads);
    }
    else
    {
        A = ReadMatrixBinary(fileA, M);
        B = ReadMatrixBinary(fileB, M);
    }
    double t_reading = timer.stop();

    timer.start();
    Matrix C(M * M, 0.0);
    MultiplyParallelOMP(A, B, C, M, numThreads, block);
    double t_multiply = timer.stop();

    timer.start();
    WriteMatrixBinary(fileC, C, M);
    double t_writing = timer.stop();

    double t_total = totalTimer.stop();

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "t_reading: " << t_reading * 1000 << " ms\n"
        << "t_multiply: " << t_multiply * 1000 << " ms\n"
        << "t_writing: " << t_writing * 1000 << " ms\n"
        << "t_total: " << t_total * 1000 << " ms\n";
}
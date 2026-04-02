#include "Common.h"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <pthread.h>

struct ThreadData
{
    double const* a;
    double const* b;
    double* c;
    size_t M;
    size_t block;
    size_t startRow;
    size_t endRow;
};

struct ReadThreadData
{
    std::string filename;
    double* matrix;
    size_t M;
    size_t startRow;
    size_t endRow;
};

void* ReadThread(void* arg)
{
    ReadThreadData* d = static_cast<ReadThreadData*>(arg);
    std::ifstream inf(d->filename, std::ios::binary);

    size_t offset = d->startRow * d->M * sizeof(double);
    inf.seekg(offset);

    size_t count = (d->endRow - d->startRow) * d->M;
    inf.read(reinterpret_cast<char*>(d->matrix + d->startRow * d->M), count * sizeof(double));

    return nullptr;
}

void* MultiplyThread(void* arg)
{
    ThreadData* d = static_cast<ThreadData*>(arg);

    for (size_t i0 = d->startRow; i0 < d->endRow; i0 += d->block)
        for (size_t k0 = 0; k0 < d->M; k0 += d->block)
            for (size_t j0 = 0; j0 < d->M; j0 += d->block)
            {
                size_t iMax = std::min(i0 + d->block, d->endRow);
                size_t kMax = std::min(k0 + d->block, d->M);
                size_t jMax = std::min(j0 + d->block, d->M);

                for (size_t i = i0; i < iMax; ++i)
                    for (size_t k = k0; k < kMax; ++k)
                    {
                        double a_ik = d->a[i * d->M + k];
                        for (size_t j = j0; j < jMax; ++j)
                            d->c[i * d->M + j] += a_ik * d->b[k * d->M + j];
                    }
            }
    return nullptr;
}

void ReadMatrixBinaryParallel(std::string const& filename, Matrix& matrix, size_t M, size_t numThreads)
{
    std::vector<pthread_t> threads(numThreads);
    std::vector<ReadThreadData> data(numThreads);

    size_t rowsPerThread = M / numThreads;
    size_t remainder = M % numThreads;

    size_t currentRow = 0;
    for (size_t t = 0; t < numThreads; ++t)
    {
        data[t].filename = filename;
        data[t].matrix = matrix.data();
        data[t].M = M;
        data[t].startRow = currentRow;
        data[t].endRow = currentRow + rowsPerThread + (t < remainder ? 1 : 0);
        currentRow = data[t].endRow;

        pthread_create(&threads[t], nullptr, ReadThread, &data[t]);
    }

    for (size_t t = 0; t < numThreads; ++t)
        pthread_join(threads[t], nullptr);
}

void MultiplyParallel(Matrix const& A, Matrix const& B, Matrix& C, size_t M, size_t numThreads, size_t block)
{
    std::vector<pthread_t> threads(numThreads);
    std::vector<ThreadData> data(numThreads);

    size_t rowsPerThread = M / numThreads;
    size_t remainder = M % numThreads;

    size_t currentRow = 0;
    for (size_t t = 0; t < numThreads; ++t)
    {
        data[t].a = A.data();
        data[t].b = B.data();
        data[t].c = C.data();
        data[t].M = M;
        data[t].block = block;
        data[t].startRow = currentRow;
        data[t].endRow = currentRow + rowsPerThread + (t < remainder ? 1 : 0);
        currentRow = data[t].endRow;

        pthread_create(&threads[t], nullptr, MultiplyThread, &data[t]);
    }

    for (size_t t = 0; t < numThreads; ++t)
        pthread_join(threads[t], nullptr);
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
    if (parallelRead) {
        ReadMatrixBinaryParallel(fileA, A, M, numThreads);
        ReadMatrixBinaryParallel(fileB, B, M, numThreads);
    } else {
        A = ReadMatrixBinary(fileA, M);
        B = ReadMatrixBinary(fileB, M);
    }
    double t_reading = timer.stop();

    timer.start();
    Matrix C(M * M, 0.0);
    MultiplyParallel(A, B, C, M, numThreads, block);
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
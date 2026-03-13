#ifndef COMMON_H
#define COMMON_H

#include "Define.h"

#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <stdexcept>

using Matrix = std::vector<double>;

Matrix ReadMatrixText(std::string const& filename, size_t M)
{
    Matrix matrix(M * M);
    std::ifstream inf(filename);
    if (!inf.is_open())
        throw std::runtime_error("Cant open file: " + filename);

    for (size_t i = 0; i < M * M; ++i)
        inf >> matrix[i];

    return matrix;
}

Matrix ReadMatrixBinary(std::string const& filename, size_t M)
{
    Matrix matrix(M * M);
    std::ifstream inf(filename, std::ios::binary);
    if (!inf.is_open())
        throw std::runtime_error("Cant open file: " + filename);

    inf.read(reinterpret_cast<char*>(matrix.data()), M * M * sizeof(double));

    return matrix;
}

void WriteMatrixText(std::string const& filename, Matrix const& matrix, size_t M)
{
    std::ofstream outf(filename);
    if (!outf.is_open())
        throw std::runtime_error("Cant open file: " + filename);

    for (size_t i = 0; i < M * M; ++i)
        outf << matrix[i] << "\n";
}

void WriteMatrixBinary(std::string const& filename, Matrix const& matrix, size_t M)
{
    std::ofstream outf(filename, std::ios::binary);
    if (!outf.is_open())
        throw std::runtime_error("Cant open file: " + filename);

    outf.write(reinterpret_cast<char const*>(matrix.data()), M * M * sizeof(double));
}

Matrix Multiply(Matrix const& A, Matrix const& B, size_t M)
{
    Matrix C(M * M, 0.0);
    for (size_t i = 0; i < M; ++i)
        for (size_t k = 0; k < M; ++k)
        {
            double A_ik = A[i * M + k];
            for (size_t j = 0; j < M; ++j)
                C[i * M + j] += A_ik * B[k * M + j];
        }
    
    return C;
}

class Timer
{
private:
    std::chrono::high_resolution_clock::time_point _start;

public:
    void start()
    {
        _start = std::chrono::high_resolution_clock::now();
    }

    double stop()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - _start;
        return elapsed.count();
    }
};

#endif
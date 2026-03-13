#include "Common.h"

#include <iostream>
#include <cstring>
#include <random>

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::cerr << "Use: " << argv[0] << " <M> <outputFile> <text | binary> <seed>\n";
        return 1;
    }

    size_t M = std::stoul(argv[1]);
    std::string filename = argv[2];
    bool binary = (std::strcmp(argv[3], "binary") == 0);
    uint32 seed = static_cast<uint32>(std::stoul(argv[4]));

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-100.0, 100.0);

    Matrix matrix(M * M);
    for (size_t i = 0; i < matrix.size(); ++i)
        matrix[i] = dist(rng);

    if (binary)
        WriteMatrixBinary(filename, matrix, M);
    else
        WriteMatrixText(filename, matrix, M);

    std::cout << "Generated " << M << "x" << M << " matrix --> " << filename << "\n";
}
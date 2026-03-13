#include "Common.h"

#include <iostream>
#include <iomanip>
#include <cstring>

int main(int argc, char** argv)
{
    if (argc < 6)
    {
        std::cerr << "Use: " << argv[0] << " <M> <FileA> <FileB> <FileC> <text | binary> [block size]\n";
        return 1;
    }

    size_t M = std::stoul(argv[1]);
    std::string fileA = argv[2];
    std::string fileB = argv[3];
    std::string fileC = argv[4];
    bool binary = (std::strcmp(argv[5], "binary") == 0);
    size_t block = (argc > 6) ? std::stoul(argv[6]) : 48;

    Timer timer;
    Timer totalTimer;
    totalTimer.start();

    timer.start();
    Matrix A = binary ? ReadMatrixBinary(fileA, M) : ReadMatrixText(fileA, M);
    Matrix B = binary ? ReadMatrixBinary(fileB, M) : ReadMatrixText(fileB, M);
    double t_reading = timer.stop();

    timer.start();
    Matrix C = Multiply(A, B, M, block);
    double t_multiply = timer.stop();

    timer.start();
    if (binary)
        WriteMatrixBinary(fileC, C, M);
    else
        WriteMatrixText(fileC, C, M);
    double t_writing = timer.stop();

    double t_total = totalTimer.stop();
    
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "t_reading: " << t_reading * 1000 << " ms\n"
        << "t_multiply: " << t_multiply * 1000 << " ms\n"
        << "t_writing: " << t_writing * 1000 << " ms\n"
        << "t_total: " << t_total * 1000 << " ms\n";
}
#include "stockfish_divide.h"
#include <array>
#include <cstdio>
#include <iostream>
#include <sstream>

static std::string ExecAndCaptureOutput(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r"); 
    if (!pipe) {
        throw std::runtime_error("popen()/_popen() failed");
    }
    std::string output;
    constexpr int bufferSize = 4096;
    std::array<char, bufferSize> buffer;
    while (fgets(buffer.data(), bufferSize, pipe) != NULL) {
        output += buffer.data();
    }

    pclose(pipe);
    return output;
}

std::string ComputeStockfishPerftDivide(const std::string &fenString, int depth) {
    // Run stockfish with fen and depth
    // std::string command = "echo -e \"position fen " + fenString + R"(\ngo perft )" + std::to_string(depth) + 
    //                       R"(\nquit" | stockfish')"; // | grep '^[a-h][1-8][a-h][1-8]' | sort)";
    std::string command = "stockfish << 'EOF' | grep '^[a-h][1-8][a-h][1-8]' | sort\n"
                          "position fen " + fenString + "\n"
                          "go perft " + std::to_string(depth) + "\n"
                          "quit\n"
                          "EOF\n";
    return ExecAndCaptureOutput(command);
}

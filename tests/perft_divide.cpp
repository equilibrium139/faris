#include "perft_divide.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include "movegen.h"
#include <sstream>
#include <streambuf>

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

std::string ComputeFarisPerftDivide(const Board &board, int depth, Color colorToMove) {
    std::stringstream ss;
    std::streambuf* oldBuf = std::cout.rdbuf(ss.rdbuf());
    enablePerftDiagnostics = true;
    perftest(board, depth, colorToMove);
    enablePerftDiagnostics = false;
    std::cout.rdbuf(oldBuf);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    std::sort(lines.begin(), lines.end());
    std::string output;
    for (const std::string& l : lines) {
       output += l + "\n";
    }
    return output;
}

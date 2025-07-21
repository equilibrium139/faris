#include "perft_divide.h"
#include "perft.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include "board.h"
#include "fen.h"
#include "movegen.h"
#include "utilities.h"
#include <iterator>
#include <sstream>
#include <streambuf>
#include <string_view>

#ifdef _WIN32
#include <cstdio>
#define popen _popen
#define pclose _pclose
#endif

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

static std::vector<std::string> ToSortedLines(const std::string& str) {
    std::vector<std::string> lines;
    std::stringstream ss {str};
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    std::sort(lines.begin(), lines.end());
    return lines;
}

std::vector<std::string> ComputeStockfishPerftDivide(const std::string &fenString, int depth) {
    // Run stockfish with fen and depth
    // std::string command = "echo -e \"position fen " + fenString + R"(\ngo perft )" + std::to_string(depth) + 
    //                       R"(\nquit" | stockfish')"; // | grep '^[a-h][1-8][a-h][1-8]' | sort)";
    std::string command = "stockfish << 'EOF' | grep '^[a-h][1-8][a-h][1-8]' | sort\n"
                          "position fen " + fenString + "\n"
                          "go perft " + std::to_string(depth) + "\n"
                          "quit\n"
                          "EOF\n";
    std::string output = ExecAndCaptureOutput(command);
    return ToSortedLines(output);
}

std::vector<std::string> ComputeFarisPerftDivide(Board &board, int depth, Color colorToMove) {
    std::stringstream ss;
    std::streambuf* oldBuf = std::cout.rdbuf(ss.rdbuf());
    maxDepth = depth;
    perftest(board, depth, colorToMove, true);
    std::cout.rdbuf(oldBuf);
    return ToSortedLines(ss.str());
}

static void ApplyMove(Board& board, Square from, Square to) {
    Bitboard occupancy = board.Occupancy();
    if (occupancy & ToBitboard(to)) { // this is a capture
        Piece toPiece = PieceAt(to, board);
        RemovePiece(to, board, toPiece.color);
    }
    Piece fromPiece = PieceAt(from, board);
    board.Move(fromPiece.type, fromPiece.color, from, to);
}

static void ApplyMove(Board& board, std::string_view move) {
    Square f = (move[0] - 'a') + (move[1] - '1') * 8;
    Square t = (move[2] - 'a') + (move[3] - '1') * 8;
    ApplyMove(board, f, t);
}

void PrintFirstIncorrectMoveChain(const std::string &fenString, int depth, const Board& board, Color colorToMove) {
    const auto moveComparator = [](const std::string& m1, const std::string& m2) { return m1.substr(0, 4) < m2.substr(0, 4); };
    std::vector<std::string> moveChain;
    Board b = board;
    std::string fenStr = fenString;
    Color c = colorToMove;
    for (int d = depth; d > 0; d--) {
        // TODO: update fen and board 
        std::vector<std::string> stockfishPerftDivide = ComputeStockfishPerftDivide(fenStr, d); 
        std::vector<std::string> farisPerftDivide = ComputeFarisPerftDivide(b, d, c);
        std::vector<std::string> difference;

        std::set_symmetric_difference(stockfishPerftDivide.begin(), stockfishPerftDivide.end(), farisPerftDivide.begin(), farisPerftDivide.end(), 
                                      std::inserter(difference, difference.begin()), moveComparator);

        if (!difference.empty()) {
            const std::string& move = difference[0];
            moveChain.push_back(move.substr(0, 4));
            break;
            /* bool stockfishMove = std::binary_search(stockfishPerftDivide.begin(), stockfishPerftDivide.end(), move);
            if (stockfishMove) {
                moveChain.push_back("stockfish");
            }
            else {
                moveChain.push_back("faris");
            }
            */
        }
        
        // If we got here, stockfish and faris moves are equivalent, but their node counts may differ
        bool addedToChain = false;
        for (const std::string& stockfishDivide : stockfishPerftDivide) {
            std::string_view stockfishMove { stockfishDivide.c_str(), 4 };
            std::string_view stockfishNodeCount { stockfishDivide.c_str() + 6 };
            for (const std::string& farisDivide : farisPerftDivide) {
                std::string_view farisMove { farisDivide.c_str(), 4 };
                if (farisMove == stockfishMove) {
                    std::string_view farisNodeCount { farisDivide.c_str() + 6 };
                    if (farisNodeCount != stockfishNodeCount) {
                        addedToChain = true;
                        moveChain.emplace_back(stockfishMove);
                        ApplyMove(b, farisMove);
                        c = ToggleColor(c);
                        Fen newFen {
                            .board = b,
                            .colorToMove = c,
                        };
                        fenStr = ToFen(newFen);
                    }
                    break;
                }
            }
            if (addedToChain) { 
                break;
            }
        }
    }
    
    for (int i = 0; i < moveChain.size() - 1; i++) {
        std::cerr << moveChain[i] << " -> ";
    }
    std::cerr << moveChain.back() << std::endl;
}

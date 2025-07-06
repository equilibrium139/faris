#include "board.h"
#include <cassert>
#include "movegen.h"
#include <iostream>
#include "uci.h"

int maxDepth;

void PrintMove(const Move& move) {
    switch (move.type) {
        case PieceType::Pawn:
            std::cout << "p";
            break;
        case PieceType::Knight:
            std::cout << "n";
            break;
        case PieceType::Bishop:
            std::cout << "b";
            break;
        case PieceType::Rook:
            std::cout << "r";
            break;
        case PieceType::Queen:
            std::cout << "q";
            break;
        case PieceType::King:
            std::cout << "k";
            break;
    }
    char fromFile = (move.from % 8) + 'a';
    char fromRank = move.from / 8 + '1';
    char toFile = (move.to % 8) + 'a';
    char toRank = move.to / 8 + '1';
    std::cout << fromFile << fromRank << toFile << toRank << '\n';
}

int main(int argc, char** argv) {
    maxDepth = 10;
    ProcessInput();
    return 0;
}

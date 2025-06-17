#pragma once

#include "board.h"
#include <string>

// TODO: move elsewhere and rename to something generic like GameState. Also figure out what the hell halfmoveClock and fullmoveNumber are
struct Fen {
    Board board;
    int halfmoveClock;
    int fullmoveNumber;
    Color colorToMove;
    bool operator==(const Fen& other) const {
        return board == other.board && halfmoveClock == other.halfmoveClock && fullmoveNumber == other.fullmoveNumber && colorToMove == other.colorToMove;
    }
    bool operator!=(const Fen& other) const {
        return !(*this == other);
    }
};

// Assuming well-formed fen, minimal error checking 
Fen ParseFen(const std::string& fen);
std::string ToFen(const Fen& fen);

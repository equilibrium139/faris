#include "board.h"
#include <string>
#include <stdexcept>

// TODO: move elsewhere and rename to something generic like GameState. Also figure out what the hell halfmoveClock and fullmoveNumber are
struct Fen {
    Board board;
    int halfmoveClock;
    int fullmoveNumber;
    bool whiteTurn;
    bool operator==(const Fen& other) const {
        return board == other.board && halfmoveClock == other.halfmoveClock && fullmoveNumber == other.fullmoveNumber && whiteTurn == other.whiteTurn;
    }
    bool operator!=(const Fen& other) const {
        return !(*this == other);
    }
};

// Assuming well-formed fen, minimal error checking 
Fen parseFEN(const std::string& fen);
std::string toFen(const Fen& fen);
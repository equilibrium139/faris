#include "board.h"
#include <string>
#include <stdexcept>

struct Fen {
    Board board;
    int halfmoveClock;
    int fullmoveNumber;
    bool whiteTurn;
};

// Assuming well-formed fen, minimal error checking 
Fen parseFEN(const std::string& fen);
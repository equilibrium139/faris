#include "board.h"
#include <string>
#include <stdexcept>

// TODO: move elsewhere and rename to something generic like GameState 
struct Fen {
    Board board;
    int halfmoveClock;
    int fullmoveNumber;
    bool whiteTurn;
};

// Assuming well-formed fen, minimal error checking 
Fen parseFEN(const std::string& fen);
std::string toFen(const Fen& fen);
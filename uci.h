#include "board.h"

struct UCIState {
    Board board;
    Color colorToMove;
    int wtime;
    int btime;
    int winc;
    int binc;
};

void ProcessInput();

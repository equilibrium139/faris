#include "board.h"

struct UCIState {
    Board board;
    Color colorToMove = White;
    int wtime;
    int btime;
    int winc = 0;
    int binc = 0;
    bool useNewFeature = false;
};

void ProcessInput();

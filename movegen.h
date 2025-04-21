#pragma once

#include "board.h"
#include <vector>

int perftest(const Board& board, int depth);
std::vector<Board> genMoves(const Board& board, bool whiteTurn);

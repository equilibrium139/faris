#pragma once 

#include "board.h"
#include "movegen.h"

// Searches for the best move for colorToMove using the minimax algorithm. The move tree is searched up to a depth of maxDepth
Move Search(const Board& board, Color colorToMove, int time, int inc);

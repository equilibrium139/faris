#pragma once 

#include "board.h"
#include "movegen.h"
#include <cstdint>
#include <unordered_map>

extern std::unordered_map<std::uint64_t, int> threefoldRepetitionTable;
// Searches for the best move for colorToMove using the minimax algorithm. The move tree is searched up to a depth of maxDepth
Move Search(const Board& board, Color colorToMove, int time, int inc);

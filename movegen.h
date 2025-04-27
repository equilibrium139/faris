#pragma once

#include "board.h"
#include <span>
#include <vector>

int perftest(const Board& board, int depth);
std::vector<Board> genMoves(const Board& board, bool whiteTurn);
void removePiece(Board& board, Bitboard captureSquare, std::span<Bitboard, 6> pieces);
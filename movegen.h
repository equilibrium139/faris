#pragma once

#include "board.h"
#include <span>
#include <vector>

int perftest(const Board& board, int depth, bool whiteTurn);
void removePiece(int squareIndex, std::span<Bitboard, 6> pieces);
void removePiece(int squareIndex, Board &board, bool white);
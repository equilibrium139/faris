#pragma once

#include "board.h"
#include <span>
#include <cstdint>

extern int maxDepth;
extern bool enablePerftDiagnostics;
std::uint64_t perftest(const Board& board, int depth, Color colorToMove);
void RemovePiece(int squareIndex, std::span<Bitboard, 6> pieceBB);
void RemovePiece(int squareIndex, Board &board, Color colorToRemove);

int IncrementCastles();
int IncrementCaptures();
int IncrementEnPassant();

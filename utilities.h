#pragma once 
#include "board.h"
#include "movegen.h"

void PrettyPrint(Bitboard bb);
void PrettyPrint(const Board& board);
Piece PieceAt(int squareIndex, const Board &board);
PieceType PieceTypeAt(Square square, const Board& board);
PieceType RemovePiece(Square square, Board& board, Color color);

void MakeMove(const Move &move, Board &board, Color colorToMove);
void MakeMove(const Move &move, Board &board, Color colorToMove, std::uint64_t &boardHash);
void UndoMove(const Move& move, Board& board, Color colorToMove);
bool underThreat(const Board &board, int squareIndex, Color threatColor);
bool InCheck(const Board& board, Color color);

#include "board.h"

void PrettyPrint(Bitboard bb);
void PrettyPrint(const Board& board);
Piece PieceAt(int squareIndex, const Board &board);
void RemovePiece(int squareIndex, Board& board, Color colorToRemove);

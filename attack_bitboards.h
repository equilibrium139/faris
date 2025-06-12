#include <array>
#include "board.h"

extern std::array<Bitboard, 64> knightAttacks;
Bitboard RookAttack(Square square, Bitboard occupancy);
Bitboard BishopAttack(Square square, Bitboard occupancy);

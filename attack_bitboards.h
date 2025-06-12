#include <array>
#include "board.h"

extern std::array<Bitboard, 64> knightAttacks;
extern std::array<std::array<Bitboard, 64>, 2> pawnAttacks; // [color][square]
extern std::array<Bitboard, 64> kingAttacks;
Bitboard RookAttack(Square square, Bitboard occupancy);
Bitboard BishopAttack(Square square, Bitboard occupancy);

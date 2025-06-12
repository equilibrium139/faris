#include "attack_bitboards.h"
#include "board.h"
#include "magic.h"
#include <cstdlib>

static std::array<Bitboard, 64> GenKnightAttacks() {
    constexpr int knightMoves[8] = {6, 10, 15, 17, -6, -10, -15, -17};
    std::array<Bitboard, 64> attacks{};
    for (Square s = 0; s < 64; s++) {
        int squareFile = s % 8;
        int squareRank = s / 8;
        for (int move : knightMoves) {
            Square sNew = s + move;
            if (sNew < 0 || sNew >= 64) {
                continue;
            }

            int newFile = sNew % 8;
            int newRank = sNew / 8;

            bool validMove = (std::abs(newFile - squareFile) == 1 && std::abs(newRank - squareRank) == 2) ||
                             (std::abs(newFile - squareFile) == 2 && std::abs(newRank - squareRank) == 1);
            if (validMove) {
                Bitboard sNewBB = ToBitboard(sNew);
                attacks[s] |= sNewBB;
            }
        }
    }
    return attacks;
}

std::array<Bitboard, 64> knightAttacks = GenKnightAttacks();

Bitboard RookAttack(Square square, Bitboard occupancy) {
    std::uint64_t magic = rookMagic[square];
    Bitboard mask = occupancy & rookMask[square];
    const unsigned int rookAttackIdx = (mask * magic) >> 52; 
    return rookAttacks[square][rookAttackIdx];
}

Bitboard BishopAttack(Square square, Bitboard occupancy) {
    std::uint64_t magic = bishopMagic[square];
    Bitboard mask = occupancy & bishopMask[square];
    const unsigned int bishopAttackIdx = (mask * magic) >> 55; 
    return bishopAttacks[square][bishopAttackIdx];
}

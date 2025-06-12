#include "attack_bitboards.h"
#include "board.h"
#include "magic.h"
#include <cstdlib>
#include <immintrin.h>

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

static std::array<std::array<Bitboard, 64>, 2> GenPawnAttacks() { 
    constexpr Color colors[2] = {Color::White, Color::Black };
    std::array<std::array<Bitboard, 64>, 2> attacks{}; 
    for (Color c : colors) {
        int moves[2] = { 7, 9 };
        if (c == Color::Black) {
            moves[0] = -7;
            moves[1] = -9;
        }
        for (Square s = 0; s < 64; s++) {
            attacks[c][s] = 0;
            int squareFile = s % 8;
            int squareRank = s / 8;
            for (int move : moves) {
                Square sNew = s + move;
                if (sNew < 0 || sNew >= 64) {
                    continue;
                }
                int newFile = sNew % 8;
                int newRank = sNew / 8;
                bool validMove = (std::abs(newFile - squareFile) == 1 && std::abs(newRank - squareRank) == 1);
                if (validMove) {
                    attacks[c][s] |= ToBitboard(sNew);
                }
            }
        }
    }
    return attacks;
}

static std::array<Bitboard, 64> GenKingAttacks() {
    constexpr int kingMoves[8] = {7, 9, -7, -9, 8, -8, 1, -1};
    std::array<Bitboard, 64> attacks{};
    for (Square s = 0; s < 64; s++) {
        int squareFile = s % 8;
        int squareRank = s / 8;
        for (int move : kingMoves) {
            Square sNew = s + move;
            if (sNew < 0 || sNew >= 64) {
                continue;
            }
            int newFile = sNew % 8;
            int newRank = sNew / 8;
            bool validMove = (std::abs(newFile - squareFile) <= 1 && std::abs(newRank - squareRank) <= 1);
            if (validMove) {
                attacks[s] |= ToBitboard(sNew);
            }
        }
    }
    return attacks;
}

std::array<Bitboard, 64> knightAttacks = GenKnightAttacks();
std::array<std::array<Bitboard, 64>, 2> pawnAttacks = GenPawnAttacks();
std::array<Bitboard, 64> kingAttacks = GenKingAttacks();

Bitboard RookAttack(Square square, Bitboard occupancy) {
#ifdef USE_PEXT
    std::uint64_t idx = _pext_u64(occupancy, rookMask[square]);
    return rookAttacks[square][idx];
#else
    std::uint64_t magic = rookMagic[square];
    Bitboard mask = occupancy & rookMask[square];
    const unsigned int rookAttackIdx = (mask * magic) >> 52; 
    return rookAttacks[square][rookAttackIdx];
#endif
}

Bitboard BishopAttack(Square square, Bitboard occupancy) {
#ifdef USE_PEXT
    std::uint64_t idx = _pext_u64(occupancy, bishopMask[square]);
    return bishopAttacks[square][idx];
#else
    std::uint64_t magic = bishopMagic[square];
    Bitboard mask = occupancy & bishopMask[square];
    const unsigned int bishopAttackIdx = (mask * magic) >> 55; 
    return bishopAttacks[square][bishopAttackIdx];
#endif
}

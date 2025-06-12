#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "board.h"
#include "utilities.h"

static constexpr int rookMoves[4] = { 8, -8, 1, -1 };
static constexpr int rookShift = 64 - 12;
static constexpr int bishopMoves[4] = { 7, 9, -7, -9 };

static std::uint64_t rookMagics[64];
static Bitboard rookMasks[64];
static Bitboard rookAttacks[64][4096];


static bool OnEdge(Square square) {
    int file = square % 8;
    int rank = square / 8;
    return file == 0 || file == 7 || rank == 0 || rank == 7; 
}

static bool RookReachedEdge(Square square, int move) {
    int file = square % 8;
    int rank = square / 8;
    return move == 8 && rank == 7 || move == -8 && rank == 0 || 
           move == 1 && file == 7 || move == -1 && file == 0;
}

static bool ValidRookMove(Square from, Square to) {
    if (to < 0 || to >= 64) {
        return false;
    }
    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;
    return std::abs(fromFile - toFile) == 0 || std::abs(fromRank - toRank) == 0;
}

static bool ValidBishopMove(Square from, Square to) {
    if (to < 0 || to >= 64) {
        return false;
    }
    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;
    return std::abs(fromFile - toFile) == 1 && std::abs(fromRank - toRank) == 1;
}

static Bitboard GenRookMask(Square square) {
    Bitboard mask = 0;
    for (int move : rookMoves) {
        Square from = square;
        for (Square s = square + move; !RookReachedEdge(s, move) && ValidRookMove(from, s); s += move) {
            mask |= ToBitboard(s);
            from = s;
        }
    }

    return mask;
}

static Bitboard GenBishopMask(Square square) {
    Bitboard mask = 0;
    for (int move : bishopMoves) {
        for (Square s = square + move; !OnEdge(s) && ValidBishopMove(square, s); s += move) {
            mask |= ToBitboard(s);
        }
    }

    return mask;
}

static std::uint64_t RandomUint64() {
    std::uint64_t u1, u2, u3, u4;
    u1 = (std::uint64_t)(random()) & 0xFFFF;
    u2 = (std::uint64_t)(random()) & 0xFFFF;
    u3 = (std::uint64_t)(random()) & 0xFFFF;
    u4 = (std::uint64_t)(random()) & 0xFFFF;
    return u1 | (u2 << 16) | (u3 << 32) | (u4 << 48);
}

// apparently this finds magics faster than the way I was doing before
// https://www.chessprogramming.org/Looking_for_Magics 
static std::uint64_t GenMagic() {
    return RandomUint64() & RandomUint64() & RandomUint64(); 
}

static Bitboard GenRookAttackBitboard(Square square, Bitboard occupancy) {
    Bitboard attackBitboard = 0;
    for (int move : rookMoves) {
        if (!ValidRookMove(square, square + move)) continue;
        for (Square s = square + move; ; s += move) {
            Bitboard sBB = ToBitboard(s);
            attackBitboard |= sBB;
            if ((occupancy & sBB) || RookReachedEdge(s, move)) { 
                break;
            }
        }
    }
    return attackBitboard;
}

static void GenRookAttackBitboardArray() {
    for (Square square = 0; square < 64; square++) {
        Bitboard mask = GenRookMask(square);
        rookMasks[square] = mask;
        std::vector<int> oneIndices;
        for (int i = 0; i < 64; i++) {
            if ((1ULL << i) & mask) {
                oneIndices.push_back(i);
            }
        }
        // 4096 is an upper bound
        constexpr std::uint32_t countPermutations = 4096;

        bool idxUsed[countPermutations];
        Bitboard occupancyPermutations[countPermutations];
        Bitboard attackBitboards[countPermutations];
        for (std::uint32_t i = 0; i < countPermutations; i++) {
            occupancyPermutations[i] = 0;
            for (std::uint32_t j = 0; j < oneIndices.size(); j++) {
                if (i & (1ULL << j)) {
                    occupancyPermutations[i] |= (1ULL << oneIndices[j]);
                }
            }
            attackBitboards[i] = GenRookAttackBitboard(square, occupancyPermutations[i]);
        }
        std::uint64_t magic;
        while (true) {
            magic = GenMagic();
            // see chessprogramming link above for this weirdness
            if (std::popcount((mask * magic) & 0xFF00000000000000ULL) < 6) { continue; }
            bool goodMagic = true;
            std::fill(idxUsed, idxUsed + countPermutations, false);
            for (std::uint32_t i = 0; i < countPermutations; i++) {
                Bitboard occupancyPermutation = occupancyPermutations[i];
                int permutationIdx = (occupancyPermutation * magic) >> rookShift;
                // bad collision = bad magic number
                if (idxUsed[permutationIdx] && rookAttacks[square][permutationIdx] != attackBitboards[i]) {
                    goodMagic = false;
                    break;
                }
                else {
                    idxUsed[permutationIdx] = true;
                    rookAttacks[square][permutationIdx] = attackBitboards[i];
                }
            }
            if (goodMagic) {
                std::cout << magic << std::endl;
                rookMagics[square] = magic;
                break;
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        // std::cerr << "Usage: magicgen <output_file>" << std::endl;            
        // return 1;
    }

    // std::string outputFile = argv[1];
    // std::cout << outputFile << std::endl;
    GenRookAttackBitboardArray();
}

#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#include "board.h"
#include "utilities.h"

static constexpr int rookMoves[4] = { 8, -8, 1, -1 };
static constexpr int rookShift = 64 - 12; // 12 is upper bound for popcount(rookMasks[i])
static constexpr int bishopMoves[4] = { 7, 9, -7, -9 };
static constexpr int bishopShift = 64 - 9; // 9 is upper bound for popcount(bishopMasks[i])

static std::uint64_t rookMagics[64];
static Bitboard rookMasks[64];
static Bitboard rookAttacks[64][4096];

static std::uint64_t bishopMagics[64];
static Bitboard bishopMasks[64];
static Bitboard bishopAttacks[64][512];

static bool BishopReachedEdge(Square square, int move) {
    int file = square % 8;
    int rank = square / 8;
    return move == 7 && (file == 0 || rank == 7) ||
           move == 9 && (file == 7 || rank == 7) ||
           move == -7 && (file == 7 || rank == 0) ||
           move == -9 && (file == 0 || rank == 0);
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
        Square from = square;
        for (Square s = square + move; !BishopReachedEdge(s, move) && ValidBishopMove(from, s); s += move) {
            mask |= ToBitboard(s);
            from = s;
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

static Bitboard GenBishopAttackBitboard(Square square, Bitboard occupancy) {
    Bitboard attackBitboard = 0;
    for (int move : bishopMoves) {
        if (!ValidBishopMove(square, square + move)) continue;
        for (Square s = square + move; ; s += move) {
            Bitboard sBB = ToBitboard(s);
            attackBitboard |= sBB;
            if ((occupancy & sBB) || BishopReachedEdge(s, move)) { 
                break;
            }
        }
    }
    return attackBitboard;
}

static void GenAttackBitboardArray(bool rook) {
    auto GenMaskFunc = rook ? GenRookMask : GenBishopMask;
    Bitboard* masks = rook ? rookMasks : bishopMasks;
    const unsigned int countPermutations = rook ? 4096 : 512;
    auto GenAttackBitboardFunc = rook ? GenRookAttackBitboard : GenBishopAttackBitboard;
    auto magics = rook ? rookMagics : bishopMagics;
    const int shift = rook ? rookShift : bishopShift;

    for (Square square = 0; square < 64; square++) {
        auto squareAttacks = rook ? rookAttacks[square] : bishopAttacks[square];
        Bitboard mask = GenMaskFunc(square);
        masks[square] = mask;
        std::vector<int> oneIndices;
        for (int i = 0; i < 64; i++) {
            if ((1ULL << i) & mask) {
                oneIndices.push_back(i);
            }
        }

        std::vector<bool> idxUsed(countPermutations, false);
        std::vector<Bitboard> occupancyPermutations(countPermutations);
        std::vector<Bitboard> attackBitboards(countPermutations);
        for (std::uint32_t i = 0; i < countPermutations; i++) {
            occupancyPermutations[i] = 0;
            for (std::uint32_t j = 0; j < oneIndices.size(); j++) {
                if (i & (1ULL << j)) {
                    occupancyPermutations[i] |= (1ULL << oneIndices[j]);
                }
            }
            attackBitboards[i] = GenAttackBitboardFunc(square, occupancyPermutations[i]);
        }
        std::uint64_t magic;
        while (true) {
            magic = GenMagic();
            // see chessprogramming link above for this weirdness
            if (std::popcount((mask * magic) & 0xFF00000000000000ULL) < 6) { continue; }
            bool goodMagic = true;
            idxUsed.assign(idxUsed.size(), false);
            for (std::uint32_t i = 0; i < countPermutations; i++) {
                Bitboard occupancyPermutation = occupancyPermutations[i];
                int permutationIdx = (occupancyPermutation * magic) >> shift;
                // bad collision = bad magic number
                if (idxUsed[permutationIdx] && squareAttacks[permutationIdx] != attackBitboards[i]) {
                    goodMagic = false;
                    break;
                }
                else {
                    idxUsed[permutationIdx] = true;
                    squareAttacks[permutationIdx] = attackBitboards[i];
                }
            }
            if (goodMagic) {
                magics[square] = magic;
                break;
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: magicgen <output_file>" << std::endl;            
        return 1;
    }

    std::string outputFilePath = argv[1];
    std::ofstream outputFile{outputFilePath};

    GenAttackBitboardArray(true);
    GenAttackBitboardArray(false);

    outputFile << "#pragma once\n"
                  "#include <cstdint>\n"
                  "static constexpr std::uint64_t rookMask[64] = {";

    for (int i = 0; i < 64; i++) {
        outputFile << rookMasks[i] << "ULL,";
    }

    outputFile << "};\nstatic constexpr std::uint64_t rookMagic[64] = {";
    
    for (int i = 0; i < 64; i++) {
        outputFile << rookMagics[i] << "ULL,";
    }

    outputFile << "};\nstatic constexpr std::uint64_t rookAttacks[64][4096] = {";

    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 4096; j++) {
            outputFile << rookAttacks[i][j] << "ULL,";
        }
    }

    outputFile << "};\nstatic constexpr std::uint64_t bishopMask[64] = {";

    for (int i = 0; i < 64; i++) {
        outputFile << bishopMasks[i] << "ULL,";
    }

    outputFile << "};\nstatic constexpr std::uint64_t bishopMagic[64] = {";
    
    for (int i = 0; i < 64; i++) {
        outputFile << bishopMagics[i] << "ULL,";
    }

    outputFile << "};\nstatic constexpr std::uint64_t bishopAttacks[64][512] = {";

    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 512; j++) {
            outputFile << bishopAttacks[i][j] << "ULL,";
        }
    }

    outputFile << "};\n";
}

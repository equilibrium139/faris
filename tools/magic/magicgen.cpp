#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

#include "board.h"
#include "utilities.h"

static constexpr int rookMoves[4] = { 8, -8, 1, -1 };
static constexpr int bishopMoves[4] = { 7, 9, -7, -9 };

static std::uint64_t rookMagics[64];
static unsigned int rookShifts[64];
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

static std::uint64_t GenMagic(int desiredHamming) {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    static std::uniform_int_distribution<uint64_t> dist(1, 63);
    // return distrib(rng);

    std::uint64_t magic = 1; // ensure number is odd
    int count = 1;
    while (count < desiredHamming) {
        int b = dist(rng);
        if (!(magic & (1ULL << b))) {
            magic |= (1ULL << b);
            ++count;
        }
    }

    return magic;
}

static Bitboard GenRookAttackBitboard(Square square, Bitboard occupancy) {
    Bitboard attackBitboard = 0;
    for (int move : rookMoves) {
        Square from = square;
        for (Square s = square + move; ValidRookMove(square, from); s += move) {
            Bitboard sBB = ToBitboard(s);
            attackBitboard |= sBB;
            from = s;
            if (occupancy & sBB) { 
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
        int countOnes = oneIndices.size();
        int shiftAmount = 64 - countOnes;
        rookShifts[square] = shiftAmount;
        const std::uint32_t maxPermutationValue = std::pow(2, countOnes) - 1;
        std::uint64_t magic;
        Bitboard attackBitboards[4096];
        while (true) {
            magic = GenMagic(countOnes);
            bool goodMagic = true;
            std::fill(attackBitboards, attackBitboards + 4096, 0);
            std::unordered_map<int, Bitboard> occupancyIdxBitboardMap;
            for (std::uint32_t i = 0; i <= maxPermutationValue; i++) {
                Bitboard occupancyPermutation = 0;
                for (int j = 0; j < oneIndices.size(); j++) {
                    if (i & (1ULL << j)) {
                        occupancyPermutation |= (1ULL << oneIndices[j]);
                    }
                }
                int permutationIdx = (occupancyPermutation * magic) >> shiftAmount;
                Bitboard attackBitboard = GenRookAttackBitboard(square, occupancyPermutation);
                attackBitboards[permutationIdx] = attackBitboard;
                // bad collision = bad magic number
                if (occupancyIdxBitboardMap.contains(permutationIdx) && occupancyIdxBitboardMap[permutationIdx] != attackBitboard) {
                    goodMagic = false;
                    break;
                } 
                else {
                    occupancyIdxBitboardMap[permutationIdx] = attackBitboard;
                }
            }
            if (goodMagic) {
                break;
            }
        }
        rookMagics[square] = magic;
        std::copy(attackBitboards, attackBitboards + 4096, rookAttacks[square]);
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

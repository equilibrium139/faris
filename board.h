#pragma once

#include <cstdint>

using Bitboard = std::uint64_t;
static constexpr Bitboard WP_START = 0xFF00ULL;
static constexpr Bitboard WN_START = (1 << 1) | (1 << 6);
static constexpr Bitboard WB_START = (1 << 2) | (1 << 5);
static constexpr Bitboard WR_START = (1)     | (1 << 7);
static constexpr Bitboard WQ_START = (1 << 3);
static constexpr Bitboard WK_START = (1 << 4);
static constexpr Bitboard BP_START = WP_START << (5 * 8);
static constexpr Bitboard BN_START = WN_START << (7 * 8);
static constexpr Bitboard BB_START = WB_START << (7 * 8);
static constexpr Bitboard BR_START = WR_START << (7 * 8);
static constexpr Bitboard BQ_START = WQ_START << (7 * 8);
static constexpr Bitboard BK_START = WK_START << (7 * 8);

struct Board {
        std::uint64_t whitePawns;
        std::uint64_t whiteKnights;
        std::uint64_t whiteBishops;
        std::uint64_t whiteRooks;
        std::uint64_t whiteQueens;
        std::uint64_t whiteKing;
        std::uint64_t blackPawns;
        std::uint64_t blackKnights;
        std::uint64_t blackBishops;
        std::uint64_t blackRooks;
        std::uint64_t blackQueen;
        std::uint64_t blackKing;
        
        std::uint64_t whitePieces() const {
            return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
        }

        std::uint64_t blackPieces() const {
            return blackPawns | blackKnights | blackBishops | blackRooks | blackQueen | blackKing;
        }

        std::uint64_t allPieces() const {
            return whitePieces() | blackPieces();
        }

        std::uint64_t emptySquares() const {
            return ~allPieces();
        }

        Board() {
            whitePawns = WP_START;
            whiteKnights = WN_START;
            whiteBishops = WB_START;
            whiteRooks = WR_START;
            whiteQueens = WQ_START;
            whiteKing = WK_START;
            blackPawns = BP_START;
            blackKnights = BN_START;
            blackBishops = BB_START;
            blackRooks = BR_START;
            blackQueen = BQ_START;
            blackKing = BK_START;
        }
};


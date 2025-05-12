#pragma once

#include <cstdint>
#include <span>

using Bitboard = std::uint64_t;
static constexpr Bitboard WP_START = 0xFF00ULL;
static constexpr Bitboard WN_START = (1 << 1) | (1 << 6);
static constexpr Bitboard WB_START = (1 << 2) | (1 << 5);
static constexpr Bitboard WR_START = (1) | (1 << 7);
static constexpr Bitboard WQ_START = (1 << 3);
static constexpr Bitboard WK_START = (1 << 4);
static constexpr Bitboard BP_START = WP_START << (5 * 8);
static constexpr Bitboard BN_START = WN_START << (7 * 8);
static constexpr Bitboard BB_START = WB_START << (7 * 8);
static constexpr Bitboard BR_START = WR_START << (7 * 8);
static constexpr Bitboard BQ_START = WQ_START << (7 * 8);
static constexpr Bitboard BK_START = WK_START << (7 * 8);
static constexpr int PAWN_OFFSET = 0;
static constexpr int KNIGHT_OFFSET = 1;
static constexpr int BISHOP_OFFSET = 2;
static constexpr int ROOK_OFFSET = 3;
static constexpr int QUEEN_OFFSET = 4;
static constexpr int KING_OFFSET = 5;
static constexpr int WHITE_PIECE_OFFSET = 0;
static constexpr int BLACK_PIECE_OFFSET = 6;

static constexpr int COUNT_BITBOARDS = 12;

enum class PieceType {
    Pawn, Knight, Bishop, Rook, Queen, King, None
};

struct Piece {
    PieceType type;
    bool color; // 0-black 1-white
};

struct Board {
    union {
        struct {
            Bitboard whitePawns;
            Bitboard whiteKnights;
            Bitboard whiteBishops;
            Bitboard whiteRooks;
            Bitboard whiteQueens;
            Bitboard whiteKing;
            Bitboard blackPawns;
            Bitboard blackKnights;
            Bitboard blackBishops;
            Bitboard blackRooks;
            Bitboard blackQueens;
            Bitboard blackKing;
        };
        Bitboard pieces[COUNT_BITBOARDS];
    };

    std::uint8_t enPassant;
    // These only indicate if the king and corresponding rook have moved,
    // not if castling is currently possible (intermediate squares clear/unattacked).
    bool whiteKingsideCastlingRight = true;
    bool whiteQueensideCastlingRight = true;
    bool blackKingsideCastlingRight = true;
    bool blackQueensideCastlingRight = true;

    std::uint64_t whitePieces() const {
        return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
    }

    std::uint64_t blackPieces() const {
        return blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
    }

    std::uint64_t allPieces() const { return whitePieces() | blackPieces(); }

    std::uint64_t emptySquares() const { return ~allPieces(); }

    std::span<const Bitboard, 6> whiteBitboards() const { return std::span<const Bitboard, 6>{&pieces[WHITE_PIECE_OFFSET], 6}; }
    std::span<const Bitboard, 6> blackBitboards() const { return std::span<const Bitboard, 6>{&pieces[BLACK_PIECE_OFFSET], 6}; }
    std::span<Bitboard, 6> whiteBitboards() { return std::span<Bitboard, 6>{&pieces[WHITE_PIECE_OFFSET], 6}; }
    std::span<Bitboard, 6> blackBitboards() { return std::span<Bitboard, 6>{&pieces[BLACK_PIECE_OFFSET], 6}; }

    Board(bool startingPosition = true) {
        if (startingPosition) {
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
            blackQueens = BQ_START;
            blackKing = BK_START;
            enPassant = 0;
        } else {
            whitePawns = 0;
            whiteKnights = 0;
            whiteBishops = 0;
            whiteRooks = 0;
            whiteQueens = 0;
            whiteKing = 0;
            blackPawns = 0;
            blackKnights = 0;
            blackBishops = 0;
            blackRooks = 0;
            blackQueens = 0;
            blackKing = 0;
            enPassant = 0;
        }
    }
};

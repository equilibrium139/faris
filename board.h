#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <span>


enum class Rank : std::uint8_t {
    First, Second, Third, Fourth, Fifth, Sixth, Seventh, Eighth
};

using Bitboard = std::uint64_t;
using Square = std::int8_t;
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
static constexpr int COLOR_OFFSET[2] = { 0, 6 }; // Matches order of Color enum
static constexpr int COUNT_BITBOARDS = 12;
static constexpr Square STARTING_KING_SQUARE[2] = { 4, 60 };

static constexpr Bitboard RANK_MASK[8] = { 0xFFULL, 0xFFULL << 8, 0xFFULL << 16, 0xFFULL << 24, 0xFFULL << 32, 0xFFULL << 40, 0xFFULL << 48, 0xFFULL << 56 };
static constexpr Bitboard PROMOTION_RANK_MASK[2] = { (Bitboard)0xFF << 56, (Bitboard)0xFF };
// TODO: finish file mask and use for pawn attack gen
static constexpr Bitboard fileAMask = 0x1ULL | 0x1ULL << 8 | 0x1ULL << 16 | 0x1ULL << 24 | 0x1ULL << 32 | 0x1ULL << 40 | 0x1ULL << 48 | 0x1ULL << 56;
static constexpr Bitboard FILE_MASK[8] = { fileAMask, fileAMask << 1, fileAMask << 2, fileAMask << 3, fileAMask << 4, fileAMask << 5, fileAMask << 6, fileAMask << 7 };

// TODO: check if countr_zero is the reason Windows build is significantly slower than WSL build
static constexpr Square LSB(Bitboard b) {
    return std::countr_zero(b);
}

static constexpr Square PopLSB(Bitboard& b) {
    Square s = std::countr_zero(b);
    b &= b - 1;
    return s;
}

static constexpr Bitboard ToBitboard(Square s) {
    return (Bitboard)1 << s;
}

enum class PieceType : std::uint8_t {
    Pawn, Knight, Bishop, Rook, Queen, King, None
};

constexpr PieceType promotionTypes[4] = {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight};

enum Color {
    White, Black
};

inline Color ToggleColor(Color color) {
    return Color(1 - color);
}

struct Piece {
    PieceType type;
    Color color; 
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
        struct {
            Bitboard bitboards2D[2][6];
        };
        Bitboard bitboards[COUNT_BITBOARDS];
    };

    Square enPassant = -1;
    // These only indicate if the king and corresponding rook have moved,
    // not if castling is currently possible (intermediate squares clear/unattacked).
    bool shortCastlingRight[2] = { true, true };
    bool longCastlingRight[2] = { true,  true };

    void RemoveCastlingRights(Color color) {
        shortCastlingRight[color] = false;
        longCastlingRight[color] = false;
    }

    Bitboard WhiteOccupancy() const {
        return whitePawns | whiteKnights | whiteBishops | whiteRooks | whiteQueens | whiteKing;
    }

    Bitboard BlackOccupancy() const {
        return blackPawns | blackKnights | blackBishops | blackRooks | blackQueens | blackKing;
    }

    Bitboard Occupancy(Color color) const {
        return color == Color::White ? WhiteOccupancy() : BlackOccupancy(); 
    }

    Bitboard Occupancy() const { return WhiteOccupancy() | BlackOccupancy(); }

    Bitboard EmptySquares() const { return ~Occupancy(); }

    bool Valid() const {
        for (int i = 0; i < COUNT_BITBOARDS; i++) {
            for (int j = i + 1; j < COUNT_BITBOARDS; j++) {
                if ((bitboards[i] & bitboards[j]) != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    std::span<const Bitboard, 6> WhiteBitboards() const { return std::span<const Bitboard, 6>{&bitboards[WHITE_PIECE_OFFSET], 6}; }
    std::span<const Bitboard, 6> BlackBitboards() const { return std::span<const Bitboard, 6>{&bitboards[BLACK_PIECE_OFFSET], 6}; }
    std::span<const Bitboard, 6> Bitboards(Color color) const { return color == Color::Black ? BlackBitboards() : WhiteBitboards(); }

    std::span<Bitboard, 6> WhiteBitboards() { return std::span<Bitboard, 6>{&bitboards[WHITE_PIECE_OFFSET], 6}; }
    std::span<Bitboard, 6> BlackBitboards() { return std::span<Bitboard, 6>{&bitboards[BLACK_PIECE_OFFSET], 6}; }
    std::span<Bitboard, 6> Bitboards(Color color) { return color == Color::Black ? BlackBitboards() : WhiteBitboards(); }

    Bitboard Pawns(Color c) const { return bitboards[PAWN_OFFSET + COLOR_OFFSET[c]]; }
    Bitboard Knights(Color c) const { return bitboards[KNIGHT_OFFSET + COLOR_OFFSET[c]]; }
    Bitboard Bishops(Color c) const { return bitboards[BISHOP_OFFSET + COLOR_OFFSET[c]]; }
    Bitboard Rooks(Color c) const { return bitboards[ROOK_OFFSET + COLOR_OFFSET[c]]; }
    Bitboard Queens(Color c) const { return bitboards[QUEEN_OFFSET + COLOR_OFFSET[c]]; }
    Bitboard Kings(Color c) const { return bitboards[KING_OFFSET + COLOR_OFFSET[c]]; }

    void Move(PieceType type, Color c, Square from, Square to) {
        bitboards[(int)type + COLOR_OFFSET[c]] &= ~ToBitboard(from);
        bitboards[(int)type + COLOR_OFFSET[c]] |= ToBitboard(to);
    }
    
    void PromotePawn(PieceType promotionType, Color c, Square square) {
        Bitboard squareBB = ToBitboard(square);
        bitboards2D[c][(int)PieceType::Pawn] &= ~squareBB;
        bitboards2D[c][(int)promotionType] |= squareBB;
    }

    bool operator==(const Board& other) const {
        return whitePawns == other.whitePawns &&
               whiteKnights == other.whiteKnights &&
               whiteBishops == other.whiteBishops &&
               whiteRooks == other.whiteRooks &&
               whiteQueens == other.whiteQueens &&
               whiteKing == other.whiteKing &&
               blackPawns == other.blackPawns &&
               blackKnights == other.blackKnights &&
               blackBishops == other.blackBishops &&
               blackRooks == other.blackRooks &&
               blackQueens == other.blackQueens &&
               blackKing == other.blackKing &&
               enPassant == other.enPassant &&
               shortCastlingRight[White] == other.shortCastlingRight[White] &&
               shortCastlingRight[Black] == other.shortCastlingRight[Black] &&
               longCastlingRight[White] == other.longCastlingRight[White] &&
               longCastlingRight[Black] == other.longCastlingRight[Black];
    }
    bool operator!=(const Board& other) const {
        return !(*this == other);
    }

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
            enPassant = -1;
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
            enPassant = -1;
        }
    }
};

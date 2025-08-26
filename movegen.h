#pragma once

#include "board.h"
#include <cstdint>
#include <vector>

struct Move {
    enum CastlingFlags : std::uint8_t {
        None = 0,
        RemovesShortCastlingRight = 1,
        RemovesLongCastlingRight = 1 << 1,
        RemovesOppShortCastlingRight = 1 << 2,
        RemovesOppLongCastlingRight = 1 << 3
    };
    Square from, to;
    PieceType type;
    PieceType capturedPieceType = PieceType::None;
    PieceType promotionType = PieceType::None;
    std::int8_t enPassantDelta = 0;
    CastlingFlags flags = None; 
    // used for killer move comparison so doesn't check all members. Should probably not use equality operator tbh
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promotionType == other.promotionType;
        // return from == other.from && to == other.to && type == other.type && capturedPieceType == other.capturedPieceType;
    }
};

inline Move::CastlingFlags operator|(Move::CastlingFlags lhs, Move::CastlingFlags rhs) {
    return static_cast<Move::CastlingFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

extern int maxDepth;
// tacticalOnly -> captures and promotions
std::vector<Move> GenMoves(const Board& board, Color colorToMove, bool tacticalOnly=false);

int IncrementCastles();
int IncrementCaptures();
int IncrementEnPassant();

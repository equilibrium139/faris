#include "transposition.h"
#include "board.h"
#include "utilities.h"
#include <cstring>
#include <random>

TT transpositionTable;

TT::TT() {
    std::random_device rd;
    std::mt19937_64 mt(rd());
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 6; j++) {
            for (int k = 0; k < 2; k++) {
                pieceZobrist[i][j][k] = mt();
            }
        }
    }
    blackToMoveZobrist = mt();
    for (std::uint64_t& value : castlingRightsZobrist) {
        value = mt();
    }
    for (std::uint64_t& value : enPassantFileZobrist) {
        value = mt();
    }
    std::memset(&table[0], 0, sizeof(TTEntry) * table.size());
}

std::uint64_t TT::Hash(const Board& board, Color colorToMove) {
    std::uint64_t hash = 0;
    for (Square square = 0; square < 64; square++) {
        Piece piece = PieceAt(square, board);
        if (piece.type != PieceType::None) { 
            hash ^= pieceZobrist[square][(int)piece.type][piece.color];
        }
    }
    hash ^= blackToMoveZobrist * (colorToMove == Black);
    unsigned int castleIndex = ((unsigned int)(board.shortCastlingRight[White])) << 3 |
                               ((unsigned int)(board.shortCastlingRight[Black])) << 2 |
                               ((unsigned int)(board.longCastlingRight[White])) << 1 |
                               ((unsigned int)(board.longCastlingRight[Black])); 
    hash ^= castlingRightsZobrist[castleIndex];
    if (board.enPassant != -1) {
        hash ^= enPassantFileZobrist[board.enPassant & 0x7]; 
    }
    return hash;
}

const TTEntry* TT::Search(const Board& board, Color colorToMove) {
    auto hash = Hash(board, colorToMove);
    return Search(board, colorToMove, hash);
}

const TTEntry* TT::Search(const Board& board, Color colorToMove, std::uint64_t hash) {
    auto index = hash % size;
    const TTEntry* entry = &table[index];
    if (entry->hash == hash) {
        return entry;
    }
    return nullptr;
}

void TT::Add(const Board& board, Color colorToMove, int depth, int score, ScoreType scoreType, const Move& bestMove) {
    auto hash = Hash(board, colorToMove);
    auto index = hash % size;
    if (table[index].hash == 0 || depth >= table[index].depth) {
        table[index] = {
            .hash = hash,
            .bestMove = bestMove,
            .score = score,
            .scoreType = scoreType,
            .depth = depth,
        };
    }
}

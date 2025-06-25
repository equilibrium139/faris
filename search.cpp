#include "search.h"
#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <vector>

static constexpr int pieceValues[7] = {100, 300, 300, 500, 900, 2000, 0};

static int DoubledPawns(Bitboard pawns) {
    int doubled = 0;
    while (pawns) {
        Square pawn = PopLSB(pawns);
        int file = pawn & 0x7;
        if (pawns & FILE_MASK[file]) {
            doubled++;
        }
    }
    return doubled;
}

static int BlockedPawns(Bitboard pawns, Bitboard occupancy, Color color) {
    constexpr int direction[2] = { 8, -8 };
    int blocked = 0;
    while (pawns) {
        Square pawn = PopLSB(pawns);
        Square oneForward = pawn + direction[color];
        if (occupancy & ToBitboard(oneForward)) {
            blocked++;
        }
    }
    return blocked;
}

static int IsolatedPawns(Bitboard pawns, Color color) {
    static constexpr Bitboard neighborFiles[8] = {
        FILE_MASK[1],
        FILE_MASK[0] | FILE_MASK[2],
        FILE_MASK[1] | FILE_MASK[3],
        FILE_MASK[2] | FILE_MASK[4],
        FILE_MASK[3] | FILE_MASK[5],
        FILE_MASK[4] | FILE_MASK[6],
        FILE_MASK[5] | FILE_MASK[7],
        FILE_MASK[6]
    };
    int isolated = 0;
    while (pawns) {
        Square pawn = PopLSB(pawns);
        int file = pawn & 0x7;
        if ((pawns & neighborFiles[file]) == 0) {
            isolated++;
        }
    }
    return isolated;
}

static int Evaluate(const Board& board, Color color) {
    Bitboard pawnBB = board.bitboards2D[color][PAWN_OFFSET];
    int pawnCount = std::popcount(pawnBB);
    int knightCount = std::popcount(board.bitboards2D[color][KNIGHT_OFFSET]);
    int bishopCount = std::popcount(board.bitboards2D[color][BISHOP_OFFSET]);
    int rookCount = std::popcount(board.bitboards2D[color][ROOK_OFFSET]);
    int queenCount = std::popcount(board.bitboards2D[color][QUEEN_OFFSET]);

    Color oppColor = ToggleColor(color);
    Bitboard oppPawnBB = board.bitboards2D[oppColor][PAWN_OFFSET];
    int oppPawnCount = std::popcount(oppPawnBB);
    int oppKnightCount = std::popcount(board.bitboards2D[oppColor][KNIGHT_OFFSET]);
    int oppBishopCount = std::popcount(board.bitboards2D[oppColor][BISHOP_OFFSET]);
    int oppRookCount = std::popcount(board.bitboards2D[oppColor][ROOK_OFFSET]);
    int oppQueenCount = std::popcount(board.bitboards2D[oppColor][QUEEN_OFFSET]);

    int materialScore = pieceValues[PAWN_OFFSET] * (pawnCount - oppPawnCount) +
                        pieceValues[BISHOP_OFFSET] * (bishopCount - oppBishopCount + knightCount - oppKnightCount) +
                        pieceValues[ROOK_OFFSET] * (rookCount - oppRookCount) +
                        pieceValues[QUEEN_OFFSET] * (queenCount - oppQueenCount);

    Bitboard occupancy = board.Occupancy();
    int doubled = DoubledPawns(pawnBB);
    int oppDoubled = DoubledPawns(oppDoubled);
    int blocked = BlockedPawns(pawnBB, occupancy, color);
    int oppBlocked = BlockedPawns(oppPawnBB, occupancy, oppColor);
    int isolated = IsolatedPawns(pawnBB, color);
    int oppIsolated = IsolatedPawns(oppPawnBB, oppColor);
    
    // TODO: could compute mobility for sliding pieces and knights by bitwise ANDing the attack board with inverse friendly occupancy
    // might be expensive
    int mobilityScore = -50 * (doubled - oppDoubled + blocked - oppBlocked + isolated - oppIsolated);
    
    return materialScore + mobilityScore;
}

static int Minimax(Board& board, int depth, Color colorToMove, Color engineColor, int alpha, int beta) {
    if (depth == 0) {
        return Evaluate(board, engineColor);
    }

    bool engineTurn = colorToMove == engineColor;
    std::vector<Move> moves = GenMoves(board, colorToMove);
    if (moves.empty()) { 
        if (InCheck(board, colorToMove)) { // checkmate
            return engineTurn ? INT_MIN : INT_MAX;
        }
        else { // stalemate
            return 0;
        }
    }
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return pieceValues[(int)a.capturedPieceType] > pieceValues[(int)b.capturedPieceType];        
    });
    int bestScore = engineTurn ? INT_MIN : INT_MAX;
    for (const Move& move : moves) {
        MakeMove(move, board, colorToMove);
        int score = Minimax(board, depth - 1, ToggleColor(colorToMove), engineColor, alpha, beta);
        UndoMove(move, board, colorToMove);
        if (engineTurn) { 
            bestScore = std::max(score, bestScore);
            alpha = std::max(alpha, bestScore);
            if (beta <= alpha) {
                break;
            }
        }
        else {
            bestScore = std::min(score, bestScore);
            beta = std::min(beta, bestScore);
            if (beta <= alpha) {
                break;
            }
        }
    }
    return bestScore;
}

Move Search(const Board& board, int maxDepth, Color colorToMove) {
    Color engineColor = colorToMove;
    std::vector<Move> moves = GenMoves(board, colorToMove);
    if (moves.empty()) {
        return Move{};
    }
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return pieceValues[(int)a.capturedPieceType] > pieceValues[(int)b.capturedPieceType];        
    });
    int bestScore = INT_MIN;
    int alpha = INT_MIN;
    int beta = INT_MAX;
    const Move* bestMove = &moves[0];
    Board boardCopy = board;
    for (const Move& move : moves) {
        MakeMove(move, boardCopy, colorToMove);
        int score = Minimax(boardCopy, maxDepth - 1, ToggleColor(engineColor), engineColor, alpha, beta);
        if (score > bestScore) {
            bestScore = score;
            bestMove = &move;
            alpha = std::max(alpha, bestScore);
        }
        UndoMove(move, boardCopy, colorToMove);
    }
    return *bestMove;
}

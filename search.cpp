#include "search.h"
#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <limits>
#include <vector>

static constexpr int pieceValues[7] = {1, 3, 3, 5, 9, 200, 0};

static int Evaluate(const Board& board, Color color) {
    int pawns = std::popcount(board.bitboards2D[color][PAWN_OFFSET]);
    int knights = std::popcount(board.bitboards2D[color][KNIGHT_OFFSET]);
    int bishops = std::popcount(board.bitboards2D[color][BISHOP_OFFSET]);
    int rooks = std::popcount(board.bitboards2D[color][ROOK_OFFSET]);
    int queens = std::popcount(board.bitboards2D[color][QUEEN_OFFSET]);

    Color oppColor = ToggleColor(color);
    int oppPawns = std::popcount(board.bitboards2D[oppColor][PAWN_OFFSET]);
    int oppKnights = std::popcount(board.bitboards2D[oppColor][KNIGHT_OFFSET]);
    int oppBishops = std::popcount(board.bitboards2D[oppColor][BISHOP_OFFSET]);
    int oppRooks = std::popcount(board.bitboards2D[oppColor][ROOK_OFFSET]);
    int oppQueens = std::popcount(board.bitboards2D[oppColor][QUEEN_OFFSET]);

    int materialScore = 1 * (pawns - oppPawns) +
                        3 * (bishops - oppBishops + knights - oppKnights) +
                        5 * (rooks - oppRooks) +
                        9 * (queens - oppQueens);
    return materialScore;
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

#include "search.h"
#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <bit>
#include <cassert>
#include <climits>
#include <limits>
#include <vector>

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

static int MinScoreMove(Board& board, int depth, Color colorToMove, Color engineColor, int& minScoreGuaranteed) {
    if (depth == 0) {
        return Evaluate(board, engineColor);
    }

    bool engineTurn = colorToMove == engineColor;
    std::vector<Move> moves = GenMoves(board, colorToMove);
    // TODO: handle checkmate or stalemate.
    // If no moves available and it's engine turn, it might be a checkmate. If it is, score the move
    // as negatively as possible. If it's a checkmate and it's opponent turn, score the move as highly
    // as possible. 
    // If it's a stalemate, things get a bit complicated because that may or may not be desired. If winning
    // significantly, stalemate should be avoided. Otherwise, it might be worth scoring stalemate positively.
    // That will probably require quite a bit of tweaking. Or could just score a stalemate as 0 and if there are 
    // better moves they will be scored higher.
    if (moves.empty()) { 
        if (InCheck(board, colorToMove)) { // checkmate
            return engineTurn ? INT_MIN : INT_MAX;
        }
        else { // stalemate
            return 0;
        }
    }
    int bestScore = engineTurn ? INT_MIN : INT_MAX;
    for (const Move& move : moves) {
        MakeMove(move, board, colorToMove);
        int score = MinScoreMove(board, depth - 1, ToggleColor(colorToMove), engineColor, minScoreGuaranteed);
        if (engineTurn) { // maximize engine score when it's our turn
            if (score > minScoreGuaranteed) {
                minScoreGuaranteed = score;
            }
            if (score > bestScore) {
                bestScore = score;
            }
        }
        else {
            if (score < minScoreGuaranteed) { // opponent can make a move that worsens our minimum score, stop considering this move
                return score;
            }
            if (score < bestScore) {
                bestScore = score;
            }
        }
        UndoMove(move, board, colorToMove);
    }
    return bestScore;
}

Move Search(const Board& board, int maxDepth, Color colorToMove) {
    Color engineColor = colorToMove;
    // TODO: also handle no moves available here
    std::vector<Move> moves = GenMoves(board, colorToMove);
    int bestScore = std::numeric_limits<int>::min();
    int alpha = INT_MIN;
    const Move* bestMove = nullptr;
    Board boardCopy = board;
    for (const Move& move : moves) {
        MakeMove(move, boardCopy, colorToMove);
        int score = MinScoreMove(boardCopy, maxDepth - 1, ToggleColor(engineColor), engineColor, alpha);
        if (score > bestScore) {
            bestScore = score;
            bestMove = &move;
        }
        UndoMove(move, boardCopy, colorToMove);
    }
    return *bestMove;
}

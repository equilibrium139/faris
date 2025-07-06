#include "search.h"
#include "board.h"
#include "movegen.h"
#include "utilities.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <cstring>
#include <utility>
#include <vector>

// All of these scores from POV of white. Square XOR 56 to get score from black POV

static constexpr std::array<int, 64> pawnScoreTable = {
      0,   0,   0,   0,   0,   0,   0,   0,
      5,  10,  10, -20, -20,  10,  10,   5,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,   5,  10,  25,  25,  10,   5,   5,
     10,  10,  20,  30,  30,  20,  10,  10,
     50,  50,  50,  50,  50,  50,  50,  50,
      0,   0,   0,   0,   0,   0,   0,   0
};

static constexpr std::array<int, 64> knightScoreTable = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

static constexpr std::array<int, 64> bishopScoreTable = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

static constexpr std::array<int, 64> rookScoreTable = {
      0,   0,   0,   5,   5,   0,   0,   0,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      5,  10,  10,  10,  10,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0
};

static constexpr std::array<int, 64> queenScoreTable = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -10,   5,   5,   5,   5,   5,   0, -10,
      0,   0,   5,   5,   5,   5,   0,  -5,
     -5,   0,   5,   5,   5,   5,   0,  -5,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

static constexpr std::array<int, 64> kingScoreTable = {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30
};

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

static int ComputePositionalScore(Bitboard bb, const std::array<int, 64>& scoreTable, bool black = false) {
    int score = 0;
    while (bb) {
        Square square = PopLSB(bb);
        square ^= black * 56;
        score += scoreTable[square];
    }
    return score;
}

static int Evaluate(const Board& board, Color color) {
    Bitboard pawnBB = board.bitboards2D[color][PAWN_OFFSET];
    Bitboard knightBB = board.bitboards2D[color][KNIGHT_OFFSET];
    Bitboard bishopBB = board.bitboards2D[color][BISHOP_OFFSET];
    Bitboard rookBB = board.bitboards2D[color][ROOK_OFFSET];
    Bitboard queenBB = board.bitboards2D[color][QUEEN_OFFSET];
    int pawnCount = std::popcount(pawnBB);
    int knightCount = std::popcount(knightBB);
    int bishopCount = std::popcount(bishopBB);
    int rookCount = std::popcount(rookBB);
    int queenCount = std::popcount(queenBB);

    Color oppColor = ToggleColor(color);
    Bitboard oppPawnBB = board.bitboards2D[oppColor][PAWN_OFFSET];
    Bitboard oppKnightBB = board.bitboards2D[oppColor][KNIGHT_OFFSET];
    Bitboard oppBishopBB = board.bitboards2D[oppColor][BISHOP_OFFSET];
    Bitboard oppRookBB = board.bitboards2D[oppColor][ROOK_OFFSET];
    Bitboard oppQueenBB = board.bitboards2D[oppColor][QUEEN_OFFSET];
    int oppPawnCount = std::popcount(oppPawnBB);
    int oppKnightCount = std::popcount(oppKnightBB);
    int oppBishopCount = std::popcount(oppBishopBB);
    int oppRookCount = std::popcount(oppRookBB);
    int oppQueenCount = std::popcount(oppQueenBB);

    int materialScore = pieceValues[PAWN_OFFSET] * (pawnCount - oppPawnCount) +
                        pieceValues[KNIGHT_OFFSET] * (knightCount - oppKnightCount) + 
                        pieceValues[BISHOP_OFFSET] * (bishopCount - oppBishopCount) +
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
    int pawnStructureScore = -50 * (doubled - oppDoubled + blocked - oppBlocked + isolated - oppIsolated);
    
    bool black = color == Black;
    int pawnPosScore = ComputePositionalScore(pawnBB, pawnScoreTable, black);
    int knightPosScore = ComputePositionalScore(knightBB, knightScoreTable, black);
    int bishopPosScore = ComputePositionalScore(bishopBB, bishopScoreTable, black);
    int rookPosScore = ComputePositionalScore(rookBB, rookScoreTable, black);
    int queenPosScore = ComputePositionalScore(queenBB, queenScoreTable, black);

    int oppPawnPosScore = ComputePositionalScore(oppPawnBB, pawnScoreTable, !black);
    int oppKnightPosScore = ComputePositionalScore(oppKnightBB, knightScoreTable, !black);
    int oppBishopPosScore = ComputePositionalScore(oppBishopBB, bishopScoreTable, !black);
    int oppRookPosScore = ComputePositionalScore(oppRookBB, rookScoreTable, !black);
    int oppQueenPosScore = ComputePositionalScore(oppQueenBB, queenScoreTable, !black);

    int positionalScore = pawnPosScore - oppPawnPosScore + knightPosScore - oppKnightPosScore + bishopPosScore - oppBishopPosScore + 
                          rookPosScore - oppRookPosScore + queenPosScore - oppQueenPosScore;

    return materialScore + pawnStructureScore + positionalScore;
}

Move killerMoves[64][2] = {};

static int ScoreMove(const Move& move, int depth) {
    int value = 0;
    if (move.capturedPieceType != PieceType::None) {
        return 10000 + pieceValues[(int)move.capturedPieceType] - pieceValues[(int)move.type];
    }
    if (move.promotionType != PieceType::None) {
        return 10000 + pieceValues[(int)move.promotionType];
    }
    if (move == killerMoves[depth][0]) {
        return 9000;
    }
    if (move == killerMoves[depth][1]) {
        return 8000;
    }
    return 0;
}


static bool MoveComparator(const Move& a, const Move& b, int depth) {
    int aScore = ScoreMove(a, depth);
    int bScore = ScoreMove(b, depth);
    return aScore > bScore;
}

static int Minimax(Board& board, int depth, Color colorToMove, Color engineColor, int alpha, int beta) {
    if (depth == 0) {
        return Evaluate(board, engineColor);
    }

    bool engineTurn = colorToMove == engineColor;
    std::vector<Move> moves = GenMoves(board, colorToMove);
    if (moves.empty()) { 
        if (InCheck(board, colorToMove)) { // checkmate
            return engineTurn ? -1000000 - depth : 1000000 + depth;
        }
        else { // stalemate
            return 0;
        }
    }
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){ return MoveComparator(a, b, depth); });
    int bestScore = engineTurn ? INT_MIN : INT_MAX;
    for (const Move& move : moves) {
        MakeMove(move, board, colorToMove);
        int score = Minimax(board, depth - 1, ToggleColor(colorToMove), engineColor, alpha, beta);
        UndoMove(move, board, colorToMove);
        if (engineTurn) { 
            bestScore = std::max(score, bestScore);
            alpha = std::max(alpha, bestScore);
            if (beta <= alpha) {
                if (move.capturedPieceType == PieceType::None) {
                    killerMoves[depth][1] = killerMoves[depth][0];
                    killerMoves[depth][0] = move;
                }
                break;
            }
        }
        else {
            bestScore = std::min(score, bestScore);
            beta = std::min(beta, bestScore);
            if (beta <= alpha) {
                if (move.capturedPieceType == PieceType::None) {
                    killerMoves[depth][1] = killerMoves[depth][0];
                    killerMoves[depth][0] = move;
                }
                break;
            }
        }
    }
    return bestScore;
}

Move Search(const Board& board, int maxDepth, Color colorToMove) {
    std::memset(killerMoves, 0, sizeof(killerMoves));
    Color engineColor = colorToMove;
    std::vector<Move> moves = GenMoves(board, colorToMove);
    if (moves.empty()) {
        return Move{};
    }
    Board boardCopy = board;
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){ return MoveComparator(a, b, maxDepth); });
    Move* bestMove = &moves[0];
    for (int depth = 1; depth <= maxDepth; depth++) {
        int bestScore = INT_MIN;
        int alpha = INT_MIN;
        int beta = INT_MAX;
        for (Move& move : moves) {
            MakeMove(move, boardCopy, colorToMove);
            int score = Minimax(boardCopy, depth - 1, ToggleColor(engineColor), engineColor, alpha, beta);
            if (score > bestScore) {
                bestScore = score;
                bestMove = &move;
                alpha = std::max(alpha, bestScore);
            }
            UndoMove(move, boardCopy, colorToMove);
        }
        std::swap(*bestMove, moves[0]); 
    }
    return *bestMove;
}

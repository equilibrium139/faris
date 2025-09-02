#include "search.h"
#include "attack_bitboards.h"
#include "board.h"
#include <chrono>
#include "magic.h"
#include "movegen.h"
#include "transposition.h"
#include "utilities.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <utility>
#include <vector>

// TODO: factor in 50 move draw rule

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

static int ComputeMobilityScore(Bitboard occupancy, Bitboard knights, Bitboard bishops, Bitboard rooks, Bitboard queens) {
    int mobility = 0;
    while (knights) {
        Square knight = PopLSB(knights);
        Bitboard moves = knightAttacks[knight];
        mobility += std::popcount(moves);
    }
    while (bishops) {
        Square bishop = PopLSB(bishops);
        Bitboard moves = BishopAttack(bishop, occupancy);
        mobility += std::popcount(moves);
    }
    while (rooks) {
        Square rook = PopLSB(rooks);
        Bitboard moves = RookAttack(rook, occupancy);
        mobility += std::popcount(moves);
    }
    while (queens) {
        Square queen = PopLSB(queens);
        Bitboard moves = QueenAttack(queen, occupancy);
        mobility += std::popcount(moves);
    }
    return mobility;
}

bool gUseNewFeature = false;

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
    int oppDoubled = DoubledPawns(oppPawnBB);
    int blocked = BlockedPawns(pawnBB, occupancy, color);
    int oppBlocked = BlockedPawns(oppPawnBB, occupancy, oppColor);
    int isolated = IsolatedPawns(pawnBB, color);
    int oppIsolated = IsolatedPawns(oppPawnBB, oppColor);
    
    // TODO: could compute mobility for sliding pieces and knights by bitwise ANDing the attack board with inverse friendly occupancy
    // might be expensive
    
    int mobilityScore = 0;
    if (gUseNewFeature) {
         mobilityScore = ComputeMobilityScore(occupancy, knightBB, bishopBB, rookBB, queenBB);
         int oppMobilityScore = ComputeMobilityScore(occupancy, oppKnightBB, oppBishopBB, oppRookBB, oppQueenBB);
         mobilityScore -= oppMobilityScore;
    }

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

    return materialScore + pawnStructureScore + positionalScore + gUseNewFeature * mobilityScore;
}

int historyTable[2][64][64] = {};
std::unordered_map<std::uint64_t, int> threefoldRepetitionTable;
constexpr int MAX_PLY = 64;
Move killerMoves[MAX_PLY][2] = {};
Move counterMoves[64][64] = {};
Move pvTable[MAX_PLY][MAX_PLY];
int pvLength[MAX_PLY];
std::vector<Move> principalVariation;
Move PVmove{};
bool isRootCall = false;
constexpr Move NULL_MOVE = Move{};
constexpr int INF_SCORE = 2'000'000;

static int ScoreMove(const Move& move, int depth, int ply, Color colorToMove, const Move& ttMove, bool followPV, Square prevMoveFrom, Square prevMoveTo) {
    if (move == ttMove) {
        return 100'000;
    }
    if (followPV && ply < principalVariation.size() && move == principalVariation[ply]) {
        return 50'000;
    }
    
    int captureAndPromotionScore = 0;
    if (move.capturedPieceType != PieceType::None) {
        captureAndPromotionScore += 10000 + pieceValues[(int)move.capturedPieceType] * 16 - pieceValues[(int)move.type];
    }
    if (move.promotionType != PieceType::None) {
        captureAndPromotionScore += 10000 + pieceValues[(int)move.promotionType];
    }
    if (captureAndPromotionScore > 0) return captureAndPromotionScore;
    
    // TODO: use ply for killerMoves
    if (move == killerMoves[ply][0]) {
        return 9000;
    }
    if (move == killerMoves[ply][1]) {
        return 8000;
    }
    if (gUseNewFeature && move == counterMoves[prevMoveFrom][prevMoveTo]) {
        return 7000;
    }
    return historyTable[colorToMove][move.from][move.to];
}

static bool MoveComparator(const Move& a, const Move& b, int depth, int ply, Color colorToMove, const Move& ttMove, bool followPV, Square prevMoveFrom, Square prevMoveTo) {
    int aScore = ScoreMove(a, depth, ply, colorToMove, ttMove, followPV, prevMoveFrom, prevMoveTo);
    int bScore = ScoreMove(b, depth, ply, colorToMove, ttMove, followPV, prevMoveFrom, prevMoveTo);
    return aScore > bScore;
}

static constexpr int NODE_INTERVAL_CHECK = 4096;
static int nodeCounter = NODE_INTERVAL_CHECK;
static constexpr int ABORT_SEARCH_VALUE = INF_SCORE * 2;

static std::uint64_t TimestampMS() {
    auto now = std::chrono::system_clock::now();
    auto duration_since_epoch = now.time_since_epoch();
    auto milliseconds_duration = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch);
    std::uint64_t timestamp_milliseconds = milliseconds_duration.count();
    return timestamp_milliseconds;
}

static int Quiesce(Board& board, int depth, Color colorToMove, Color engineColor, int alpha, int beta, std::uint64_t boardHash, std::uint64_t maxSearchTime) {
    --nodeCounter;
    if (nodeCounter <= 0) {
        nodeCounter = NODE_INTERVAL_CHECK;
        // TODO: check shared boolean variable 
        if (TimestampMS() >= maxSearchTime) {
            return ABORT_SEARCH_VALUE;
        }
    }
    bool engineTurn = colorToMove == engineColor;
    const int alphaOrig = alpha;
    const int betaOrig = beta;
    const TTEntry* entry = transpositionTable.Search(boardHash);
    if (entry &&  entry->depth == 0) {
        if (entry->scoreType == Exact) {
            return entry->score;
        }
        else if (entry->scoreType == LowerBound) {
            if (entry->score >= beta) {
                return entry->score;
            }
        }
        else if (entry->scoreType == UpperBound) {
            if (entry->score <= alpha) {
                return entry->score;
            }
        }
    }
    int bestScore = Evaluate(board, engineColor);
    if (engineTurn && bestScore > alpha) {
        alpha = bestScore;
    }
    if (!engineTurn && bestScore < beta) {
        beta = bestScore;
    }
    if (alpha >= beta) {
        transpositionTable.Add(board, colorToMove, 0, bestScore, engineTurn ? LowerBound : UpperBound, NULL_MOVE);
        return bestScore;
    }
    if (depth <= -8) { // stand-pat
        transpositionTable.Add(board, colorToMove, 0, bestScore, Exact, NULL_MOVE);
        return bestScore;
    }

    std::vector<Move> tacticalMoves = GenMoves(board, colorToMove, true);
    if (tacticalMoves.empty()) { 
        transpositionTable.Add(board, colorToMove, 0, bestScore, Exact, NULL_MOVE);
        return bestScore;
    }
    std::sort(tacticalMoves.begin(), tacticalMoves.end(), [](const Move& a, const Move& b) {
            int aScore = 0;
            if (a.capturedPieceType != PieceType::None) {
                aScore += pieceValues[(int)a.capturedPieceType] * 16 - pieceValues[(int)a.type];
            }
            if (a.promotionType != PieceType::None) {
                aScore += pieceValues[(int)a.promotionType] * 16;
            }
            int bScore = 0;
            if (b.capturedPieceType != PieceType::None) {
                bScore += pieceValues[(int)b.capturedPieceType] * 16 - pieceValues[(int)b.type];
            }
            if (b.promotionType != PieceType::None) {
                bScore += pieceValues[(int)b.promotionType] * 16;
            }
            return aScore > bScore;
    });
    ScoreType scoreType = Exact;
    Move bestMove = NULL_MOVE;
    for (const Move& move : tacticalMoves) {
        auto newBoardHash = boardHash;
        MakeMove(move, board, colorToMove, newBoardHash);
        int repetitionCount = ++threefoldRepetitionTable[newBoardHash];
        bool draw = repetitionCount >= 3;
        int score = draw ? 0 : Quiesce(board, depth - 1, ToggleColor(colorToMove), engineColor, alpha, beta, newBoardHash, maxSearchTime);
        UndoMove(move, board, colorToMove);
        --threefoldRepetitionTable[newBoardHash];
        if (score == ABORT_SEARCH_VALUE) {
            return ABORT_SEARCH_VALUE;
        }
        if (engineTurn) {
            if (score > bestScore) {
                bestMove = move;
                bestScore = score;
                if (bestScore > alpha) {
                    alpha = bestScore;
                }
            }
            if (alpha >= beta) {
                scoreType = LowerBound;
                break;
            }
        }
        else {
            if (score < bestScore) {
                bestMove = move;
                bestScore = score;
                if (bestScore < beta) {
                    beta = bestScore;
                }
            }
            if (alpha >= beta) {
                scoreType = UpperBound;
                break;
            }
        }
    }
    if (bestScore <= alphaOrig) {
        scoreType = UpperBound;
    }
    if (bestScore >= betaOrig) {
        scoreType = LowerBound;
    }
    transpositionTable.Add(board, colorToMove, 0, bestScore, scoreType, bestMove);
    return bestScore;
}

static int Minimax(Board& board, int depth, int ply, Color colorToMove, Color engineColor, int alpha, int beta, const std::uint64_t boardHash, std::uint64_t maxSearchTime, bool followPV, Square prevMoveFrom, Square prevMoveTo) {
    --nodeCounter;
    if (nodeCounter <= 0) {
        nodeCounter = NODE_INTERVAL_CHECK;
        // TODO: check shared boolean variable 
        if (TimestampMS() >= maxSearchTime) {
            return ABORT_SEARCH_VALUE;
        }
    }
    pvLength[ply] = 0;
    bool root = isRootCall;
    if (isRootCall) {
        isRootCall = false;
    }
    const int alphaOrig = alpha;
    const int betaOrig = beta;
    bool engineTurn = colorToMove == engineColor;
    const TTEntry* entry = transpositionTable.Search(boardHash);
    if (entry && entry->depth >= depth) {
        if (entry->scoreType == Exact) {
            return entry->score;
        }
        else if (entry->scoreType == LowerBound) {
            if (entry->score >= beta) {
                return entry->score;
            }
        }
        else if (entry->scoreType == UpperBound) {
            if (entry->score <= alpha) {
                return entry->score;
            }
        }
    }
    if (depth == 0) {
        pvLength[ply + 1] = 0;
        return Quiesce(board, 0, colorToMove, engineColor, alpha, beta, boardHash, maxSearchTime);
    }

    std::vector<Move> moves = GenMoves(board, colorToMove);
    bool inCheck = InCheck(board, colorToMove);
    if (moves.empty()) { 
        if (inCheck) { // checkmate
            return engineTurn ? -1'000'000 - depth : 1'000'000 + depth;
        }
        else { // stalemate
            return 0;
        }
    }


    const bool pvNode = (beta - alpha) > 1;
    bool enableNMP = !inCheck && depth > 3;
    if (enableNMP) {
        Bitboard nonKingNonPawnBB = (board.Occupancy(colorToMove) & ~board.Pawns(colorToMove) & ~board.Kings(colorToMove));
        bool pawnEndgame = nonKingNonPawnBB == 0;
        enableNMP = !pawnEndgame;
    }
    if (enableNMP) {
        int R = 2;
        if (depth > 6) R = 3;

        int a = alpha;
        int b = beta;
        if (engineTurn) a = b - 1;
        else b = a + 1;
        auto newBoardHash = boardHash ^ transpositionTable.blackToMoveZobrist;
        auto originalEP = board.enPassant;
        if (board.enPassant != -1) {
            newBoardHash ^= transpositionTable.enPassantFileZobrist[board.enPassant & 0x7];
            board.enPassant = -1;
        }
        int nullScore = Minimax(board, depth - R, ply + 1, ToggleColor(colorToMove), engineColor, a, b, newBoardHash, maxSearchTime, false, 0, 0);
        board.enPassant = originalEP; 
        if (nullScore == ABORT_SEARCH_VALUE) return ABORT_SEARCH_VALUE;
        if (engineTurn ? nullScore >= b : nullScore <= a){
            return nullScore;
        }
    }
    const Move ttMove = entry ? entry->bestMove : NULL_MOVE;
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){ return MoveComparator(a, b, depth, ply, colorToMove, ttMove, followPV, prevMoveFrom, prevMoveTo); });

    const bool enableLMR = !inCheck && moves.size() > 10 && depth > 3;

    int bestScore = engineTurn ? -INF_SCORE : INF_SCORE;
    ScoreType scoreType = Exact;
    const Move* bestMove = &moves[0];
    for (int i = 0; i < moves.size(); i++) {
        const Move& move = moves[i];
        auto newBoardHash = boardHash;
        MakeMove(move, board, colorToMove, newBoardHash);
        int repetitionCount = ++threefoldRepetitionTable[newBoardHash];
        bool draw = repetitionCount >= 3;
        bool childFollowPV = followPV && ply < principalVariation.size() && move == principalVariation[ply];

        bool fullWindow = i == 0;
        int score;
        if (draw) score = 0;
        else if (!fullWindow) {
            bool lmrPruneSuccess = false;
            int a = alpha;
            int b = beta;
            if (engineTurn) b = a + 1;
            else a = b - 1;
            if (gUseNewFeature && enableLMR && i > 3 && !childFollowPV) {
                bool tacticalMove = move.capturedPieceType != PieceType::None || move.promotionType != PieceType::None;
                if (!tacticalMove) {
                    int reduc = 2;
                    if (depth > 6) reduc++;
                    score = Minimax(board, depth-1-reduc, ply+1, ToggleColor(colorToMove), engineColor, a, b, newBoardHash, maxSearchTime, childFollowPV, prevMoveFrom, prevMoveTo);
                    if (score == ABORT_SEARCH_VALUE) goto after_recursion;
                    lmrPruneSuccess = (engineTurn ? score <= a : score >= b);
                }
            }
            if (!lmrPruneSuccess) {
                score = Minimax(board, depth - 1, ply + 1, ToggleColor(colorToMove), engineColor, a, b, newBoardHash, maxSearchTime, childFollowPV, prevMoveFrom, prevMoveTo);
                if (score == ABORT_SEARCH_VALUE) goto after_recursion;
                if (pvNode && (engineTurn ? score > a : score < b)) {
                    score = Minimax(board, depth - 1, ply + 1, ToggleColor(colorToMove), engineColor, alpha, beta, newBoardHash, maxSearchTime, childFollowPV, prevMoveFrom, prevMoveTo);
                }
            }
        }
        else score = Minimax(board, depth - 1, ply + 1, ToggleColor(colorToMove), engineColor, alpha, beta, newBoardHash, maxSearchTime, childFollowPV, prevMoveFrom, prevMoveTo);

        if (score > alpha && score < beta) {
            pvTable[ply][0] = move;
            int n = pvLength[ply + 1];
            std::copy(pvTable[ply + 1], pvTable[ply + 1] + n, pvTable[ply] + 1);
            pvLength[ply] = 1 + n;
        }

after_recursion:
        UndoMove(move, board, colorToMove);
        --threefoldRepetitionTable[newBoardHash];
        if (score == ABORT_SEARCH_VALUE) {
            // TODO: find a better way to handle PV when a timeout occurs
            if (root) {
                if (PVmove == NULL_MOVE) {
                    PVmove = *bestMove;
                }
            }
            return ABORT_SEARCH_VALUE;
        }
        if (engineTurn) { 
            if (score > bestScore) {
                bestScore = score;
                bestMove = &move;
            }
            alpha = std::max(alpha, bestScore);
            if (beta <= alpha) {
                historyTable[colorToMove][move.from][move.to] += depth * depth;
                if (move.capturedPieceType == PieceType::None) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;
                    if (gUseNewFeature) counterMoves[prevMoveFrom][prevMoveTo] = move;
                }
                scoreType = LowerBound;
                break;
            }
        }
        else {
            if (score < bestScore) {
                bestScore = score;
                bestMove = &move;
            }
            beta = std::min(beta, bestScore);
            if (beta <= alpha) {
                historyTable[colorToMove][move.from][move.to] += depth * depth;
                if (move.capturedPieceType == PieceType::None) {
                    killerMoves[ply][1] = killerMoves[ply][0];
                    killerMoves[ply][0] = move;
                    if (gUseNewFeature) counterMoves[prevMoveFrom][prevMoveTo] = move;
                }
                scoreType = UpperBound;
                break;
            }
        }
    }
    if (bestScore <= alphaOrig) {
        scoreType = UpperBound;
    }
    if (bestScore >= betaOrig) {
        scoreType = LowerBound;
    }
    transpositionTable.Add(board, colorToMove, depth, bestScore, scoreType, *bestMove);
    return bestScore;
}

Move Search(const Board& board, Color colorToMove, int totalTimeRemaining, int inc, bool useNewFeature) {
    gUseNewFeature = useNewFeature;
    std::uint64_t startTime = TimestampMS();
    std::memset(killerMoves, 0, sizeof(killerMoves));
    std::memset(counterMoves, 0, sizeof(counterMoves));
    std::memset(historyTable, 0, sizeof(historyTable)); 
    std::memset(pvLength, 0, sizeof(pvLength));
    std::fill(&pvTable[0][0], &pvTable[0][0] + MAX_PLY * MAX_PLY, NULL_MOVE);
    principalVariation.clear();
    PVmove = {};
    int searchTime = totalTimeRemaining / 20 + inc / 2;
    if (searchTime > totalTimeRemaining) {
        searchTime = totalTimeRemaining - 500;
    }
    if (searchTime <= 0) {
        searchTime = totalTimeRemaining;
    }
    auto maxSearchTime = startTime + searchTime;
    Color engineColor = colorToMove;
    const auto boardHash = transpositionTable.Hash(board, colorToMove);
    int score = 0;
    
    for (int depth = 1; ;depth++) {
        int alpha = -INF_SCORE;
        int beta = INF_SCORE;
        int delta = 50;
        if (depth > 1) {
            alpha = score - delta;
            beta = score + delta;
            while (true) {
                Board boardCopy = board;
                isRootCall = true;
                score = Minimax(boardCopy, depth, 0, colorToMove, engineColor, alpha, beta, boardHash, maxSearchTime, true, 0, 0);
                if (score == ABORT_SEARCH_VALUE) break;
                if (score <= alpha) { alpha -= delta; delta *= 2; continue; }
                if (score >= beta) { beta += delta; delta *= 2; continue; }
                break;
            }
        }
        else {
            Board boardCopy = board;
            isRootCall = true;
            score = Minimax(boardCopy, depth, 0, colorToMove, engineColor, alpha, beta, boardHash, maxSearchTime, true, 0, 0);
        }
        const TTEntry* entry = transpositionTable.Search(boardHash);
        if (entry) {
            PVmove = entry->bestMove;
        }
        if (score == ABORT_SEARCH_VALUE || TimestampMS() >= maxSearchTime) {
            break;
        }
        else {
            principalVariation.clear();
            std::copy(pvTable[0], pvTable[0] + pvLength[0], std::back_inserter(principalVariation));
        }
    }
    if (principalVariation.empty () && pvLength[0] == 0) {
        return PVmove;
    }
    return principalVariation.empty() ? pvTable[0][0] : principalVariation[0];
}

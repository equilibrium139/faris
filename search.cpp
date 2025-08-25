#include "search.h"
#include "board.h"
#include <chrono>
#include "movegen.h"
#include "transposition.h"
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
    int oppDoubled = DoubledPawns(oppPawnBB);
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
int historyTable[2][64][64] = {};
std::unordered_map<std::uint64_t, int> threefoldRepetitionTable;
Move PVmove{};
bool gUseNewFeature = false;
bool isRootCall = false;

static int ScoreMove(const Move& move, int depth, Color colorToMove, const Move& ttMove) {
    if (move == PVmove) {
        return 15000;
    }
    if (move == ttMove) {
        return 14000;
    }
    if (move.capturedPieceType != PieceType::None) {
        return 10000 + pieceValues[(int)move.capturedPieceType] - pieceValues[(int)move.type]/10;
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
    return historyTable[colorToMove][move.from][move.to];
}


static bool MoveComparator(const Move& a, const Move& b, int depth, Color colorToMove, const Move& ttMove) {
    int aScore = ScoreMove(a, depth, colorToMove, ttMove);
    int bScore = ScoreMove(b, depth, colorToMove, ttMove);
    return aScore > bScore;
}

static constexpr int NODE_INTERVAL_CHECK = 4096;
static int nodeCounter = NODE_INTERVAL_CHECK;
static constexpr int ABORT_SEARCH_VALUE = INT_MAX;

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
        transpositionTable.Add(board, colorToMove, 0, bestScore, engineTurn ? LowerBound : UpperBound, Move{});
        return bestScore;
    }
    if (depth <= -8) { // stand-pat
        transpositionTable.Add(board, colorToMove, 0, bestScore, Exact, Move{});
        return bestScore;
    }

    std::vector<Move> moves = GenMoves(board, colorToMove);
    // TODO: GenCaptures func
    std::vector<Move> captures;
    for (const Move& m : moves) {
        if (m.capturedPieceType != PieceType::None) captures.push_back(m);
    }
    if (captures.empty()) { 
        transpositionTable.Add(board, colorToMove, 0, bestScore, Exact, Move{});
        return bestScore;
    }
    std::sort(captures.begin(), captures.end(), [](const Move& a, const Move& b) {
            int aScore = pieceValues[(int)a.capturedPieceType] * 16 - pieceValues[(int)a.type];
            int bScore = pieceValues[(int)b.capturedPieceType] * 16 - pieceValues[(int)b.type];
            return aScore > bScore;
    });
    ScoreType scoreType = Exact;
    Move bestMove = Move{};
    for (const Move& move : captures) {
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
    transpositionTable.Add(board, colorToMove, 0, bestScore, scoreType, bestMove);
    return bestScore;
}

static int Minimax(Board& board, int depth, Color colorToMove, Color engineColor, int alpha, int beta, const std::uint64_t boardHash, std::uint64_t maxSearchTime) {
    bool root = isRootCall;
    if (isRootCall) {
        isRootCall = false;
    }
    --nodeCounter;
    if (nodeCounter <= 0) {
        nodeCounter = NODE_INTERVAL_CHECK;
        // TODO: check shared boolean variable 
        auto currentTime = TimestampMS();
        if (currentTime >= maxSearchTime) {
            return ABORT_SEARCH_VALUE;
        }
    }
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
        if (gUseNewFeature) return Quiesce(board, 0, colorToMove, engineColor, alpha, beta, boardHash, maxSearchTime);
        else return Evaluate(board, engineColor);
    }

    std::vector<Move> moves = GenMoves(board, colorToMove);
    if (moves.empty()) { 
        if (InCheck(board, colorToMove)) { // checkmate
            return engineTurn ? -1000000 - depth : 1000000 + depth;
        }
        else { // stalemate
            return 0;
        }
    }
    const Move ttMove = entry && gUseNewFeature ? entry->bestMove : Move{};
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b){ return MoveComparator(a, b, depth, colorToMove, ttMove); });
    int bestScore = engineTurn ? INT_MIN : INT_MAX;
    ScoreType scoreType = Exact;
    const Move* bestMove = &moves[0];
    for (const Move& move : moves) {
        auto newBoardHash = boardHash;
        MakeMove(move, board, colorToMove, newBoardHash);
        int repetitionCount = ++threefoldRepetitionTable[newBoardHash];
        bool draw = repetitionCount >= 3;
        int score = draw ? 0 : Minimax(board, depth - 1, ToggleColor(colorToMove), engineColor, alpha, beta, newBoardHash, maxSearchTime);
        UndoMove(move, board, colorToMove);
        --threefoldRepetitionTable[newBoardHash];
        if (score == ABORT_SEARCH_VALUE) {
            // TODO: find a better way to handle PV when a timeout occurs
            if (root) {
                if (PVmove == Move{}) {
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
                    killerMoves[depth][1] = killerMoves[depth][0];
                    killerMoves[depth][0] = move;
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
                    killerMoves[depth][1] = killerMoves[depth][0];
                    killerMoves[depth][0] = move;
                }
                scoreType = UpperBound;
                break;
            }
        }
    }
    transpositionTable.Add(board, colorToMove, depth, bestScore, scoreType, *bestMove);
    return bestScore;
}

// TODO: handle 3-fold repetition draw
Move Search(const Board& board, Color colorToMove, int totalTimeRemaining, int inc, bool useNewFeature) {
    gUseNewFeature = useNewFeature;
    std::uint64_t startTime = TimestampMS();
    std::memset(killerMoves, 0, sizeof(killerMoves));
    std::memset(historyTable, 0, sizeof(historyTable)); 
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
    int alpha = INT_MIN;
    int beta = INT_MAX;
    int score = 0;
    for (int depth = 1; ;depth++) {
        Board boardCopy = board;
        isRootCall = true;
        score = Minimax(boardCopy, depth, colorToMove, engineColor, alpha, beta, boardHash, maxSearchTime);
        if (score == ABORT_SEARCH_VALUE || TimestampMS() >= maxSearchTime) {
            break;
        }
        const TTEntry* entry = transpositionTable.Search(boardHash);
        if (entry) {
            PVmove = entry->bestMove;
        }
    }
    return PVmove; 
}

#include "board.h"
#include "movegen.h"
#include <array>
#include <cstdint>

enum ScoreType {
    Exact,
    LowerBound,
    UpperBound
};

struct TTEntry {
    std::uint64_t hash;
    Move bestMove;
    int score;
    ScoreType scoreType;
    int depth;
};

struct TT {
    static constexpr int size = 10'000'000;

    std::uint64_t hits = 0;
    std::vector<TTEntry> table{size};
    std::uint64_t pieceZobrist[64][6][2];
    std::uint64_t blackToMoveZobrist;
    std::array<std::uint64_t, 16> castlingRightsZobrist;
    std::array<std::uint64_t, 8> enPassantFileZobrist;
    
    std::uint64_t Hash(const Board& board, Color colorToMove);
    const TTEntry* Search(const Board& board, Color colorToMove);
    const TTEntry* Search(std::uint64_t hash);
    void Add(const Board& board, Color colorToMove, int depth, int score, ScoreType scoreType, const Move& bestMove);
    TT();
};

extern TT transpositionTable;

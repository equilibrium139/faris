#include "perft.h"
#include "board.h"
#include <iostream>
#include "movegen.h"
#include "utilities.h"

static void printMoveWithCount(const Move& move, std::uint64_t count) {
    int fromFile = move.from % 8;
    int fromRank = move.from / 8;
    int toFile = move.to % 8;
    int toRank = move.to / 8;
    char fromFileChar = 'a' + fromFile;
    char toFileChar = 'a' + toFile;
    std::cout << fromFileChar << fromRank + 1 << toFileChar << toRank + 1 << ": " << count << '\n';
}

std::uint64_t perftest(Board& board, int depth, Color colorToMove, bool enablePerftDiagnostics) {
    std::vector<Move> moves = GenMoves(board, colorToMove);
    if (depth == 1) {
        return moves.size();
    }
    std::uint64_t nodeCount = 0;
    Color nextColorToMove = ToggleColor(colorToMove);
    Board oldBoard = board;
    for (const Move& move : moves) {
        MakeMove(move, board, colorToMove);
        std::uint64_t moveNodeCount = perftest(board, depth - 1, nextColorToMove, enablePerftDiagnostics);
        if (depth == maxDepth && enablePerftDiagnostics) {
            printMoveWithCount(move, moveNodeCount);
        }
        nodeCount += moveNodeCount;
        UndoMove(move, board, colorToMove);
        assert(board == oldBoard);
    }
    return nodeCount;
}

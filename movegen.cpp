#include "movegen.h"
#include <cassert>
#include <cmath>
#include <span>

// TODO: take into account castling, en passant, promotion, check, checkmate,
// stalemate, draw.

int perftest(const Board& board, int depth) {
    int countLeafNodes = 0;

    return countLeafNodes;
}

void removePiece(int squareIndex, std::span<Bitboard, 6> pieces) {
    assert(squareIndex >= 0 && squareIndex < 64);
    Bitboard captureSquareBB = (Bitboard)1 << squareIndex;
    for (Bitboard& pieceBoard : pieces) {
        if (pieceBoard & captureSquareBB) {
            pieceBoard &= ~captureSquareBB;
            return;
        }
    }
}

std::vector<Board> genMoves(const Board& board, bool whiteTurn) {
    std::vector<Board> moves;
    const Bitboard occupancy = board.allPieces();
    const Bitboard enemyOccupancy = whiteTurn ? board.blackPieces() : board.whitePieces();
    const Bitboard friendlyOccupancy = whiteTurn ? board.whitePieces() : board.blackPieces();
    std::span<const Bitboard, 6> enemyPieces = whiteTurn ? std::span<const Bitboard, 6>(&board.pieces[BLACK_PIECE_OFFSET], 6) : 
                                                           std::span<const Bitboard, 6>{&board.pieces[WHITE_PIECE_OFFSET], 6 }; 
    std::span<const Bitboard, 6> friendlyPieces = whiteTurn ? std::span<const Bitboard, 6>(&board.pieces[WHITE_PIECE_OFFSET], 6) :
                                                               std::span<const Bitboard, 6>{&board.pieces[BLACK_PIECE_OFFSET], 6 };
    const int friendlyPieceOffset = whiteTurn ? WHITE_PIECE_OFFSET : BLACK_PIECE_OFFSET;
    const int enemyPieceOffset = whiteTurn ? BLACK_PIECE_OFFSET : WHITE_PIECE_OFFSET;
    const Bitboard pawns = friendlyPieces[PAWN_OFFSET];
    const int pawnDirection = whiteTurn ? 8 : -8;
    const int pawnStartRank = whiteTurn ? 1 : 6;
    const int leftCaptureOffset = whiteTurn ? 7 : -9;  
    const int rightCaptureOffset = whiteTurn ? 9 : -7; 
    
    for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
        Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
        if (squareIndexBB & pawns)
        {
            int rank = squareIndex / 8;
            int file = squareIndex % 8;

            int oneSquareForwardIndex = squareIndex + pawnDirection;
            if (oneSquareForwardIndex >= 0 && oneSquareForwardIndex < 64) { 
                Bitboard oneSquareForwardBB = (Bitboard)1 << oneSquareForwardIndex;
                if ((occupancy & oneSquareForwardBB) == 0) {
                    Board newBoard = board;
                    newBoard.pieces[friendlyPieceOffset] &= ~squareIndexBB;
                    newBoard.pieces[friendlyPieceOffset] |= oneSquareForwardBB;
                    // TODO: Handle promotion
                    moves.push_back(newBoard);

                    // Double Push only if single push is possible
                    if (rank == pawnStartRank)
                    {
                        int twoSquaresForwardIndex = oneSquareForwardIndex + pawnDirection;
                        if (twoSquaresForwardIndex >= 0 && twoSquaresForwardIndex < 64) { 
                            Bitboard twoSquaresForwardBB = (Bitboard)1 << twoSquaresForwardIndex;
                            if ((occupancy & twoSquaresForwardBB) == 0)
                            {
                                Board doublePushBoard = board;
                                doublePushBoard.pieces[friendlyPieceOffset] &= ~squareIndexBB;
                                doublePushBoard.pieces[friendlyPieceOffset] |= twoSquaresForwardBB;
                                // TODO: Set en passant target square
                                moves.push_back(doublePushBoard);
                            }
                        }
                    }
                }
            }
            // Left Capture
            if (file > 0) { 
                int leftDiagIndex = squareIndex + leftCaptureOffset;
                if (leftDiagIndex >= 0 && leftDiagIndex < 64) { 
                    Bitboard leftDiagBB = (Bitboard)1 << leftDiagIndex;
                    if (enemyOccupancy & leftDiagBB) {
                        Board newBoard = board;
                        newBoard.pieces[friendlyPieceOffset] &= ~squareIndexBB;
                        newBoard.pieces[friendlyPieceOffset] |= leftDiagBB;
                        std::span<Bitboard, 6> newEnemyPieces = std::span<Bitboard, 6>(&newBoard.pieces[enemyPieceOffset], 6);
                        removePiece(leftDiagIndex, newEnemyPieces);
                        // TODO: Handle promotion
                        moves.push_back(newBoard);
                    }
                    // TODO: Handle en passant capture left
                }
            }
            // Right Capture
            if (file < 7) {
                int rightDiagIndex = squareIndex + rightCaptureOffset;
                if (rightDiagIndex >= 0 && rightDiagIndex < 64) { 
                    Bitboard rightDiagBB = (Bitboard)1 << rightDiagIndex;
                    if (enemyOccupancy & rightDiagBB) {
                        Board newBoard = board;
                        newBoard.pieces[friendlyPieceOffset] &= ~squareIndexBB;
                        newBoard.pieces[friendlyPieceOffset] |= rightDiagBB;
                        std::span<Bitboard, 6> newEnemyPieces = std::span<Bitboard, 6>(&newBoard.pieces[enemyPieceOffset], 6);
                        removePiece(rightDiagIndex, newEnemyPieces);
                        // TODO: Handle promotion
                        moves.push_back(newBoard);
                    }
                    // TODO: Handle en passant capture right
                }
            }
        }
    }

    const Bitboard knights = friendlyPieces[KNIGHT_OFFSET];
    constexpr int knightMoves[8] = {6, 10, 15, 17, -6, -10, -15, -17};
    for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
        Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
        if (squareIndexBB & knights) {
            int file = squareIndex % 8;
            int rank = squareIndex / 8;
            for (int moveOffset : knightMoves) {
                int newSquareIndex = squareIndex + moveOffset;
                if (newSquareIndex < 0 || newSquareIndex >= 64)
                    continue;

                int newFile = newSquareIndex % 8;
                int newRank = newSquareIndex / 8;

                bool validMove = (std::abs(newFile - file) == 1 && std::abs(newRank - rank) == 2) ||
                                 (std::abs(newFile - file) == 2 && std::abs(newRank - rank) == 1);
                if (!validMove) { continue; }
                Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                if ((friendlyOccupancy & newSquareBB) == 0) { // Can't move to a square occupied by a friendly piece
                    Board newBoard = board;
                    newBoard.pieces[friendlyPieceOffset + KNIGHT_OFFSET] &= ~squareIndexBB; // Remove knight from original square
                    newBoard.pieces[friendlyPieceOffset + KNIGHT_OFFSET] |= newSquareBB;    // Place knight on new square
                    
                    std::span<Bitboard, 6> newEnemyPieces(newBoard.pieces + enemyPieceOffset, 6);
                    if (enemyOccupancy & newSquareBB) { 
                        removePiece(newSquareIndex, newEnemyPieces);
                    }
                    moves.push_back(newBoard);
                }
            }
        }
    }

    struct SlidingPiece {
        int pieceIndex;
        const int* moves;
        int numMoves;
    };

    int bishopMoves[4] = {7, 9, -7, -9};
    int rookMoves[4] = {8, -8, 1, -1};
    int queenMoves[8] = {7, 9, -7, -9, 8, -8, 1, -1}; // Diagonal first

    SlidingPiece slidingPieces[] = {
        {BISHOP_OFFSET, bishopMoves, 4},
        {ROOK_OFFSET, rookMoves, 4},
        {QUEEN_OFFSET, queenMoves, 8}
    };

    for (const SlidingPiece& pieceInfo : slidingPieces) {
        const Bitboard pieceBoard = friendlyPieces[pieceInfo.pieceIndex];
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & pieceBoard) {
                for (int i = 0; i < pieceInfo.numMoves; i++) {
                    int moveOffset = pieceInfo.moves[i];
                    int newSquareIndex = squareIndex;
                    int prevFile = squareIndex % 8;
                    int prevRank = squareIndex / 8;
                    bool diagonalMove = pieceInfo.pieceIndex == BISHOP_OFFSET || (pieceInfo.pieceIndex == QUEEN_OFFSET && i < 4);
                    while (true) {
                        newSquareIndex += moveOffset;
                        if (newSquareIndex < 0 || newSquareIndex >= 64) break;

                        int newFile = newSquareIndex % 8;
                        int newRank = newSquareIndex / 8;

                        bool validStep;
                        if (diagonalMove) {
                            validStep = std::abs(newFile - prevFile) == 1 && std::abs(newRank - prevRank) == 1;
                        } else {
                            validStep = std::abs(newFile - prevFile) == 0 || std::abs(newRank - prevRank) == 0;
                        }

                        if (!validStep) break;
                        prevFile = newFile;
                        prevRank = newRank;
                        Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                        
                        if (friendlyOccupancy & newSquareBB) break; // Blocked by friendly piece

                        Board newBoard = board;
                        newBoard.pieces[friendlyPieceOffset + pieceInfo.pieceIndex] &= ~squareIndexBB; 
                        newBoard.pieces[friendlyPieceOffset + pieceInfo.pieceIndex] |= newSquareBB;  
                        moves.push_back(newBoard);
                        if (enemyOccupancy & newSquareBB) {
                            std::span<Bitboard, 6> newEnemyPieces(newBoard.pieces + enemyPieceOffset, 6);
                            removePiece(newSquareIndex, newEnemyPieces);
                            moves.push_back(newBoard);
                            break; // Stop sliding after capturing an enemy piece
                        }
                    }
                }
            }
        }
    }

    const Bitboard king = friendlyPieces[KING_OFFSET];
    int kingMoves[8] = {7, 9, -7, -9, 8, -8, 1, -1};
    for (int squareIndex = 0; squareIndex < 64; squareIndex++)
    {
        Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
        if (squareIndexBB & king)
        {
            int file = squareIndex % 8;
            int rank = squareIndex / 8;
            for (int moveOffset : kingMoves)
            {
                int newSquareIndex = squareIndex + moveOffset;
                if (newSquareIndex < 0 || newSquareIndex >= 64)
                    continue; // Off board

                int newFile = newSquareIndex % 8;
                int newRank = newSquareIndex / 8;

                // Check if move wraps around the board incorrectly (distance > 1 in file or rank)
                if (std::abs(newFile - file) > 1 || std::abs(newRank - rank) > 1)
                    continue;

                Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                if ((friendlyOccupancy & newSquareBB) == 0)
                { // Can't move to a square occupied by a friendly piece
                    Board newBoard = board;
                    newBoard.pieces[friendlyPieceOffset + KING_OFFSET] &= ~squareIndexBB; // Remove king from original square
                    newBoard.pieces[friendlyPieceOffset + KING_OFFSET] |= newSquareBB;    // Place king on new square

                    if (enemyOccupancy & newSquareBB) {
                        std::span<Bitboard, 6> newEnemyPieces(newBoard.pieces + enemyPieceOffset, 6);
                        removePiece(newSquareIndex, newEnemyPieces);
                    }
                    // TODO: Add check generation logic here - king cannot move into check
                    moves.push_back(newBoard);
                }
            }
            // TODO: Add castling logic
            break; // Only one king per side
        }
    }
    return moves;
}

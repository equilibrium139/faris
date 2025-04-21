#include "movegen.h"

void perftest(const Board& board, int& countLeafNodes, int depth) {
    if (depth == 0) {
        countLeafNodes++;
        return;
    }

}

std::vector<Board> genMoves(const Board& board, bool whiteTurn) {
    std::vector<Board> moves;
    // assuming whiteTurn is true for now
    if (whiteTurn) {
        const Bitboard& pawns = board.whitePawns;
        const Bitboard& occupancy = board.allPieces();
        int squareIndex = 0;
        while (squareIndex < 64) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & pawns) { // we have a pawn at squareIndex 
                int oneSquareForwardIndex = squareIndex + 8;
                Bitboard oneSquareForwardBB = (Bitboard)1 << oneSquareForwardIndex;
                if ((occupancy & oneSquareForwardBB) == 0) { // pawn can move one square forward
                    Board newBoard = board;
                    newBoard.whitePawns &= ~squareIndexBB;  // zero out original square
                    newBoard.whitePawns |= oneSquareForwardIndex; // 1 where destination square is
                    moves.push_back(newBoard);
                }
                int twoSquaresForwardIndex = oneSquareForwardIndex + 8;
                Bitboard twoSquaresForwardBB = (Bitboard)1 << twoSquaresForwardIndex;
                if ((occupancy & twoSquaresForwardBB) == 0) {
                    Board newBoard = board;
                    newBoard.whitePawns &= ~squareIndexBB;  // zero out original square
                    newBoard.whitePawns |= twoSquaresForwardBB; // 1 where destination square is
                    moves.push_back(newBoard);
                } 
                int file = squareIndex % 8;
                const Bitboard& enemyOccupancy = board.blackPieces();
                if (file > 0) { // left diagonal exists
                    int leftDiagIndex = oneSquareForwardIndex - 1;
                    Bitboard leftDiagBB = (Bitboard)1 << leftDiagIndex;
                    if (enemyOccupancy & leftDiagBB) {
                        Board newBoard = board;
                        newBoard.whitePawns &= ~squareIndexBB;
                        newBoard.whitePawns |= leftDiagBB;
                        // figure out which enemy piece is at destination square
                        if (board.blackPawns & leftDiagBB) {
                            newBoard.blackPawns &= ~leftDiagBB;
                        } else if (board.blackKnights & leftDiagBB) {
                            newBoard.blackKnights &= ~leftDiagBB;
                        } else if (board.blackBishops & leftDiagBB) {
                            newBoard.blackBishops &= ~leftDiagBB;
                        } else if (board.blackRooks & leftDiagBB) {
                            newBoard.blackRooks &= ~leftDiagBB;
                        } else if (board.blackQueen & leftDiagBB) {
                            newBoard.blackQueen &= ~leftDiagBB;
                        } else if (board.blackKing & leftDiagBB) {
                            newBoard.blackKing &= ~leftDiagBB;
                        }
                        moves.push_back(newBoard);
                    }
                }
                if (file < 7) { // right diagonal exists
                    int rightDiagIndex = oneSquareForwardIndex + 1;
                    Bitboard rightDiagBB = (Bitboard)1 << rightDiagIndex;
                    if (enemyOccupancy & rightDiagBB) {
                        Board newBoard = board;
                        newBoard.whitePawns &= ~squareIndexBB;
                        newBoard.whitePawns |= rightDiagBB;
                        // figure out which enemy piece is at destination square
                        if (board.blackPawns & rightDiagBB) {
                            newBoard.blackPawns &= ~rightDiagBB;
                        } else if (board.blackKnights & rightDiagBB) {
                            newBoard.blackKnights &= ~rightDiagBB;
                        } else if (board.blackBishops & rightDiagBB) {
                            newBoard.blackBishops &= ~rightDiagBB;
                        } else if (board.blackRooks & rightDiagBB) {
                            newBoard.blackRooks &= ~rightDiagBB;
                        } else if (board.blackQueen & rightDiagBB) {
                            newBoard.blackQueen &= ~rightDiagBB;
                        } else if (board.blackKing & rightDiagBB) {
                            newBoard.blackKing &= ~rightDiagBB;
                        }
                        moves.push_back(newBoard);
                    }
                }

            }
            squareIndex++;
        }        
    }    
    return moves;
}

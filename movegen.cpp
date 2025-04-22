#include "movegen.h"
#include <cmath>

// TODO: take into account castling, en passant, promotion, check, checkmate, stalemate, draw.

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
        const Bitboard pawns = board.whitePawns;
        const Bitboard occupancy = board.allPieces();
        const Bitboard enemyOccupancy = board.blackPieces();
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
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
        }        
        const Bitboard knights = board.whiteKnights;
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & knights) { 
                int file = squareIndex % 8;
                int rank = squareIndex / 8;
                int knightMoves[8] = { 6, 10, 15, 17, -6, -10, -15, -17 };
                for (int i = 0; i < 8; i++) {
                    int newSquareIndex = squareIndex + knightMoves[i];
                    int newFile = newSquareIndex % 8;
                    int newRank = newSquareIndex / 8;
                    bool validMove = std::abs(newFile - file) == 1 && std::abs(newRank - rank) == 2 ||
                                     std::abs(newFile - file) == 2 && std::abs(newRank - rank) == 1;
                    validMove &= newSquareIndex < 64 && newSquareIndex >= 0;
                    if (validMove) {
                        Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                        if ((occupancy & newSquareBB) == 0) { 
                            Board newBoard = board;
                            newBoard.whiteKnights &= ~squareIndexBB; 
                            newBoard.whiteKnights |= newSquareBB; 
                            moves.push_back(newBoard);
                        } else if (enemyOccupancy & newSquareBB) { 
                            Board newBoard = board;
                            newBoard.whiteKnights &= ~squareIndexBB; 
                            newBoard.whiteKnights |= newSquareBB; 
                            if (board.blackPawns & newSquareBB) {
                                newBoard.blackPawns &= ~newSquareBB;
                            } else if (board.blackKnights & newSquareBB) {
                                newBoard.blackKnights &= ~newSquareBB;
                            } else if (board.blackBishops & newSquareBB) {
                                newBoard.blackBishops &= ~newSquareBB;
                            } else if (board.blackRooks & newSquareBB) {
                                newBoard.blackRooks &= ~newSquareBB;
                            } else if (board.blackQueen & newSquareBB) {
                                newBoard.blackQueen &= ~newSquareBB;
                            } else if (board.blackKing & newSquareBB) {
                                newBoard.blackKing &= ~newSquareBB;
                            }
                            moves.push_back(newBoard);
                        }
                    }
                }
            }
        }
        const Bitboard bishops = board.whiteBishops;
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & bishops) { 
                int bishopMoves[4] = { 7, 9, -7, -9 };
                for (int i = 0; i < 4; i++) {
                    int newSquareIndex = squareIndex;
                    int prevFile = squareIndex % 8;
                    int prevRank = squareIndex / 8;
                    while (true) {
                        newSquareIndex += bishopMoves[i];
                        int newFile = newSquareIndex % 8;
                        int newRank = newSquareIndex / 8;
                        bool validMove = std::abs(newFile - prevFile) == std::abs(newRank - prevRank);
                        validMove &= newSquareIndex < 64 && newSquareIndex >= 0;
                        prevFile = newFile;
                        prevRank = newRank;
                        if (!validMove) break;
                        Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                        if ((occupancy & newSquareBB) == 0) { 
                            Board newBoard = board;
                            newBoard.whiteBishops &= ~squareIndexBB; 
                            newBoard.whiteBishops |= newSquareBB; 
                            moves.push_back(newBoard);
                        } else if (enemyOccupancy & newSquareBB) { 
                            Board newBoard = board;
                            newBoard.whiteBishops &= ~squareIndexBB; 
                            newBoard.whiteBishops |= newSquareBB; 
                            if (board.blackPawns & newSquareBB) {
                                newBoard.blackPawns &= ~newSquareBB;
                            } else if (board.blackKnights & newSquareBB) {
                                newBoard.blackKnights &= ~newSquareBB;
                            } else if (board.blackBishops & newSquareBB) {
                                newBoard.blackBishops &= ~newSquareBB;
                            } else if (board.blackRooks & newSquareBB) {
                                newBoard.blackRooks &= ~newSquareBB;
                            } else if (board.blackQueen & newSquareBB) {
                                newBoard.blackQueen &= ~newSquareBB;
                            } else if (board.blackKing & newSquareBB) {
                                newBoard.blackKing &= ~newSquareBB;
                            }
                            moves.push_back(newBoard);
                            break; 
                        } else { // friendly piece at destination
                            break; 
                        }
                    }
                }
            }
        }
        const Bitboard rooks = board.whiteRooks;
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & rooks) { 
                int rookMoves[4] = { 8, -8, 1, -1 };
                for (int i = 0; i < 4; i++) {
                    int newSquareIndex = squareIndex;
                    int prevFile = squareIndex % 8;
                    int prevRank = squareIndex / 8;
                    while (true) {
                        newSquareIndex += rookMoves[i];
                        int newFile = newSquareIndex % 8;
                        int newRank = newSquareIndex / 8;
                        bool validMove = std::abs(newFile - prevFile) == 0 || std::abs(newRank - prevRank) == 0;
                        validMove &= newSquareIndex < 64 && newSquareIndex >= 0;
                        prevFile = newFile;
                        prevRank = newRank;
                        if (!validMove) break;
                        Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                        if ((occupancy & newSquareBB) == 0) { 
                            Board newBoard = board;
                            newBoard.whiteRooks &= ~squareIndexBB; 
                            newBoard.whiteRooks |= newSquareBB; 
                            moves.push_back(newBoard);
                        } else if (enemyOccupancy & newSquareBB) { 
                            Board newBoard = board;
                            newBoard.whiteRooks &= ~squareIndexBB; 
                            newBoard.whiteRooks |= newSquareBB; 
                            if (board.blackPawns & newSquareBB) {
                                newBoard.blackPawns &= ~newSquareBB;
                            } else if (board.blackKnights & newSquareBB) {
                                newBoard.blackKnights &= ~newSquareBB;
                            } else if (board.blackBishops & newSquareBB) {
                                newBoard.blackBishops &= ~newSquareBB;
                            } else if (board.blackRooks & newSquareBB) {
                                newBoard.blackRooks &= ~newSquareBB;
                            } else if (board.blackQueen & newSquareBB) {
                                newBoard.blackQueen &= ~newSquareBB;
                            } else if (board.blackKing & newSquareBB) {
                                newBoard.blackKing &= ~newSquareBB;
                            }
                            moves.push_back(newBoard);
                            break;
                        } else { 
                            break; 
                        }
                    }
                }
            }
        }
        const Bitboard queens = board.whiteQueens;
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & queens) { 
                // don't change the order of these moves. bishop moves must come first
                // see diagonalMove variable in the loop below
                int queenMoves[8] = { 7, 9, -7, -9, 8, -8, 1, -1 };
                for (int i = 0; i < 8; i++) {
                    int newSquareIndex = squareIndex;
                    int prevFile = squareIndex % 8;
                    int prevRank = squareIndex / 8;
                    bool diagonalMove = i < 4;
                    while (true) {
                        newSquareIndex += queenMoves[i];
                        int newFile = newSquareIndex % 8;
                        int newRank = newSquareIndex / 8;
                        bool validMove = diagonalMove ? std::abs(newFile - prevFile) == std::abs(newRank - prevRank) :
                                         std::abs(newFile - prevFile) == 0 || std::abs(newRank - prevRank) == 0;
                        validMove &= newSquareIndex < 64 && newSquareIndex >= 0;
                        prevFile = newFile;
                        prevRank = newRank;
                        if (!validMove) break;
                        Bitboard newSquareBB = (Bitboard)1 << newSquareIndex;
                        if ((occupancy & newSquareBB) == 0) { 
                            Board newBoard = board;
                            newBoard.whiteQueens &= ~squareIndexBB; 
                            newBoard.whiteQueens |= newSquareBB; 
                            moves.push_back(newBoard);
                        } else if (enemyOccupancy & newSquareBB) { 
                            Board newBoard = board;
                            newBoard.whiteQueens &= ~squareIndexBB; 
                            newBoard.whiteQueens |= newSquareBB; 
                            if (board.blackPawns & newSquareBB) {
                                newBoard.blackPawns &= ~newSquareBB;
                            } else if (board.blackKnights & newSquareBB) {
                                newBoard.blackKnights &= ~newSquareBB;
                            } else if (board.blackBishops & newSquareBB) {
                                newBoard.blackBishops &= ~newSquareBB;
                            } else if (board.blackRooks & newSquareBB) {
                                newBoard.blackRooks &= ~newSquareBB;
                            } else if (board.blackQueen & newSquareBB) {
                                newBoard.blackQueen &= ~newSquareBB;
                            } else if (board.blackKing & newSquareBB) {
                                newBoard.blackKing &= ~newSquareBB;
                            }
                            moves.push_back(newBoard);
                            break;
                        } else { 
                            break; 
                        }
                    }
                }
            }
        }
    }    
    return moves;
}

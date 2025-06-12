#include "movegen.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include "board.h"
#include "fen.h"
#include "attack_bitboards.h"
#include "utilities.h"

bool enablePerftDiagnostics = false;

int IncrementCastles() {
    static int castles = 0;
    return ++castles;
}

int IncrementCaptures() {
    static int captures = 0;
    return ++captures;
}

int IncrementEnPassant() {
    static int enPassant = 0;
    return ++enPassant;
}

static bool underThreat(const Board &board, int squareIndex, Color threatColor) {
    if (knightAttacks[squareIndex] & board.Knights(threatColor)) { return true; }
    if (pawnAttacks[ToggleColor(threatColor)][squareIndex] & board.Pawns(threatColor)) { return true; }
    if (kingAttacks[squareIndex] & board.Kings(threatColor)) { return true; } 

    Bitboard enemyQueenBB = board.Queens(threatColor);
    Bitboard enemyRookBB = board.Rooks(threatColor);
    Bitboard occupancy = board.Occupancy();

    Bitboard rookAttack = RookAttack(squareIndex, occupancy);
    if (rookAttack & (enemyRookBB | enemyQueenBB)) { return true; }

    Bitboard enemyBishopBB = board.Bishops(threatColor);
    Bitboard bishopAttack = BishopAttack(squareIndex, occupancy);
    if (bishopAttack & (enemyBishopBB | enemyQueenBB)) { return true; }

    return false;
}

static void printMoveWithCount(int fromSquareIndex, int toSquareIndex, std::uint64_t count) {
    int fromFile = fromSquareIndex % 8;
    int fromRank = fromSquareIndex / 8;
    int toFile = toSquareIndex % 8;
    int toRank = toSquareIndex / 8;
    char fromFileChar = 'a' + fromFile;
    char toFileChar = 'a' + toFile;
    std::cout << fromFileChar << fromRank + 1 << toFileChar << toRank + 1 << ": " << count << '\n';
}

// TODO: make move arrays constexpr
// TODO: change whiteTurn to color and don't hardcode 0/1 for black/white
// TODO: use Square instead of int or other integer types
std::uint64_t perftest(const Board& board, int depth, Color colorToMove) {
    if (depth == 0) {
        return 1;
    }
    std::uint64_t countLeafNodes = 0;
    const Color opponentColor = ToggleColor(colorToMove);
    const Bitboard occupancy = board.Occupancy();
    const Bitboard enemyOccupancy = board.Occupancy(opponentColor);
    const Bitboard friendlyOccupancy = board.Occupancy(colorToMove);
    const Bitboard promotionRankMask = PROMOTION_RANK_MASK[colorToMove];
    /*
    const int enemyPieceOffset = whiteTurn ? BLACK_PIECE_OFFSET : WHITE_PIECE_OFFSET;
    */

    const Bitboard king = board.Kings(colorToMove);
    Square originalKingSquareIndex = LSB(king);
    
    Bitboard pawns = board.Pawns(colorToMove);
    const int pawnDirection = colorToMove == White ? 8 : -8;
    const int pawnStartRank = colorToMove == White ? 1 : 6;
    const int leftCaptureOffset = colorToMove == White ? 7 : -9; 
    const int rightCaptureOffset = colorToMove == White ? 9 : -7;
    const int kingsideRookSquareIndex = colorToMove == White ? 7 : 63;
    const int queensideRookSquareIndex = colorToMove == White ? 0 : 56;
    while (pawns) {
        Square squareIndex = PopLSB(pawns);
        // TODO: avoid computing rank and file and find ways to use bitboard ops
        int rank = squareIndex / 8;
        int file = squareIndex % 8;

        int oneSquareForwardIndex = squareIndex + pawnDirection;
        if (oneSquareForwardIndex >= 0 && oneSquareForwardIndex < 64) { 
            Bitboard oneSquareForwardBB = (Bitboard)1 << oneSquareForwardIndex;
            if ((occupancy & oneSquareForwardBB) == 0) {
                Board newBoard = board;
                newBoard.Move(PieceType::Pawn, colorToMove, squareIndex, oneSquareForwardIndex);
                newBoard.enPassant = 0;
                if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                    if (newBoard.Pawns(colorToMove) & promotionRankMask) {
                        for (int i = 0; i < 4; i++)
                        {
                            Board promotionBoard = newBoard;
                            promotionBoard.PromotePawn(promotionTypes[i], colorToMove);
                            std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
                            if (enablePerftDiagnostics && depth == maxDepth) {
                                printMoveWithCount(squareIndex, oneSquareForwardIndex, movePerftCount);
                            }
                            countLeafNodes += movePerftCount;
                        }
                    } 
                    else {
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        if (enablePerftDiagnostics && depth == maxDepth) {
                            printMoveWithCount(squareIndex, oneSquareForwardIndex, movePerftCount);
                        }
                        countLeafNodes += movePerftCount;
                    }
                }

                // Double Push only if single push is possible
                if (rank == pawnStartRank)
                {
                    int twoSquaresForwardIndex = oneSquareForwardIndex + pawnDirection;
                    if (twoSquaresForwardIndex >= 0 && twoSquaresForwardIndex < 64) { 
                        Bitboard twoSquaresForwardBB = (Bitboard)1 << twoSquaresForwardIndex;
                        if ((occupancy & twoSquaresForwardBB) == 0)
                        {
                            Board doublePushBoard = board;
                            doublePushBoard.Move(PieceType::Pawn, colorToMove, squareIndex, twoSquaresForwardIndex);
                            doublePushBoard.enPassant = twoSquaresForwardIndex;
                            if (!underThreat(doublePushBoard, originalKingSquareIndex, opponentColor)) {
                                std::uint64_t movePerftCount = perftest(doublePushBoard, depth - 1, opponentColor);
                                if (enablePerftDiagnostics && depth == maxDepth) {
                                    printMoveWithCount(squareIndex, twoSquaresForwardIndex, movePerftCount);
                                }
                                countLeafNodes += movePerftCount;
                            }
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
                    newBoard.Move(PieceType::Pawn, colorToMove, squareIndex, leftDiagIndex);
                    RemovePiece(leftDiagIndex, newBoard, opponentColor);
                    newBoard.enPassant = 0;
                    if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                        if (newBoard.Pawns(colorToMove) & promotionRankMask) {
                            for (int i = 0; i < 4; i++)
                            {
                                Board promotionBoard = newBoard;
                                promotionBoard.PromotePawn(promotionTypes[i], colorToMove);
                                std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
                                if (enablePerftDiagnostics) {
                                    if (depth == 1) IncrementCaptures();
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                                    }
                                }
                                countLeafNodes += movePerftCount;
                            }
                        }
                        else {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            if (enablePerftDiagnostics) {
                                if (depth == 1) IncrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                                }
                            }
                            countLeafNodes += movePerftCount;
                        }
                    }
                }

                // left en passant capture
                int leftIndex = squareIndex - 1;
                if (board.enPassant == leftIndex) {
                    Board newBoard = board;
                    // TODO: refactor these move and remove calls to a single capture call
                    newBoard.Move(PieceType::Pawn, colorToMove, squareIndex, leftDiagIndex);
                    RemovePiece(leftIndex, newBoard, opponentColor);
                    newBoard.enPassant = 0; // reset en passant
                    if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        if (enablePerftDiagnostics) {
                            if (depth == 1) IncrementEnPassant();
                            if (depth == 1) IncrementCaptures();
                            if (depth == maxDepth) {
                                printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                            }
                        }
                        countLeafNodes += movePerftCount;
                    }
                }
            }
        }
        // Right Capture
        if (file < 7) {
            int rightDiagIndex = squareIndex + rightCaptureOffset;
            if (rightDiagIndex >= 0 && rightDiagIndex < 64) { 
                Bitboard rightDiagBB = (Bitboard)1 << rightDiagIndex;
                if (enemyOccupancy & rightDiagBB) {
                    Board newBoard = board;
                    newBoard.Move(PieceType::Pawn, colorToMove, squareIndex, rightDiagIndex);
                    RemovePiece(rightDiagIndex, newBoard, opponentColor);
                    newBoard.enPassant = 0;
                    if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                        if (newBoard.Pawns(colorToMove) & promotionRankMask) {
                            for (int i = 0; i < 4; i++)
                            {
                                Board promotionBoard = newBoard;
                                promotionBoard.PromotePawn(promotionTypes[i], colorToMove);
                                std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
                                if (enablePerftDiagnostics) {
                                    if (depth == 1) IncrementCaptures();
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                                    }
                                }
                                countLeafNodes += movePerftCount;
                            }
                        } 
                        else {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            if (enablePerftDiagnostics) {
                                if (depth == 1) IncrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                                }
                            }
                            countLeafNodes += movePerftCount;
                        }
                    }
                }
                // en passant capture right
                int rightIndex = squareIndex + 1;
                if (board.enPassant == rightIndex) {
                    Board newBoard = board;
                    newBoard.Move(PieceType::Pawn, colorToMove, squareIndex, rightDiagIndex);
                    RemovePiece(rightIndex, newBoard, opponentColor);
                    newBoard.enPassant = 0; 
                    if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        if (enablePerftDiagnostics) { 
                            if (depth == 1) IncrementEnPassant();
                            if (depth == 1) IncrementCaptures();
                            if (depth == maxDepth) {
                                printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                            }
                        }
                        countLeafNodes += movePerftCount;
                    }
                }
            }
        }
    }

    Bitboard knights = board.Knights(colorToMove);
    constexpr int knightMoves[8] = {6, 10, 15, 17, -6, -10, -15, -17};
    while (knights) {
        Square squareIndex = PopLSB(knights);
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
                newBoard.Move(PieceType::Knight, colorToMove, squareIndex, newSquareIndex);
                bool isCapture = false;
                if (enemyOccupancy & newSquareBB) { 
                    RemovePiece(newSquareIndex, newBoard, opponentColor);
                    isCapture = true;
                }
                
                newBoard.enPassant = 0; 
                if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                    std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                    if (enablePerftDiagnostics) {
                        if (isCapture && depth == 1) IncrementCaptures();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                    }
                    countLeafNodes += movePerftCount;
                }
            }
        }
    }

    struct SlidingPiece {
        PieceType type;
        const int* moves;
        int numMoves;
    };

    int bishopMoves[4] = {7, 9, -7, -9};
    int rookMoves[4] = {8, -8, 1, -1};
    int queenMoves[8] = {7, 9, -7, -9, 8, -8, 1, -1}; // Diagonal first

    SlidingPiece slidingPieces[] = {
        {PieceType::Bishop, bishopMoves, 4},
        {PieceType::Rook, rookMoves, 4},
        {PieceType::Queen, queenMoves, 8}
    };
    
    const auto friendlyBitboards = board.Bitboards(colorToMove);
    for (const SlidingPiece& pieceInfo : slidingPieces) {
        Bitboard pieceBoard = board.bitboards2D[colorToMove][(int)pieceInfo.type];
        while (pieceBoard) {
            Square squareIndex = PopLSB(pieceBoard);
            for (int i = 0; i < pieceInfo.numMoves; i++) {
                int moveOffset = pieceInfo.moves[i];
                int newSquareIndex = squareIndex;
                int prevFile = squareIndex % 8;
                int prevRank = squareIndex / 8;
                bool diagonalMove = pieceInfo.type == PieceType::Bishop || (pieceInfo.type == PieceType::Queen && i < 4);
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
                    newBoard.Move(pieceInfo.type, colorToMove, squareIndex, newSquareIndex);
                    // TODO: these two blocks can be combined with a break if it's a capture
                    if (enemyOccupancy & newSquareBB) {
                        RemovePiece(newSquareIndex, newBoard, opponentColor);
                        
                        newBoard.enPassant = 0; 
                        
                        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                            if (pieceInfo.type == PieceType::Rook) {
                                if (squareIndex == kingsideRookSquareIndex) {
                                    if (colorToMove == Color::White) {
                                        newBoard.whiteKingsideCastlingRight = false;
                                    } else {
                                        newBoard.blackKingsideCastlingRight = false;
                                    }
                                } else if (squareIndex == queensideRookSquareIndex) {
                                    if (colorToMove == Color::White) {
                                        newBoard.whiteQueensideCastlingRight = false;
                                    } else {
                                        newBoard.blackQueensideCastlingRight = false;
                                    }
                                }
                            }
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            if (enablePerftDiagnostics) {
                                if (depth == 1) IncrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                                }
                            }
                            countLeafNodes += movePerftCount;
                        }
                        break; // Stop sliding after capturing an enemy piece
                    } else {
                        newBoard.enPassant = 0;
                        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                            if (pieceInfo.type == PieceType::Rook) {
                                if (squareIndex == kingsideRookSquareIndex) {
                                    if (colorToMove == Color::White) {
                                        newBoard.whiteKingsideCastlingRight = false;
                                    } else {
                                        newBoard.blackKingsideCastlingRight = false;
                                    }
                                } else if (squareIndex == queensideRookSquareIndex) {
                                    if (colorToMove == Color::White) {
                                        newBoard.whiteQueensideCastlingRight = false;
                                    } else {
                                        newBoard.blackQueensideCastlingRight = false;
                                    }
                                }
                            }
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            if (enablePerftDiagnostics) {
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                                }
                            }
                            countLeafNodes += movePerftCount;
                        }
                    }
                }
            }
        }
    }

    int kingMoves[8] = {7, 9, -7, -9, 8, -8, 1, -1};
    int file = originalKingSquareIndex % 8;
    int rank = originalKingSquareIndex / 8;
    for (int moveOffset : kingMoves)
    {
        int newSquareIndex = originalKingSquareIndex + moveOffset;
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
            newBoard.Move(PieceType::King, colorToMove, originalKingSquareIndex, newSquareIndex);

            bool isCapture = false;
            if (enemyOccupancy & newSquareBB) {
                RemovePiece(newSquareIndex, newBoard, opponentColor);
                isCapture = true;
            }
            if (!underThreat(newBoard, newSquareIndex, opponentColor)) {
                newBoard.RemoveCastlingRights(colorToMove);
                // TODO: find a better way to handle resetting en passant. This is error 
                // prone because it has to be done every recursive call
                newBoard.enPassant = 0;
                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                if (enablePerftDiagnostics) {
                    if (depth == 1 && isCapture) IncrementCaptures();
                    if (depth == maxDepth) {
                        printMoveWithCount(originalKingSquareIndex, newSquareIndex, movePerftCount);
                    }
                }
                countLeafNodes += movePerftCount;
            }
        }
    }

    if (board.KingsideCastlingRight(colorToMove)) {
        bool squaresVacant = (occupancy & ((Bitboard)1 << (originalKingSquareIndex + 1))) == 0 &&
                             (occupancy & ((Bitboard)1 << (originalKingSquareIndex + 2))) == 0;
        if (squaresVacant) {
            bool enemyPrevents = underThreat(board, originalKingSquareIndex, opponentColor)     ||
                                 underThreat(board, originalKingSquareIndex + 1, opponentColor) ||
                                 underThreat(board, originalKingSquareIndex + 2, opponentColor);
            if (!enemyPrevents) {
                Board newBoard = board;
                int newSquareIndex = originalKingSquareIndex + 2;
                newBoard.Move(PieceType::King, colorToMove, originalKingSquareIndex, originalKingSquareIndex + 2);
                newBoard.Move(PieceType::Rook, colorToMove, kingsideRookSquareIndex, originalKingSquareIndex + 1);
                newBoard.RemoveCastlingRights(colorToMove);
                // no need to check for threat, already checked above
                newBoard.enPassant = 0;
                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                if (enablePerftDiagnostics) {
                    if (depth == 1) IncrementCastles();
                    if (depth == maxDepth) {
                        printMoveWithCount(originalKingSquareIndex, newSquareIndex, movePerftCount);
                    }
                }
                countLeafNodes += movePerftCount;
            }
        }
    }
    if (board.QueensideCastlingRight(colorToMove)) {
        bool squaresVacant = (occupancy & ((Bitboard)1 << (originalKingSquareIndex - 1))) == 0 &&
                             (occupancy & ((Bitboard)1 << (originalKingSquareIndex - 2))) == 0 &&
                             (occupancy & ((Bitboard)1 << (originalKingSquareIndex - 3))) == 0;
        if (squaresVacant) {
            bool enemyPrevents = underThreat(board, originalKingSquareIndex, opponentColor)     ||
                                 underThreat(board, originalKingSquareIndex - 1, opponentColor) ||
                                 underThreat(board, originalKingSquareIndex - 2, opponentColor);
            if (!enemyPrevents) {
                Board newBoard = board;
                int newSquareIndex = originalKingSquareIndex - 2;
                Bitboard queensideRookBB = (Bitboard)1 << queensideRookSquareIndex;
                newBoard.Move(PieceType::King, colorToMove, originalKingSquareIndex, originalKingSquareIndex - 2);
                newBoard.Move(PieceType::Rook, colorToMove, queensideRookSquareIndex, originalKingSquareIndex - 1);
                // no need to check for threat, already checked above
                newBoard.RemoveCastlingRights(colorToMove);
                newBoard.enPassant = 0;
                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                if (enablePerftDiagnostics) {
                    if (depth == 1) IncrementCastles();
                    if (depth == maxDepth) {
                        printMoveWithCount(originalKingSquareIndex, newSquareIndex, movePerftCount);
                    }
                }
                countLeafNodes += movePerftCount;
            }
        }
    }
    
    return countLeafNodes;
}

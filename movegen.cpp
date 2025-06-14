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
    const int kingsideRookSquareIndex = colorToMove == White ? 7 : 63;
    const int queensideRookSquareIndex = colorToMove == White ? 0 : 56;

    const Bitboard singlePushes = colorToMove == White ? (pawns << 8) & ~occupancy :
                                                              (pawns >> 8) & ~occupancy;
    Bitboard promotions = singlePushes & promotionRankMask;
    Bitboard nonPromotions = singlePushes & ~promotionRankMask;
    while (promotions) {
        Square to = PopLSB(promotions);
        Square from = to - pawnDirection;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        newBoard.enPassant = 0;
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        Bitboard toBB = ToBitboard(to);
        for (int i = 0; i < 4; i++) {
            Board promotionBoard = newBoard;
            promotionBoard.bitboards2D[colorToMove][(int)PieceType::Pawn] &= ~toBB;
            promotionBoard.bitboards2D[colorToMove][(int)promotionTypes[i]] |= toBB;
            std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
            if (enablePerftDiagnostics && depth == maxDepth) {
                printMoveWithCount(from, to, movePerftCount);
            }
            countLeafNodes += movePerftCount;
        }
    }
    while (nonPromotions) {
        Square to = PopLSB(nonPromotions); 
        Square from = to - pawnDirection;
        Board newBoard = board;
        newBoard.enPassant = 0;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
        if (enablePerftDiagnostics && depth == maxDepth) {
            printMoveWithCount(from, to, movePerftCount);
        }
        countLeafNodes += movePerftCount;
    }
    
    const Bitboard doublePushRankMask = colorToMove == White ? RANK_MASK[(int)Rank::Third] : RANK_MASK[(int)Rank::Sixth];
    Bitboard doublePushes = colorToMove == White ? ((singlePushes & doublePushRankMask) << 8) & ~occupancy :
                                                   ((singlePushes & doublePushRankMask) >> 8) & ~occupancy;
    while (doublePushes) {
        Square to = PopLSB(doublePushes);
        Square from = to - pawnDirection * 2;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        newBoard.enPassant = to - pawnDirection;
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
        if (enablePerftDiagnostics && depth == maxDepth) {
            printMoveWithCount(from, to, movePerftCount);
        }
        countLeafNodes += movePerftCount;
    }

    const int leftCaptureOffset = colorToMove == White ? 7 : -9; 
    const int rightCaptureOffset = colorToMove == White ? 9 : -7;
    
    const Bitboard pawnAttackMask = board.enPassant == 0 ? enemyOccupancy : enemyOccupancy | ToBitboard(board.enPassant);
    const Bitboard leftCaptures = (colorToMove == White ? (pawns << 7) & pawnAttackMask : (pawns >> 9) & pawnAttackMask) & ~FILE_MASK[7];
    promotions = leftCaptures & promotionRankMask;
    nonPromotions = leftCaptures & ~promotionRankMask;
    while (promotions) {
        Square to = PopLSB(promotions);
        Square from = to - leftCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        RemovePiece(to, newBoard, opponentColor);
        newBoard.enPassant = 0;
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        Bitboard toBB = ToBitboard(to);
        for (int i = 0; i < 4; i++) {
            Board promotionBoard = newBoard;
            promotionBoard.bitboards2D[colorToMove][(int)PieceType::Pawn] &= ~toBB;
            promotionBoard.bitboards2D[colorToMove][(int)promotionTypes[i]] |= toBB;
            std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
            if (enablePerftDiagnostics && depth == maxDepth) {
                printMoveWithCount(from, to, movePerftCount);
            }
            countLeafNodes += movePerftCount;
        }
    }
    while (nonPromotions) {
        Square to = PopLSB(nonPromotions);
        Square from = to - leftCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        // TODO: fix enPassant everywhere. Switching to standard FEN convention (en passant is attack square)
        if (board.enPassant == to) {
            RemovePiece(to - pawnDirection, newBoard, opponentColor);
        }
        else {
            RemovePiece(to, newBoard, opponentColor);
        }
        newBoard.enPassant = 0;
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
        if (enablePerftDiagnostics && depth == maxDepth) {
            printMoveWithCount(from, to, movePerftCount);
        }
        countLeafNodes += movePerftCount;
    }

    const Bitboard rightCaptures = (colorToMove == White ? (pawns << 9) & pawnAttackMask : (pawns >> 7) & pawnAttackMask) & ~FILE_MASK[0];
    promotions = rightCaptures & promotionRankMask;
    nonPromotions = rightCaptures & ~promotionRankMask;
    while (promotions) {
        Square to = PopLSB(promotions);
        Square from = to - rightCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        RemovePiece(to, newBoard, opponentColor);
        newBoard.enPassant = 0;
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        Bitboard toBB = ToBitboard(to);
        for (int i = 0; i < 4; i++) {
            Board promotionBoard = newBoard;
            promotionBoard.bitboards2D[colorToMove][(int)PieceType::Pawn] &= ~toBB;
            promotionBoard.bitboards2D[colorToMove][(int)promotionTypes[i]] |= toBB;
            std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
            if (enablePerftDiagnostics && depth == maxDepth) {
                printMoveWithCount(from, to, movePerftCount);
            }
            countLeafNodes += movePerftCount;
        }
    }
    while (nonPromotions) {
        Square to = PopLSB(nonPromotions);
        Square from = to - rightCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        if (board.enPassant == to) {
            RemovePiece(to - pawnDirection, newBoard, opponentColor);
        }
        else {
            RemovePiece(to, newBoard, opponentColor);
        }
        newBoard.enPassant = 0;
        if (underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            continue;
        }
        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
        if (enablePerftDiagnostics && depth == maxDepth) {
            printMoveWithCount(from, to, movePerftCount);
        }
        countLeafNodes += movePerftCount;
    }

    Bitboard knights = board.Knights(colorToMove);
    while (knights) {
        Square squareIndex = PopLSB(knights);
        int file = squareIndex % 8;
        int rank = squareIndex / 8;
        Bitboard knightMoves = knightAttacks[squareIndex];
        while (knightMoves) {
            Square newSquareIndex = PopLSB(knightMoves);
            Bitboard newSquareBB = ToBitboard(newSquareIndex);
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
        Bitboard (*Attack)(Square, Bitboard);
        int numMoves;
    };

    SlidingPiece slidingPieces[] = {
        {PieceType::Bishop, BishopAttack, 4},
        {PieceType::Rook, RookAttack, 4},
        {PieceType::Queen, QueenAttack, 8}
    };
    
    const auto friendlyBitboards = board.Bitboards(colorToMove);
    for (const SlidingPiece& pieceInfo : slidingPieces) {
        Bitboard pieceBoard = board.bitboards2D[colorToMove][(int)pieceInfo.type];
        while (pieceBoard) {
            Square squareIndex = PopLSB(pieceBoard);
            Bitboard moves = pieceInfo.Attack(squareIndex, occupancy);
            while (moves) {
                Square newSquareIndex = PopLSB(moves);
                Bitboard newSquareBB = ToBitboard(newSquareIndex);
                if (friendlyOccupancy & newSquareBB) continue; 
                Board newBoard = board;
                newBoard.Move(pieceInfo.type, colorToMove, squareIndex, newSquareIndex);
                if (enemyOccupancy & newSquareBB) {
                    RemovePiece(newSquareIndex, newBoard, opponentColor);
                } 
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

    Bitboard kingMoves = kingAttacks[originalKingSquareIndex];
    while (kingMoves) {
        Square newSquareIndex = PopLSB(kingMoves);
        Bitboard newSquareBB = ToBitboard(newSquareIndex);
        if ((friendlyOccupancy & newSquareBB) == 0) {
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

#include "movegen.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <span>
#include "board.h"
#include "utilities.h"
#include "fen.h"

// TODO: take into account castling, en passant, promotion, check, checkmate,
// stalemate, draw.

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

// TODO: use uint8_t for squareIndex
// TODO: move to Board, or move all these low-level methods that operate on Board to a different file
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

void removePiece(int squareIndex, Board& board, Color colorToRemove) {
    assert(squareIndex >= 0 && squareIndex < 64);
    removePiece(squareIndex, board.Bitboards(colorToRemove));
    board.whiteKingsideCastlingRight = board.whiteKingsideCastlingRight && squareIndex != 7;
    board.whiteQueensideCastlingRight = board.whiteQueensideCastlingRight && squareIndex != 0;
    board.blackKingsideCastlingRight = board.blackKingsideCastlingRight && squareIndex != 63;
    board.blackQueensideCastlingRight = board.blackQueensideCastlingRight && squareIndex != 56;
}
/*
static bool underThreat(const Board& board, int squareIndex, Color threatColor, bool temp) {
    if (knightAttacks[squareIndex] & board.knights[threatColor]) return true;
    if (pawnAttacks[threatColor][squareIndex] & board.pawns[threatColor]) return true;
    if (kingAttacks[sq] & board.kings[threatColor]) return true;
    Bitboard occupancy = board.allPieces();
    if (bishopAttacks[squareIndex][occupancy] & (board.bishops[threatColor] | board.queens[threatColor])) return true;
    if (rookAttacks[squareIndex][occupancy] & (board.rooks[threatColor] | board.queens[threatColor])) return true;

    return false;
}
*/

static bool underThreat(const Board &board, int squareIndex, Color threatColor) {
    Bitboard bb = (Bitboard)1 << squareIndex;

    // Check up, down, left, right for rook/queen
    Bitboard enemyQueenBB = board.Queens(threatColor);
    Bitboard enemyRookBB = board.Rooks(threatColor);
    Bitboard occupancy = board.Occupancy();
    const int squareRank = squareIndex / 8;
    const int squareFile = squareIndex % 8;
    for (int rank = squareRank + 1; rank < 8; rank++)
    {
        int index = rank * 8 + squareFile;
        Bitboard indexBB = (Bitboard)1 << index;
        if ((enemyQueenBB & indexBB) || (enemyRookBB & indexBB)) {
            return true;
        }
        if (occupancy & indexBB) {// friendly or enemy non-rook/queen piece, no need to continue
            break;
        }
    }
    for (int rank = squareRank - 1; rank >= 0; rank--)
    {
        int index = rank * 8 + squareFile;
		Bitboard indexBB = (Bitboard)1 << index;
		if ((enemyQueenBB & indexBB) || (enemyRookBB & indexBB)) {
			return true;
		}
		if (occupancy & indexBB) {// friendly or enemy non-rook/queen piece, no need to continue
			break;
		}
    }
    for (int file = squareFile + 1; file < 8; file++)
    {
        int index = squareRank * 8 + file;
		Bitboard indexBB = (Bitboard)1 << index;
		if ((enemyQueenBB & indexBB) || (enemyRookBB & indexBB)) {
			return true;
		}
		if (occupancy & indexBB) {// friendly or enemy non-rook/queen piece, no need to continue
			break;
		}
    }
    for (int file = squareFile - 1; file >= 0; file--)
    {
        int index = squareRank * 8 + file;
        Bitboard indexBB = (Bitboard)1 << index;
        if ((enemyQueenBB & indexBB) || (enemyRookBB & indexBB)) {
            return true;
        }
        if (occupancy & indexBB) {// friendly or enemy non-rook/queen piece, no need to continue
            break;
        }
    }

    // Check diagonals for bishop/queen
    Bitboard enemyBishopBB = board.Bishops(threatColor);
    for (int rank = squareRank + 1, file = squareFile + 1; rank < 8 && file < 8; rank++, file++)
    {
        int index = rank * 8 + file;
		Bitboard indexBB = (Bitboard)1 << index;
		if ((enemyQueenBB & indexBB) || (enemyBishopBB & indexBB)) {
			return true;
		}
		if (occupancy & indexBB) {// friendly or enemy non-bishop/queen piece, no need to continue
			break;
		}
    }
    for (int rank = squareRank + 1, file = squareFile - 1; rank < 8 && file >= 0; rank++, file--)
    {
        int index = rank * 8 + file;
		Bitboard indexBB = (Bitboard)1 << index;
		if ((enemyQueenBB & indexBB) || (enemyBishopBB & indexBB)) {
			return true;
		}
        if (occupancy & indexBB) {// friendly or enemy non-bishop/queen piece, no need to continue
            break;
        }
    }
    for (int rank = squareRank - 1, file = squareFile + 1; rank >= 0 && file < 8; rank--, file++)
    {
        int index = rank * 8 + file;
        Bitboard indexBB = (Bitboard)1 << index;
        if ((enemyQueenBB & indexBB) || (enemyBishopBB & indexBB)) {
            return true;
        }
        if (occupancy & indexBB) {// friendly or enemy non-bishop/queen piece, no need to continue
            break;
        }
    }
    for (int rank = squareRank - 1, file = squareFile - 1; rank >= 0 && file >= 0; rank--, file--)
    {
        int index = rank * 8 + file;
        Bitboard indexBB = (Bitboard)1 << index;
        if ((enemyQueenBB & indexBB) || (enemyBishopBB & indexBB)) {
            return true;
        }
        if (occupancy & indexBB) {// friendly or enemy non-bishop/queen piece, no need to continue
            break;
        }
    }

    Bitboard enemyKnightBB = board.Knights(threatColor);
    constexpr int knightMoves[8] = {6, 10, 15, 17, -6, -10, -15, -17};
    for (int moveOffset : knightMoves)
    {
        int newSquareIndex = squareIndex + moveOffset;
        if (newSquareIndex < 0 || newSquareIndex >= 64)
            continue;

        int newFile = newSquareIndex % 8;
        int newRank = newSquareIndex / 8;

        bool validMove = (std::abs(newFile - squareFile) == 1 && std::abs(newRank - squareRank) == 2) ||
                         (std::abs(newFile - squareFile) == 2 && std::abs(newRank - squareRank) == 1);
        if (!validMove) {
            continue;
        }
		if (enemyKnightBB & ((Bitboard)1 << newSquareIndex)) {
            return true;
		}
    }

	Bitboard enemyPawnBB = board.Pawns(threatColor);
    int pawnCaptureOffsets[2] = {7, 9};
    if (threatColor == Color::White) 
    {
        pawnCaptureOffsets[0] *= -1;
        pawnCaptureOffsets[1] *= -1;
    }
    for (int i = 0; i < 2; i++)
    {
        int newSquareIndex = squareIndex + pawnCaptureOffsets[i];
        if (newSquareIndex < 0 || newSquareIndex >= 64)
            continue;

        int newFile = newSquareIndex % 8;
        int newRank = newSquareIndex / 8;

        bool validMove = (std::abs(newFile - squareFile) == 1 && std::abs(newRank - squareRank) == 1);
        if (!validMove)
        {
            continue;
        }
		if (enemyPawnBB & ((Bitboard)1 << newSquareIndex)) {
			return true;
		}
    }

    Bitboard enemyKingBB = board.Kings(threatColor); 
    int kingAttackOffsets[8] = {7, 9, -7, -9, 8, -8, 1, -1};
    for (int offset : kingAttackOffsets)
    {
        int attackedSquare = squareIndex + offset;
        if (attackedSquare < 0 || attackedSquare >= 64)
            continue;

        // Check for board wrap-around for king moves
        int attackedFile = attackedSquare % 8;
        int attackedRank = attackedSquare / 8;
        if (std::abs(attackedFile - squareFile) > 1 || std::abs(attackedRank - squareRank) > 1)
            continue;

        if (((Bitboard)1 << attackedSquare) & enemyKingBB)
        {
            return true;
        }
    }

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
    int originalKingSquareIndex = 0; 
    while (originalKingSquareIndex < 64)
    {
        Bitboard squareIndexBB = (Bitboard)1 << originalKingSquareIndex;
        if (squareIndexBB & king)
        {
            break;
        }
        originalKingSquareIndex++;
    }
    
    const Bitboard pawns = board.Pawns(colorToMove);
    const int pawnDirection = colorToMove == White ? 8 : -8;
    const int pawnStartRank = colorToMove == White ? 1 : 6;
    const int leftCaptureOffset = colorToMove == White ? 7 : -9; 
    const int rightCaptureOffset = colorToMove == White ? 9 : -7;
    const int kingsideRookSquareIndex = colorToMove == White ? 7 : 63;
    const int queensideRookSquareIndex = colorToMove == White ? 0 : 56;
    for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
        Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
        if (squareIndexBB & pawns)
        {
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
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, oneSquareForwardIndex, movePerftCount);
                                }
                                #endif
                                countLeafNodes += movePerftCount;
                            }
                        } 
                        else {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            #ifdef PRINT_DIAGNOSTICS
                            if (depth == maxDepth) {
                                printMoveWithCount(squareIndex, oneSquareForwardIndex, movePerftCount);
                            }
                            #endif
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
                                    #ifdef PRINT_DIAGNOSTICS
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, twoSquaresForwardIndex, movePerftCount);
                                    }
                                    #endif
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
                        removePiece(leftDiagIndex, newBoard, opponentColor);
                        newBoard.enPassant = 0;
                        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                            if (newBoard.Pawns(colorToMove) & promotionRankMask) {
                                for (int i = 0; i < 4; i++)
                                {
                                    Board promotionBoard = newBoard;
                                    promotionBoard.PromotePawn(promotionTypes[i], colorToMove);
                                    std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
                                    #ifdef PRINT_DIAGNOSTICS
                                    if (depth == 1) IncrementCaptures();
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                                    }
                                    #endif
                                    countLeafNodes += movePerftCount;
                                }
                            }
                            else {
                                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == 1) IncrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                                }
                                #endif
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
                        removePiece(leftIndex, newBoard, opponentColor);
                        newBoard.enPassant = 0; // reset en passant
                        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            #ifdef PRINT_DIAGNOSTICS
                            if (depth == 1) IncrementEnPassant();
                            if (depth == 1) IncrementCaptures();
                            if (depth == maxDepth) {
                                printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                            }
                            #endif
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
                        removePiece(rightDiagIndex, newBoard, opponentColor);
                        newBoard.enPassant = 0;
                        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                            if (newBoard.Pawns(colorToMove) & promotionRankMask) {
                                for (int i = 0; i < 4; i++)
                                {
                                    Board promotionBoard = newBoard;
                                    promotionBoard.PromotePawn(promotionTypes[i], colorToMove);
                                    std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, opponentColor);
                                    #ifdef PRINT_DIAGNOSTICS
                                    if (depth == 1) IncrementCaptures();
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                                    }
                                    #endif
                                    countLeafNodes += movePerftCount;
                                }
                            } 
                            else {
                                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == 1) IncrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                                }
                                #endif
                                countLeafNodes += movePerftCount;
                            }
                        }
                    }
                    // en passant capture right
                    int rightIndex = squareIndex + 1;
                    if (board.enPassant == rightIndex) {
                        Board newBoard = board;
                        newBoard.Move(PieceType::Pawn, colorToMove, squareIndex, rightDiagIndex);
                        removePiece(rightIndex, newBoard, opponentColor);
                        newBoard.enPassant = 0; 
                        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                            #ifdef PRINT_DIAGNOSTICS
                            if (depth == 1) IncrementEnPassant();
                            if (depth == 1) IncrementCaptures();
                            if (depth == maxDepth) {
                                printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                            }
                            #endif
                            countLeafNodes += movePerftCount;
                        }
                    }
                }
            }
        }
    }

    const Bitboard knights = board.Knights(colorToMove);
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
                    newBoard.Move(PieceType::Knight, colorToMove, squareIndex, newSquareIndex);
                    bool isCapture = false;
                    if (enemyOccupancy & newSquareBB) { 
                        removePiece(newSquareIndex, newBoard, opponentColor);
                        isCapture = true;
                    }
                    
                    newBoard.enPassant = 0; 
                    if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        #ifdef PRINT_DIAGNOSTICS
                        if (isCapture && depth == 1) IncrementCaptures();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                        #endif
                        countLeafNodes += movePerftCount;
                    }
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
        const Bitboard pieceBoard = board.bitboards2D[colorToMove][(int)pieceInfo.type];
        for (int squareIndex = 0; squareIndex < 64; squareIndex++) {
            Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
            if (squareIndexBB & pieceBoard) {
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
                            removePiece(newSquareIndex, newBoard, opponentColor);
                            
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
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == 1) IncrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                                }
                                #endif
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
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                                }
                                #endif
                                countLeafNodes += movePerftCount;
                            }
                        }
                    }
                }
            }
        }
    }

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
                    newBoard.Move(PieceType::King, colorToMove, squareIndex, newSquareIndex);

                    bool isCapture = false;
                    if (enemyOccupancy & newSquareBB) {
                        removePiece(newSquareIndex, newBoard, opponentColor);
                        isCapture = true;
                    }
                    if (!underThreat(newBoard, newSquareIndex, opponentColor)) {
                        newBoard.RemoveCastlingRights(colorToMove);
                        // TODO: find a better way to handle resetting en passant. This is error 
                        // prone because it has to be done every recursive call
                        newBoard.enPassant = 0;
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        #ifdef PRINT_DIAGNOSTICS
                        if (depth == 1 && isCapture) IncrementCaptures();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                        #endif
                        countLeafNodes += movePerftCount;
                    }
                }
            }

            if (board.KingsideCastlingRight(colorToMove)) {
                bool squaresVacant = (occupancy & ((Bitboard)1 << (squareIndex + 1))) == 0 &&
                                     (occupancy & ((Bitboard)1 << (squareIndex + 2))) == 0;
                if (squaresVacant) {
                    bool enemyPrevents = underThreat(board, originalKingSquareIndex, opponentColor)     ||
                                         underThreat(board, originalKingSquareIndex + 1, opponentColor) ||
                                         underThreat(board, originalKingSquareIndex + 2, opponentColor);
                    if (!enemyPrevents) {
                        Board newBoard = board;
                        int newSquareIndex = squareIndex + 2;
                        newBoard.Move(PieceType::King, colorToMove, squareIndex, squareIndex + 2);
                        newBoard.Move(PieceType::Rook, colorToMove, kingsideRookSquareIndex, squareIndex + 1);
                        newBoard.RemoveCastlingRights(colorToMove);
                        // no need to check for threat, already checked above
                        newBoard.enPassant = 0;
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        #ifdef PRINT_DIAGNOSTICS
                        if (depth == 1) IncrementCastles();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                        #endif
                        countLeafNodes += movePerftCount;
                    }
                }
            }
            if (board.QueensideCastlingRight(colorToMove)) {
                bool squaresVacant = (occupancy & ((Bitboard)1 << (squareIndex - 1))) == 0 &&
                                     (occupancy & ((Bitboard)1 << (squareIndex - 2))) == 0 &&
                                     (occupancy & ((Bitboard)1 << (squareIndex - 3))) == 0;
                if (squaresVacant) {
                    bool enemyPrevents = underThreat(board, originalKingSquareIndex, opponentColor)     ||
                                         underThreat(board, originalKingSquareIndex - 1, opponentColor) ||
                                         underThreat(board, originalKingSquareIndex - 2, opponentColor);
                    if (!enemyPrevents) {
                        Board newBoard = board;
                        int newSquareIndex = squareIndex - 2;
                        Bitboard queensideRookBB = (Bitboard)1 << queensideRookSquareIndex;
                        newBoard.Move(PieceType::King, colorToMove, squareIndex, squareIndex - 2);
                        newBoard.Move(PieceType::Rook, colorToMove, queensideRookSquareIndex, squareIndex - 1);
                        // no need to check for threat, already checked above
                        newBoard.RemoveCastlingRights(colorToMove);
                        newBoard.enPassant = 0;
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, opponentColor);
                        #ifdef PRINT_DIAGNOSTICS
                        if (depth == 1) IncrementCastles();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                        #endif
                        countLeafNodes += movePerftCount;
                    }
                }
            }
            break; // Only one king per side
        }
    }
    
    return countLeafNodes;
}

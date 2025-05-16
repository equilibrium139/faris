#include "movegen.h"
#include <cassert>
#include <cmath>
#include <span>
#include "utilities.h"
#include "fen.h"

// TODO: take into account castling, en passant, promotion, check, checkmate,
// stalemate, draw.

#define PRINT_DIAGNOSTICS

int incrementCastles() {
    static int castles = 0;
    return ++castles;
}

int incrementCaptures() {
    static int captures = 0;
    return ++captures;
}

int incrementEnPassant() {
    static int enPassant = 0;
    return ++enPassant;
}

// TODO: use uint8_t for squareIndex
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

void removePiece(int squareIndex, Board& board, bool white) {
    assert(squareIndex >= 0 && squareIndex < 64);
    removePiece(squareIndex, white ? board.whiteBitboards() : board.blackBitboards());
    board.whiteKingsideCastlingRight = board.whiteKingsideCastlingRight && squareIndex != 7;
    board.whiteQueensideCastlingRight = board.whiteQueensideCastlingRight && squareIndex != 0;
    board.blackKingsideCastlingRight = board.blackKingsideCastlingRight && squareIndex != 63;
    board.blackQueensideCastlingRight = board.blackQueensideCastlingRight && squareIndex != 56;
}

static bool underThreat(const Board &board, int squareIndex, bool threatColor) {
    Bitboard bb = (Bitboard)1 << squareIndex;

    // Check up, down, left, right for rook/queen
    Bitboard enemyQueenBB = threatColor ? board.whiteQueens : board.blackQueens;
	Bitboard enemyRookBB = threatColor ? board.whiteRooks : board.blackRooks;
    Bitboard occupancy = board.allPieces();
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
	Bitboard enemyBishopBB = threatColor ? board.whiteBishops : board.blackBishops;
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

	Bitboard enemyKnightBB = threatColor ? board.whiteKnights : board.blackKnights;
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

	Bitboard enemyPawnBB = threatColor ? board.whitePawns : board.blackPawns;
    int pawnCaptureOffsets[2] = {7, 9};
    if (threatColor) 
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

    Bitboard enemyKingBB = threatColor ? board.whiteKing : board.blackKing; 
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
std::uint64_t perftest(const Board& board, int depth, bool whiteTurn) {
    Fen fen = {board, 0, 1, whiteTurn};
    std::string fenStr = toFen(fen);
    Fen strToFen = parseFEN(fenStr);
    assert(strToFen.board == board);
    if (depth == 0) {
        return 1;
    }
    std::uint64_t countLeafNodes = 0;
    const Bitboard occupancy = board.allPieces();
    const Bitboard enemyOccupancy = whiteTurn ? board.blackPieces() : board.whitePieces();
    const Bitboard friendlyOccupancy = whiteTurn ? board.whitePieces() : board.blackPieces();
    std::span<const Bitboard, 6> enemyPieces = whiteTurn ? board.blackBitboards() : 
                                                           board.whiteBitboards(); 
    std::span<const Bitboard, 6> friendlyPieces = whiteTurn ? board.whiteBitboards():
                                                              board.blackBitboards();
    const int friendlyPieceOffset = whiteTurn ? WHITE_PIECE_OFFSET : BLACK_PIECE_OFFSET;
    const int enemyPieceOffset = whiteTurn ? BLACK_PIECE_OFFSET : WHITE_PIECE_OFFSET;
    const Bitboard pawns = friendlyPieces[PAWN_OFFSET];
    const int pawnDirection = whiteTurn ? 8 : -8;
    const int pawnStartRank = whiteTurn ? 1 : 6;
    const int leftCaptureOffset = whiteTurn ? 7 : -9; 
    const int rightCaptureOffset = whiteTurn ? 9 : -7;
    const int kingsideRookSquareIndex = whiteTurn ? 7 : 63;
    const int queensideRookSquareIndex = whiteTurn ? 0 : 56;

    int originalKingSquareIndex = 0; 
    while (originalKingSquareIndex < 64)
    {
        Bitboard squareIndexBB = (Bitboard)1 << originalKingSquareIndex;
        if (squareIndexBB & friendlyPieces[KING_OFFSET])
        {
            break;
        }
        originalKingSquareIndex++;
    }
    
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
                    newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~squareIndexBB;
                    newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] |= oneSquareForwardBB;
                    newBoard.enPassant = 0;
                    if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                        bool isPromotion = (whiteTurn && rank == 6) || (!whiteTurn && rank == 1);
                        if (isPromotion) {
                            constexpr int pawnPromotionPieceOffsets[4] = {QUEEN_OFFSET, ROOK_OFFSET, BISHOP_OFFSET, KNIGHT_OFFSET};
                            for (int i = 0; i < 4; i++)
                            {
                                Board promotionBoard = newBoard;
                                promotionBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~oneSquareForwardBB;
                                promotionBoard.pieces[friendlyPieceOffset + pawnPromotionPieceOffsets[i]] |= oneSquareForwardBB;
                                std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, !whiteTurn);
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, oneSquareForwardIndex, movePerftCount);
                                }
                                #endif
                                countLeafNodes += movePerftCount;
                            }
                        } 
                        else {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
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
                                doublePushBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~squareIndexBB;
                                doublePushBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] |= twoSquaresForwardBB;
                                doublePushBoard.enPassant = twoSquaresForwardIndex;
                                if (!underThreat(doublePushBoard, originalKingSquareIndex, !whiteTurn)) {
                                    std::uint64_t movePerftCount = perftest(doublePushBoard, depth - 1, !whiteTurn);
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
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~squareIndexBB;
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] |= leftDiagBB;
                        removePiece(leftDiagIndex, newBoard, !whiteTurn);
                        newBoard.enPassant = 0;
                        if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                            bool isPromotion = (whiteTurn && rank == 6) || (!whiteTurn && rank == 1);
                            if (isPromotion) {
                                constexpr int pawnPromotionPieceOffsets[4] = {QUEEN_OFFSET, ROOK_OFFSET, BISHOP_OFFSET, KNIGHT_OFFSET};
                                for (int i = 0; i < 4; i++)
                                {
                                    Board promotionBoard = newBoard;
                                    promotionBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~leftDiagBB;
                                    promotionBoard.pieces[friendlyPieceOffset + pawnPromotionPieceOffsets[i]] |= leftDiagBB;
                                    std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, !whiteTurn);
                                    #ifdef PRINT_DIAGNOSTICS
                                    if (depth == 1) incrementCaptures();
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, leftDiagIndex, movePerftCount);
                                    }
                                    #endif
                                    countLeafNodes += movePerftCount;
                                }
                            }
                            else {
                                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == 1) incrementCaptures();
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
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~squareIndexBB;
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] |= leftDiagBB;
                        removePiece(leftIndex, newBoard, !whiteTurn);
                        newBoard.enPassant = 0; // reset en passant
                        if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                            #ifdef PRINT_DIAGNOSTICS
                            if (depth == 1) incrementEnPassant();
                            if (depth == 1) incrementCaptures();
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
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~squareIndexBB;
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] |= rightDiagBB;
                        removePiece(rightDiagIndex, newBoard, !whiteTurn);
                        newBoard.enPassant = 0;
                        if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                            bool isPromotion = (whiteTurn && rank == 6) || (!whiteTurn && rank == 1);
                            if (isPromotion) {
                                constexpr int pawnPromotionPieceOffsets[4] = {QUEEN_OFFSET, ROOK_OFFSET, BISHOP_OFFSET, KNIGHT_OFFSET};
                                for (int i = 0; i < 4; i++)
                                {
                                    Board promotionBoard = newBoard;
                                    promotionBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~rightDiagBB;
                                    promotionBoard.pieces[friendlyPieceOffset + pawnPromotionPieceOffsets[i]] |= rightDiagBB;
                                    std::uint64_t movePerftCount = perftest(promotionBoard, depth - 1, !whiteTurn);
                                    #ifdef PRINT_DIAGNOSTICS
                                    if (depth == 1) incrementCaptures();
                                    if (depth == maxDepth) {
                                        printMoveWithCount(squareIndex, rightDiagIndex, movePerftCount);
                                    }
                                    #endif
                                    countLeafNodes += movePerftCount;
                                }
                            } 
                            else {
                                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == 1) incrementCaptures();
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
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] &= ~squareIndexBB;
                        newBoard.pieces[friendlyPieceOffset + PAWN_OFFSET] |= rightDiagBB;
                        removePiece(rightIndex, newBoard, !whiteTurn);
                        newBoard.enPassant = 0; 
                        if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                            std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                            #ifdef PRINT_DIAGNOSTICS
                            if (depth == 1) incrementEnPassant();
                            if (depth == 1) incrementCaptures();
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
                    bool isCapture = false;
                    if (enemyOccupancy & newSquareBB) { 
                        removePiece(newSquareIndex, newBoard, !whiteTurn);
                        isCapture = true;
                    }
                    
                    newBoard.enPassant = 0; 
                    if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                        #ifdef PRINT_DIAGNOSTICS
                        if (isCapture && depth == 1) incrementCaptures();
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
                        // TODO: these two blocks can be combined with a break if it's a capture
                        if (enemyOccupancy & newSquareBB) {
                            removePiece(newSquareIndex, newBoard, !whiteTurn);
                            
                            newBoard.enPassant = 0; 
                            
                            if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                                if (pieceInfo.pieceIndex == ROOK_OFFSET) {
                                    if (squareIndex == kingsideRookSquareIndex) {
                                        if (whiteTurn) {
                                            newBoard.whiteKingsideCastlingRight = false;
                                        } else {
                                            newBoard.blackKingsideCastlingRight = false;
                                        }
                                    } else if (squareIndex == queensideRookSquareIndex) {
                                        if (whiteTurn) {
                                            newBoard.whiteQueensideCastlingRight = false;
                                        } else {
                                            newBoard.blackQueensideCastlingRight = false;
                                        }
                                    }
                                }
                                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                                #ifdef PRINT_DIAGNOSTICS
                                if (depth == 1) incrementCaptures();
                                if (depth == maxDepth) {
                                    printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                                }
                                #endif
                                countLeafNodes += movePerftCount;
                            }
                            break; // Stop sliding after capturing an enemy piece
                        } else {
                            newBoard.enPassant = 0;
                            if (!underThreat(newBoard, originalKingSquareIndex, !whiteTurn)) {
                                if (pieceInfo.pieceIndex == ROOK_OFFSET) {
                                    if (squareIndex == kingsideRookSquareIndex) {
                                        if (whiteTurn) {
                                            newBoard.whiteKingsideCastlingRight = false;
                                        } else {
                                            newBoard.blackKingsideCastlingRight = false;
                                        }
                                    } else if (squareIndex == queensideRookSquareIndex) {
                                        if (whiteTurn) {
                                            newBoard.whiteQueensideCastlingRight = false;
                                        } else {
                                            newBoard.blackQueensideCastlingRight = false;
                                        }
                                    }
                                }
                                std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
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

                    bool isCapture = false;
                    if (enemyOccupancy & newSquareBB) {
                        removePiece(newSquareIndex, newBoard, !whiteTurn);
                        isCapture = true;
                    }
                    if (!underThreat(newBoard, newSquareIndex, !whiteTurn)) {
                        if (whiteTurn) {
                            newBoard.whiteKingsideCastlingRight = false;
                            newBoard.whiteQueensideCastlingRight = false;
                        } else {
                            newBoard.blackKingsideCastlingRight = false;
                            newBoard.blackQueensideCastlingRight = false;
                        }
                        // TODO: find a better way to handle resetting en passant. This is error 
                        // prone because it has to be done every recursive call
                        newBoard.enPassant = 0;
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                        #ifdef PRINT_DIAGNOSTICS
                        if (depth == 1 && isCapture) incrementCaptures();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                        #endif
                        countLeafNodes += movePerftCount;
                    }
                }
            }

            bool kingsideCastlingRight = whiteTurn ? board.whiteKingsideCastlingRight : board.blackKingsideCastlingRight;
            bool queensideCastlingRight = whiteTurn ? board.whiteQueensideCastlingRight : board.blackQueensideCastlingRight;
            if (kingsideCastlingRight) {
                bool squaresVacant = (occupancy & ((Bitboard)1 << (squareIndex + 1))) == 0 &&
                                     (occupancy & ((Bitboard)1 << (squareIndex + 2))) == 0;
                if (squaresVacant) {
                    bool enemyPrevents = underThreat(board, originalKingSquareIndex, !whiteTurn)     ||
                                         underThreat(board, originalKingSquareIndex + 1, !whiteTurn) ||
                                         underThreat(board, originalKingSquareIndex + 2, !whiteTurn);
                    if (!enemyPrevents) {
                        Board newBoard = board;
                        int newSquareIndex = squareIndex + 2;
                        Bitboard kingsideRookBB = (Bitboard)1 << kingsideRookSquareIndex;
                        newBoard.pieces[friendlyPieceOffset + KING_OFFSET] &= ~squareIndexBB; // Remove king from original square
                        newBoard.pieces[friendlyPieceOffset + KING_OFFSET] |= (Bitboard)1 << (squareIndex + 2); // Place king on new square
                        newBoard.pieces[friendlyPieceOffset + ROOK_OFFSET] &= ~kingsideRookBB; // Remove rook from original square
                        newBoard.pieces[friendlyPieceOffset + ROOK_OFFSET] |= (Bitboard)1 << (squareIndex + 1); // Place rook on new square
                        if (whiteTurn) {
                            newBoard.whiteKingsideCastlingRight = false;
                            newBoard.whiteQueensideCastlingRight = false;
                        } else {
                            newBoard.blackKingsideCastlingRight = false;
                            newBoard.blackQueensideCastlingRight = false;
                        }
                        // no need to check for threat, already checked above
                        newBoard.enPassant = 0;
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                        #ifdef PRINT_DIAGNOSTICS
                        if (depth == 1) incrementCastles();
                        if (depth == maxDepth) {
                            printMoveWithCount(squareIndex, newSquareIndex, movePerftCount);
                        }
                        #endif
                        countLeafNodes += movePerftCount;
                    }
                }
            }
            if (queensideCastlingRight) {
                bool squaresVacant = (occupancy & ((Bitboard)1 << (squareIndex - 1))) == 0 &&
                                     (occupancy & ((Bitboard)1 << (squareIndex - 2))) == 0 &&
                                     (occupancy & ((Bitboard)1 << (squareIndex - 3))) == 0;
                if (squaresVacant) {
                    bool enemyPrevents = underThreat(board, originalKingSquareIndex, !whiteTurn)     ||
                                         underThreat(board, originalKingSquareIndex - 1, !whiteTurn) ||
                                         underThreat(board, originalKingSquareIndex - 2, !whiteTurn);
                    if (!enemyPrevents) {
                        Board newBoard = board;
                        int newSquareIndex = squareIndex - 2;
                        Bitboard queensideRookBB = (Bitboard)1 << queensideRookSquareIndex;
                        newBoard.pieces[friendlyPieceOffset + KING_OFFSET] &= ~squareIndexBB; // Remove king from original square
                        newBoard.pieces[friendlyPieceOffset + KING_OFFSET] |= (Bitboard)1 << (squareIndex - 2); // Place king on new square
                        newBoard.pieces[friendlyPieceOffset + ROOK_OFFSET] &= ~queensideRookBB; // Remove rook from original square
                        newBoard.pieces[friendlyPieceOffset + ROOK_OFFSET] |= (Bitboard)1 << (squareIndex - 1); // Place rook on new square
                        // no need to check for threat, already checked above
                        if (whiteTurn) {
                            newBoard.whiteKingsideCastlingRight = false;
                            newBoard.whiteQueensideCastlingRight = false;
                        } else {
                            newBoard.blackKingsideCastlingRight = false;
                            newBoard.blackQueensideCastlingRight = false;
                        }
                        newBoard.enPassant = 0;
                        std::uint64_t movePerftCount = perftest(newBoard, depth - 1, !whiteTurn);
                        #ifdef PRINT_DIAGNOSTICS
                        if (depth == 1) incrementCastles();
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

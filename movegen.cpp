#include "movegen.h"
#include <cassert>
#include <cmath>
#include "board.h"
#include "attack_bitboards.h"
#include "utilities.h"

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

// TODO: make move arrays constexpr
// TODO: change whiteTurn to color and don't hardcode 0/1 for black/white
// TODO: use Square instead of int or other integer types
std::vector<Move> GenMoves(const Board& board, Color colorToMove) {
    std::vector<Move> moves;
    const Color opponentColor = ToggleColor(colorToMove);
    const Bitboard occupancy = board.Occupancy();
    const Bitboard enemyOccupancy = board.Occupancy(opponentColor);
    const Bitboard friendlyOccupancy = board.Occupancy(colorToMove);
    const Bitboard promotionRankMask = PROMOTION_RANK_MASK[colorToMove];
    constexpr int kingsideRookStartingSquare[2] = {7, 63};
    constexpr int queensideRookStartingSquare[2] = {0, 56};

    const Bitboard king = board.Kings(colorToMove);
    Square originalKingSquareIndex = LSB(king);
    
    Bitboard pawns = board.Pawns(colorToMove);
    const int pawnDirection = colorToMove == White ? 8 : -8;
    const int pawnStartRank = colorToMove == White ? 1 : 6;

    const Bitboard singlePushes = colorToMove == White ? (pawns << 8) & ~occupancy :
                                                              (pawns >> 8) & ~occupancy;
    Bitboard promotions = singlePushes & promotionRankMask;
    Bitboard nonPromotions = singlePushes & ~promotionRankMask;
    while (promotions) {
        Square to = PopLSB(promotions);
        Square from = to - pawnDirection;
        // TODO: consider makemove/unmakemove instead of creating a whole new board
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        newBoard.enPassant = -1;
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            for (int i = 0; i < 4; i++) {
                moves.emplace_back(from, to, PieceType::Pawn, PieceType::None, promotionTypes[i], newBoard.enPassant - board.enPassant);
            }
        }
    }
    while (nonPromotions) {
        Square to = PopLSB(nonPromotions); 
        Square from = to - pawnDirection;
        Board newBoard = board;
        newBoard.enPassant = -1;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            moves.emplace_back(from, to, PieceType::Pawn, PieceType::None, PieceType::None, newBoard.enPassant - board.enPassant);
        }
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
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            moves.emplace_back(from, to, PieceType::Pawn, PieceType::None, PieceType::None, newBoard.enPassant - board.enPassant);
        }
    }

    const int leftCaptureOffset = colorToMove == White ? 7 : -9; 
    const int rightCaptureOffset = colorToMove == White ? 9 : -7;
    
    const Bitboard pawnAttackMask = board.enPassant < 0 ? enemyOccupancy : enemyOccupancy | ToBitboard(board.enPassant);
    const Bitboard leftCaptures = (colorToMove == White ? (pawns << 7) & pawnAttackMask : (pawns >> 9) & pawnAttackMask) & ~FILE_MASK[7];
    promotions = leftCaptures & promotionRankMask;
    nonPromotions = leftCaptures & ~promotionRankMask;
    while (promotions) {
        Square to = PopLSB(promotions);
        Square from = to - leftCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        PieceType removedPieceType = RemovePiece(to, newBoard, opponentColor);
        newBoard.enPassant = -1;
        Move::CastlingFlags flags = Move::CastlingFlags((board.shortCastlingRight[opponentColor] && to == kingsideRookStartingSquare[opponentColor]) *
                                         Move::RemovesOppShortCastlingRight) | 
                            Move::CastlingFlags((board.longCastlingRight[opponentColor] && to == queensideRookStartingSquare[opponentColor]) *
                                         Move::RemovesOppLongCastlingRight);
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            for (int i = 0; i < 4; i++) {
                moves.emplace_back(from, to, PieceType::Pawn, removedPieceType, promotionTypes[i], newBoard.enPassant - board.enPassant, flags);
            }
        }
    }
    while (nonPromotions) {
        Square to = PopLSB(nonPromotions);
        Square from = to - leftCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        PieceType removedPieceType;
        if (board.enPassant == to) {
            removedPieceType = RemovePiece(to - pawnDirection, newBoard, opponentColor);
        }
        else {
            removedPieceType = RemovePiece(to, newBoard, opponentColor);
        }
        newBoard.enPassant = -1;
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            moves.emplace_back(from, to, PieceType::Pawn, removedPieceType, PieceType::None, newBoard.enPassant - board.enPassant);
        }
    }

    const Bitboard rightCaptures = (colorToMove == White ? (pawns << 9) & pawnAttackMask : (pawns >> 7) & pawnAttackMask) & ~FILE_MASK[0];
    promotions = rightCaptures & promotionRankMask;
    nonPromotions = rightCaptures & ~promotionRankMask;
    while (promotions) {
        Square to = PopLSB(promotions);
        Square from = to - rightCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        PieceType removedPieceType = RemovePiece(to, newBoard, opponentColor);
        newBoard.enPassant = -1;
        Move::CastlingFlags flags = Move::CastlingFlags((board.shortCastlingRight[opponentColor] && to == kingsideRookStartingSquare[opponentColor]) *
                                         Move::RemovesOppShortCastlingRight) | 
                            Move::CastlingFlags((board.longCastlingRight[opponentColor] && to == queensideRookStartingSquare[opponentColor]) *
                                         Move::RemovesOppLongCastlingRight);
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            for (int i = 0; i < 4; i++) {
                moves.emplace_back(from, to, PieceType::Pawn, removedPieceType, promotionTypes[i], newBoard.enPassant - board.enPassant, flags);
            }
        }
    }
    while (nonPromotions) {
        Square to = PopLSB(nonPromotions);
        Square from = to - rightCaptureOffset;
        Board newBoard = board;
        newBoard.Move(PieceType::Pawn, colorToMove, from, to);
        PieceType removedPieceType;
        if (board.enPassant == to) {
            removedPieceType = RemovePiece(to - pawnDirection, newBoard, opponentColor);
        }
        else {
            removedPieceType = RemovePiece(to, newBoard, opponentColor);
        }
        newBoard.enPassant = -1;
        if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
            moves.emplace_back(from, to, PieceType::Pawn, removedPieceType, PieceType::None, newBoard.enPassant - board.enPassant);
        }
    }

    Bitboard knights = board.Knights(colorToMove);
    while (knights) {
        Square from = PopLSB(knights);
        Bitboard knightMoves = knightAttacks[from];
        while (knightMoves) {
            Square to = PopLSB(knightMoves);
            Bitboard newSquareBB = ToBitboard(to);
            if ((friendlyOccupancy & newSquareBB) == 0) { // Can't move to a square occupied by a friendly piece
                Board newBoard = board;
                newBoard.Move(PieceType::Knight, colorToMove, from, to);
                PieceType removedPieceType = PieceType::None;
                if (enemyOccupancy & newSquareBB) { 
                    removedPieceType = RemovePiece(to, newBoard, opponentColor);
                }
                newBoard.enPassant = -1; 
                if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                    Move::CastlingFlags flags = Move::CastlingFlags((board.shortCastlingRight[opponentColor] && to == kingsideRookStartingSquare[opponentColor]) *
                                                     Move::RemovesOppShortCastlingRight) | 
                                        Move::CastlingFlags((board.longCastlingRight[opponentColor] && to == queensideRookStartingSquare[opponentColor]) *
                                                     Move::RemovesOppLongCastlingRight);
                    moves.emplace_back(from, to, PieceType::Knight, removedPieceType, PieceType::None, newBoard.enPassant - board.enPassant, flags);
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
    
    for (const SlidingPiece& pieceInfo : slidingPieces) {
        Bitboard pieceBoard = board.bitboards2D[colorToMove][(int)pieceInfo.type];
        while (pieceBoard) {
            Square from = PopLSB(pieceBoard);
            Bitboard attacks = pieceInfo.Attack(from, occupancy);
            while (attacks) {
                Square to = PopLSB(attacks);
                Bitboard newSquareBB = ToBitboard(to);
                if (friendlyOccupancy & newSquareBB) continue; 
                Board newBoard = board;
                newBoard.Move(pieceInfo.type, colorToMove, from, to);
                PieceType removedPieceType = PieceType::None;
                if (enemyOccupancy & newSquareBB) {
                    removedPieceType = RemovePiece(to, newBoard, opponentColor);
                } 
                newBoard.enPassant = -1;
                if (!underThreat(newBoard, originalKingSquareIndex, opponentColor)) {
                    bool kingsideRookCondition = board.shortCastlingRight[colorToMove] && pieceInfo.type == PieceType::Rook && 
                                                 from == kingsideRookStartingSquare[colorToMove];
                    bool queensideRookCondition = board.longCastlingRight[colorToMove] && pieceInfo.type == PieceType::Rook && 
                                                  from == queensideRookStartingSquare[colorToMove];
                    Move::CastlingFlags flags = Move::CastlingFlags((board.shortCastlingRight[opponentColor] && to == kingsideRookStartingSquare[opponentColor]) *
                                                     Move::RemovesOppShortCastlingRight) | 
                                        Move::CastlingFlags((board.longCastlingRight[opponentColor] && to == queensideRookStartingSquare[opponentColor]) *
                                                     Move::RemovesOppLongCastlingRight) |
                                        Move::CastlingFlags(kingsideRookCondition * Move::RemovesShortCastlingRight) |
                                        Move::CastlingFlags(queensideRookCondition * Move::RemovesLongCastlingRight);
                    moves.emplace_back(from, to, pieceInfo.type, removedPieceType, PieceType::None, newBoard.enPassant - board.enPassant, flags);
                }
            }
        }
    }

    Bitboard kingMoves = kingAttacks[originalKingSquareIndex];
    while (kingMoves) {
        Square to = PopLSB(kingMoves);
        Bitboard newSquareBB = ToBitboard(to);
        if ((friendlyOccupancy & newSquareBB) == 0) {
            Board newBoard = board;
            newBoard.Move(PieceType::King, colorToMove, originalKingSquareIndex, to);

            PieceType removedPieceType = PieceType::None;
            if (enemyOccupancy & newSquareBB) {
                removedPieceType = RemovePiece(to, newBoard, opponentColor);
            }
            if (!underThreat(newBoard, to, opponentColor)) {
                // TODO: find a better way to handle resetting en passant. This is error 
                // prone because it has to be done every recursive call
                Move::CastlingFlags flags = Move::CastlingFlags((board.shortCastlingRight[opponentColor] && to == kingsideRookStartingSquare[opponentColor]) *
                                                 Move::RemovesOppShortCastlingRight) | 
                                    Move::CastlingFlags((board.longCastlingRight[opponentColor] && to == queensideRookStartingSquare[opponentColor]) *
                                                 Move::RemovesOppLongCastlingRight) |
                                    Move::CastlingFlags(board.shortCastlingRight[colorToMove] * Move::RemovesShortCastlingRight) |
                                    Move::CastlingFlags(board.longCastlingRight[colorToMove] * Move::RemovesLongCastlingRight);
                newBoard.enPassant = -1;
                moves.emplace_back(originalKingSquareIndex, to, PieceType::King, removedPieceType, PieceType::None, newBoard.enPassant - board.enPassant, flags);
            }
        }
    }

    if (board.shortCastlingRight[colorToMove]) {
        bool squaresVacant = (occupancy & ((Bitboard)1 << (originalKingSquareIndex + 1))) == 0 &&
                             (occupancy & ((Bitboard)1 << (originalKingSquareIndex + 2))) == 0;
        if (squaresVacant) {
            bool enemyPrevents = underThreat(board, originalKingSquareIndex, opponentColor)     ||
                                 underThreat(board, originalKingSquareIndex + 1, opponentColor) ||
                                 underThreat(board, originalKingSquareIndex + 2, opponentColor);
            if (!enemyPrevents) {
                Square to = originalKingSquareIndex + 2;
                // no need to check for threat, already checked above
                Move::CastlingFlags flags = Move::CastlingFlags(board.shortCastlingRight[colorToMove] * Move::RemovesShortCastlingRight) |
                                    Move::CastlingFlags(board.longCastlingRight[colorToMove] * Move::RemovesLongCastlingRight);
                moves.emplace_back(originalKingSquareIndex, to, PieceType::King, PieceType::None, PieceType::None, -1 - board.enPassant, flags);
            }
        }
    }
    if (board.longCastlingRight[colorToMove]) {
        bool squaresVacant = (occupancy & ((Bitboard)1 << (originalKingSquareIndex - 1))) == 0 &&
                             (occupancy & ((Bitboard)1 << (originalKingSquareIndex - 2))) == 0 &&
                             (occupancy & ((Bitboard)1 << (originalKingSquareIndex - 3))) == 0;
        if (squaresVacant) {
            bool enemyPrevents = underThreat(board, originalKingSquareIndex, opponentColor)     ||
                                 underThreat(board, originalKingSquareIndex - 1, opponentColor) ||
                                 underThreat(board, originalKingSquareIndex - 2, opponentColor);
            if (!enemyPrevents) {
                Square to = originalKingSquareIndex - 2;
                // no need to check for threat, already checked above
                Move::CastlingFlags flags = Move::CastlingFlags(board.shortCastlingRight[colorToMove] * Move::RemovesShortCastlingRight) |
                                    Move::CastlingFlags(board.longCastlingRight[colorToMove] * Move::RemovesLongCastlingRight);
                moves.emplace_back(originalKingSquareIndex, to, PieceType::King, PieceType::None, PieceType::None, -1 - board.enPassant, flags);
            }
        }
    }
    
    return moves;
}

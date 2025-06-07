#include "fen.h"
#include "board.h"
#include "utilities.h"
#include <stdexcept>

Fen ParseFen(const std::string& fenStr) {
    Fen fen;
    fen.board = Board{false};
    Board& board = fen.board;
    int rank = 7;
    int file = 0;
    int fenIdx = 0;
    while (rank >= 0)
    {
        bool incrementFile = true;
        int squareIndex = rank * 8 + file;
        Bitboard squareIndexBB = (Bitboard)1 << squareIndex;
        char c = fenStr[fenIdx++];
        switch (c)
        {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        {
            int emptySquares = c - '0';
            file += emptySquares;
            incrementFile = false;
            break;
        }
        case 'P':
            board.whitePawns |= squareIndexBB;
            break;
        case 'N':
            board.whiteKnights |= squareIndexBB;
            break;
        case 'B':
            board.whiteBishops |= squareIndexBB;
            break;
        case 'R':
            board.whiteRooks |= squareIndexBB;
            break;
        case 'Q':
            board.whiteQueens |= squareIndexBB;
            break;
        case 'K':
            board.whiteKing |= squareIndexBB;
            break;
        case 'p':
            board.blackPawns |= squareIndexBB;
            break;
        case 'n':
            board.blackKnights |= squareIndexBB;
            break;
        case 'b':
            board.blackBishops |= squareIndexBB;
            break;
        case 'r':
            board.blackRooks |= squareIndexBB;
            break;
        case 'q':
            board.blackQueens |= squareIndexBB;
            break;
        case 'k':
            board.blackKing |= squareIndexBB;
            break;
        case '/':
            incrementFile = false;
            break;
        default:
            throw std::invalid_argument("Invalid FEN character");
        }
        if (incrementFile) {
            file++;
        }
        if (file > 7) {
            rank--;
            file = 0;
        }
    }

    if (fenStr[fenIdx] == ' ') {
        fenIdx++;
    } else {
        throw std::invalid_argument("Invalid FEN format");  
    }

    if (fenStr[fenIdx] == 'w') {
        fen.colorToMove = Color::White;
    } else if (fenStr[fenIdx] == 'b') {
        fen.colorToMove = Color::Black;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }
    fenIdx++;

    if (fenStr[fenIdx] == ' ') {
        fenIdx++;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }

    board.whiteKingsideCastlingRight = false;
    board.whiteQueensideCastlingRight = false;
    board.blackKingsideCastlingRight = false;
    board.blackQueensideCastlingRight = false;
    if (fenStr[fenIdx] == '-') {
        fenIdx++;
    } else {
        while (fenStr[fenIdx] != ' ') {
            if (fenStr[fenIdx] == 'K') {
                board.whiteKingsideCastlingRight = true;
                fenIdx++;
            } else if (fenStr[fenIdx] == 'Q') {
                board.whiteQueensideCastlingRight = true;
                fenIdx++;
            } else if (fenStr[fenIdx] == 'k') {
                board.blackKingsideCastlingRight = true;
                fenIdx++;
            } else if (fenStr[fenIdx] == 'q') {
                board.blackQueensideCastlingRight = true;
                fenIdx++;
            } else {
                throw std::invalid_argument("Invalid FEN format");
            }        
        }
    } 
    if (fenStr[fenIdx] == ' ') {
        fenIdx++;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }

    if (fenStr[fenIdx] == '-') {
        board.enPassant = 0;
        fenIdx++;
    } else {
        int file = fenStr[fenIdx] - 'a';
        int rank = fenStr[fenIdx + 1] - '1';
        // My convention is to use the destination square for en passant, not the potential capture square as FEN notation does. This converts
        // the FEN notation to my convention. example: 1. e4 sets en passant square to e4, not e3 like fen.
        if (fen.colorToMove == Color::White) {
            rank--;
        } else {
            rank++;
        }
        if (file < 0 || file > 7 || rank < 0 || rank > 7) {
            throw std::invalid_argument("Invalid FEN format");
        }
        board.enPassant = rank * 8 + file;
        fenIdx += 2;
    }

    if (fenStr[fenIdx] == ' ') {
        fenIdx++;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }
    
    fen.halfmoveClock = fenStr[fenIdx++] - '0';
    if (fenStr[fenIdx] >= '0' && fenStr[fenIdx] <= '9') {
        fen.halfmoveClock = fen.halfmoveClock * 10 + (fenStr[fenIdx++] - '0');
    }
    if (fenStr[fenIdx] == ' ') {
        fenIdx++;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }

    fen.fullmoveNumber = fenStr[fenIdx++] - '0';
    if (fenStr[fenIdx] >= '0' && fenStr[fenIdx] <= '9') {
        fen.fullmoveNumber = fen.fullmoveNumber * 10 + (fenStr[fenIdx++] - '0');
    }

    return fen;
}

std::string ToFen(const Fen& fen) {
    std::string fenStr;
    char nextChar = '0';
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int squareIndex = rank * 8 + file;
            Piece piece = PieceAt(squareIndex, fen.board);
            if (piece.type == PieceType::None) {
                if (!fenStr.empty() && std::isdigit(fenStr.back())) {
                    fenStr.back()++;
                } else {
                    fenStr += '1';
                }
            } else {
                switch (piece.type) {
                    case PieceType::Pawn:
                        fenStr += piece.color == White ? 'P' : 'p';
                        break;
                    case PieceType::Knight:
                        fenStr += piece.color == White ? 'N' : 'n';
                        break;
                    case PieceType::Bishop:
                        fenStr += piece.color == White ? 'B' : 'b';
                        break;
                    case PieceType::Rook:
                        fenStr += piece.color == White ? 'R' : 'r';
                        break;
                    case PieceType::Queen:
                        fenStr += piece.color == White ? 'Q' : 'q';
                        break;
                    case PieceType::King:   
                        fenStr += piece.color == White ? 'K' : 'k';
                        break;
                    default:
                        throw std::invalid_argument("Invalid piece type");
                }
            }
        } 
        fenStr += '/';
    }

    fenStr.pop_back(); // Remove the last '/'
    fenStr += ' ';
    fenStr += fen.colorToMove == Color::White ? 'w' : 'b';
    fenStr += ' ';
    if (fen.board.whiteKingsideCastlingRight) {
        fenStr += 'K';
    }
    if (fen.board.whiteQueensideCastlingRight) {
        fenStr += 'Q';
    }
    if (fen.board.blackKingsideCastlingRight) {
        fenStr += 'k';
    }
    if (fen.board.blackQueensideCastlingRight) {
        fenStr += 'q';
    }
    if (fenStr.back() == ' ') {
        fenStr += '-';
    } 
    fenStr += ' ';
    if (fen.board.enPassant == 0) {
        fenStr += '-';
    } else {
        int file = fen.board.enPassant % 8;
        int rank = fen.board.enPassant / 8;
        if (fen.colorToMove == Color::White) {
            rank++;
        } else {
            rank--;
        }
        fenStr += 'a' + file;
        fenStr += '1' + rank;
    }
    fenStr += ' ';
    fenStr += std::to_string(fen.halfmoveClock) + ' ';
    fenStr += std::to_string(fen.fullmoveNumber);
    return fenStr;
}

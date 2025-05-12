#include "fen.h"
#include "utilities.h"

Fen parseFEN(const std::string& fenStr) {
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
        fen.whiteTurn = true;
    } else if (fenStr[fenIdx] == 'b') {
        fen.whiteTurn = false;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }
    fenIdx++;

    if (fenStr[fenIdx] == ' ') {
        fenIdx++;
    } else {
        throw std::invalid_argument("Invalid FEN format");
    }

    if (fenStr[fenIdx] == '-') {
        board.whiteKingsideCastlingRight = false;
        board.whiteQueensideCastlingRight = false;
        board.blackKingsideCastlingRight = false;
        board.blackQueensideCastlingRight = false;
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
        if (fen.whiteTurn) {
            rank--;
        } else {
            rank++;
        }
        if (file < 0 || file > 7 || rank < 0 || rank > 7) {
            throw std::invalid_argument("Invalid FEN format");
        }
        board.enPassant = (Bitboard)1 << (rank * 8 + file);
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
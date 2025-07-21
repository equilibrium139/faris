#include "uci.h"
#include "board.h"
#include "fen.h"
#include "movegen.h"
#include "search.h"
#include "transposition.h"
#include "utilities.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string MoveToUCINotation(const Move& move) {
    std::string uciMove;
    uciMove += (move.from % 8) + 'a';
    uciMove += (move.from / 8) + '1';
    uciMove += (move.to % 8) + 'a';
    uciMove += (move.to / 8) + '1';
    switch (move.promotionType) {
        case PieceType::Queen:
            uciMove += 'q';
            break;
        case PieceType::Rook:
            uciMove += 'r';
            break;
        case PieceType::Bishop:
            uciMove += 'b';
            break;
        case PieceType::Knight:
            uciMove += 'n';
            break;
        default:
            break;
    }
    return uciMove;
}

void ProcessInput() {
    UCIState state;
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss{line};
        std::string token;
        ss >> token;
        if (token == "position") {
            ss >> token;
            state.colorToMove = White;
            if (token == "fen") {
                std::string fenString;
                while (ss >> token && token != "moves") {
                    fenString += token + ' ';
                }
                Fen fen = ParseFen(fenString);
                state.board = fen.board;
                state.colorToMove = fen.colorToMove;
            }
            else {
                state.board = Board();
                ss >> token; // skip "startingpos" token
            }
            if (token == "moves") {
                while (ss >> token) {
                    std::vector<Move> moves = GenMoves(state.board, state.colorToMove);
                    for (const Move& move : moves) {
                        if (MoveToUCINotation(move) == token) {
                            MakeMove(move, state.board, state.colorToMove);
                            state.colorToMove = ToggleColor(state.colorToMove);
                            break;
                        }
                    }
                }
            }
        }
        else if (token == "go") {
            ss >> token;
            assert(token == "wtime");
            ss >> state.wtime;
            ss >> token;
            assert(token == "btime");
            ss >> state.btime;
            ss >> token;
            assert(token == "winc");
            ss >> state.winc;
            ss >> token;
            assert(token == "binc");
            ss >> state.binc;
            std::cerr << "Computing...\n";
            int time, inc;
            if (state.colorToMove == White) {
                time = state.wtime;
                inc = state.winc;
            }
            else {
                time = state.btime;
                inc = state.binc;
            }
            Move move = Search(state.board, state.colorToMove, time, inc);
            // TODO: implement ponder
            std::cout << "bestmove " << MoveToUCINotation(move) << std::endl;
            std::cerr << transpositionTable.hits << std::endl;
        }
        else if (token == "uci") {
            std::cout << "id name Faris\n\tid Author Zaid Al-ruwaishan\n\tuciok" << std::endl;
        }
        else if (token == "ucinewgame") {
            // Not much to do here at this point...
        }
        else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if (token == "quit") {
            return;
        }
    }
}

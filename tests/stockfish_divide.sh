#! /bin/bash
FEN="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH=3
echo -e "position fen $FEN\ngo perft $DEPTH\nquit" | stockfish | grep '^[a-h][1-8][a-h][1-8]' | sort

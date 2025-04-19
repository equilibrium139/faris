#include <iostream>
#include "board.h"

int main() {
    Board board;
    // print binary representation of the board   
    std::cout << std::hex << board.allPieces() << std::endl;
    return 0; 
}

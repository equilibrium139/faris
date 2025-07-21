#include "board.h"
#include <cassert>
#include "movegen.h"
#include <iostream>
#include "uci.h"

int maxDepth;

int main(int argc, char** argv) {
    maxDepth = 10;
    ProcessInput();
    return 0;
}

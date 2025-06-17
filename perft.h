#pragma once

#include "board.h"

std::uint64_t perftest(Board& board, int depth, Color colorToMove, bool enablePerftDiagnostics = false);

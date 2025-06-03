#include "board.h"
#include <vector>
#include <string>

std::string ComputeStockfishPerftDivide(const std::string& fenString, int depth);
std::string ComputeFarisPerftDivide(const Board& board, int depth, Color colorToMove);

#include "board.h"
#include <string>
#include <vector> 

std::vector<std::string> ComputeStockfishPerftDivide(const std::string& fenString, int depth);
std::vector<std::string> ComputeFarisPerftDivide(const Board& board, int depth, Color colorToMove);
void PrintFirstIncorrectMoveChain(const std::string& fenString, int depth, const Board& board, Color colorToMove);

#include "fen.h"
#include <vector>

struct PerftTest {
    Fen fen;
    std::vector<std::uint64_t> nodeCounts;
};

std::vector<PerftTest> LoadPerftTests(const char* filename);

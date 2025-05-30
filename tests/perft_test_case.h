#include "fen.h"
#include <cstdint>
#include <utilities.h>
#include <vector>

struct PerftTest {
    struct Result {
        int depth;
        std::uint64_t nodeCount;
    };
    Fen fen;
    std::vector<Result> nodeCounts;
};

std::vector<PerftTest> LoadPerftTests(const char* filename);

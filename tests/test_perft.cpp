#include "gtest/gtest.h"
#include "movegen.h"
#include "perft_test_case.h"

// TODO: handle maxDepth in a better way
int maxDepth;

class PerftTestFixture : public ::testing::TestWithParam<PerftTest> {
};

TEST_P(PerftTestFixture, VerifyNodeCounts) {
    const PerftTest& testCase = GetParam();
    for (const PerftTest::Result& result : testCase.nodeCounts) {
        maxDepth = result.depth;
        std::uint64_t nodeCount = perftest(testCase.fen.board, result.depth, testCase.fen.colorToMove);
        ASSERT_EQ(result.nodeCount, nodeCount);
    }
}

INSTANTIATE_TEST_SUITE_P(PerftTestsFromFile, PerftTestFixture, ::testing::ValuesIn(LoadPerftTests("perft_test_data.txt")));

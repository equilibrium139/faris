#include "gtest/gtest.h"
#include "board.h"
#include "perft_test_case.h"

int maxDepth = 3;

class PerftTestFixture : public ::testing::TestWithParam<PerftTest> {
    protected:
        Board board;
};

TEST_P(PerftTestFixture, VerifyNodeCounts) {
    const PerftTest& testCase = GetParam();
    ASSERT_TRUE(testCase.fen.board.Valid());
}

INSTANTIATE_TEST_SUITE_P(PerftTestsFromFile, PerftTestFixture, ::testing::ValuesIn(LoadPerftTests("perft_test_data.txt")));

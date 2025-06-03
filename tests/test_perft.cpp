#include "gtest/gtest.h"
#include "movegen.h"
#include "perft_test_case.h"
#include "perft_divide.h"
#include <iostream>

// TODO: handle maxDepth in a better way
int maxDepth;

class PerftTestFixture : public ::testing::TestWithParam<PerftTest> {
};

TEST_P(PerftTestFixture, VerifyNodeCounts) {
    const PerftTest& testCase = GetParam();
    std::string fenString = ToFen(testCase.fen);
    std::cerr << "[ FEN      ]: " << fenString << std::endl; 
    for (const PerftTest::Result& result : testCase.nodeCounts) {
        maxDepth = result.depth;
        std::uint64_t nodeCount = perftest(testCase.fen.board, result.depth, testCase.fen.colorToMove);
        if (result.nodeCount != nodeCount) {

            // const std::vector<std::string>& farisPerftDivide = perftDivide;

            /* for (const std::string& s : farisPerftDivide) {
                std::cerr << s << std::endl;
            }
            */
            

            std::cerr << "\n-------- DIAGNOSTICS for FAILED Perft --------\n"
                      << "FEN: " << fenString << "\n"
                      << "Depth: " << result.depth
                      << ", Expected: " << result.nodeCount
                      << ", Got: " << nodeCount << "\n"
                      << "Stockfish divide output:" << std::endl; 
            std::string stockfishPerftDivide = ComputeStockfishPerftDivide(fenString, result.depth); 
            std::cerr << stockfishPerftDivide << std::endl;
            std::string farisPerftDivide = ComputeFarisPerftDivide(testCase.fen.board, result.depth, testCase.fen.colorToMove);
            std::cerr << "Faris divide output:\n" << farisPerftDivide << std::endl;
            std::cerr << "---------------------------------------------\n" << std::endl;
            GTEST_FAIL() 
                << "Perft mismatch for FEN: " << fenString
                << " at Depth: " << result.depth;
        } 
    }
}

INSTANTIATE_TEST_SUITE_P(PerftTestsFromFile, PerftTestFixture, ::testing::ValuesIn(LoadPerftTests("perft_test_data.txt")));

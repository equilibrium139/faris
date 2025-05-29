#include "perft_test_case.h"
#include <string>
#include <fstream>

std::vector<PerftTest> LoadPerftTests(const char* filename) {
    std::ifstream ifs(filename);
    std::vector<PerftTest> tests;
    bool parsingFen = true;
    std::string line;

    while (std::getline(ifs, line)) {
        if (parsingFen) {
            tests.emplace_back();
            tests.back().fen = ParseFen(line);
            parsingFen = false;
        }
        else {
            if (line.empty()) {
                parsingFen = true;
            }
            else {
                tests.back().nodeCounts.push_back(std::stoull(line));
            }
        }
    }

    return tests;
}

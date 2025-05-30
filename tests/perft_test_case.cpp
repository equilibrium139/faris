#include "perft_test_case.h"
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>

std::vector<PerftTest> LoadPerftTests(const char* filename) {
    std::ifstream ifs(filename);
    std::vector<PerftTest> tests;
    bool parsingFen = true;
    std::string line;
    int depth = 0;

    while (std::getline(ifs, line)) {
        if (parsingFen) {
            tests.emplace_back();
            tests.back().fen = ParseFen(line);
            parsingFen = false;
        }
        else {
            if (line.empty()) {
                parsingFen = true;
                depth = 0;
            }
            else {
                // depth is either specified explicitly prior to node count or assumed to be one greater than the depth
                // of the previous line
                std::uint64_t nodeCount;
                auto spaceIdx = line.find(' ');
                bool lineHasDepth = spaceIdx != std::string::npos && (spaceIdx + 1) < line.size() && std::isdigit(line[spaceIdx + 1]);
                if (lineHasDepth) {
                    std::stringstream ss(line);
                    ss >> depth >> nodeCount;
                } 
                else {
                    nodeCount = std::stoull(line);
                }

                tests.back().nodeCounts.push_back({depth, nodeCount});
                depth++;
            }
        }
    }

    return tests;
}

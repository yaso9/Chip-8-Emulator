#pragma once

#include <fstream>

#include "./Program.hpp"

class Programs
{
public:
    std::vector<Program> programs{};

    Programs(std::string filename)
    {
        // Read the programs from a file
        std::ifstream stream(filename);
        std::string line;
        while (getline(stream, line))
        {
            size_t separator_idx = line.find("-=-=-=-");
            programs.emplace_back(line.substr(0, separator_idx), line.substr(separator_idx + 7));
        }
    }
};
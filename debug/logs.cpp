#include "logs.h"

#include <array>
#include <iostream>

using namespace oops_bcode_compiler::debug;

const std::array<std::string, 4> levels = {"DEBUG", "INFO", "WARNING", "ERROR"};

void logging::log(logging::level lvl, const std::string &msg)
{
    if (lvl >= this->output_level)
    {
        if (lvl == level::error)
        {
            std::cerr << levels[static_cast<unsigned>(lvl)] << ": " << msg << "\n";
        }
        else
        {
            std::cout << levels[static_cast<unsigned>(lvl)] << ": " << msg << std::endl;
        }
    }
}
void logging::log(logging::level lvl, const char *msg)
{
    if (lvl >= this->output_level)
    {
        if (lvl == level::error)
        {
            std::cerr << levels[static_cast<unsigned>(lvl)] << ": " << msg << "\n";
        }
        else
        {
            std::cout << levels[static_cast<unsigned>(lvl)] << ": " << msg << std::endl;
        }
    }
}
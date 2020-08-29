#include "logs.h"

#include <iostream>

using namespace oops_bcode_compiler::debug;

void logger::log(logger::level lvl, const std::string &msg)
{
    if (lvl >= this->output_level)
    {
        if (lvl == level::error)
        {
            std::cerr << msg << "\n";
        }
        else
        {
            std::cout << msg << std::endl;
        }
    }
}
void logger::log(logger::level lvl, const char *msg)
{
    if (lvl >= this->output_level)
    {
        if (lvl == level::error)
        {
            std::cerr << msg << "\n";
        }
        else
        {
            std::cout << msg << std::endl;
        }
    }
}
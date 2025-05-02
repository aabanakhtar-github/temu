
#include <iostream>

#include "util.h"

[[noreturn]] int repl()
{
    while (true)
    {
        std::cout << "temu $ ";
        std::string input;
        std::getline(std::cin, input);
        for (auto args = splitString(input, ' '); auto &arg : args)
        {
            std::cout << arg << std::endl;
        }
    }
}

int main()
{
    return repl();
}
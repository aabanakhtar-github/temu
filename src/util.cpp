//
// Created by aabanakhtar on 5/1/25.
//

#include "util.h"
#include <sstream>

std::vector<std::string> splitString(const std::string& s, char delim)
{
    std::istringstream ss(s);
    std::string arg;
    std::vector<std::string> args;

    while (std::getline(ss, arg, delim))
    {
        args.push_back(arg);
    }

    return args;
}

std::vector<char*> makeCArgs(const std::vector<std::string> &args)
{
    std::vector<char*> cargs;
    cargs.reserve(args.size());
    for (auto& arg : args)
    {
        cargs.push_back(const_cast<char*>(arg.c_str()));
    }
    // required by execvp
    cargs.push_back(nullptr);
    return cargs;
}

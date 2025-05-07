// Created by aabanakhtar on 5/1/25.
//
#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

std::vector<std::string> splitString(const std::string& s, char delim);
std::vector<char*> makeCArgs(const std::vector<std::string>& args);
#endif //UTIL_H

//
// Created by aabanakhtar on 5/17/25.
//

#ifndef TEMU_APPLICATION_H
#define TEMU_APPLICATION_H

#include <string>
#include <vector>

// two types of evals
void evalWithPTY(const std::string& input);
void evalPiped(const std::vector<std::string>& input);
// main loop
int repl();

#endif //TEMU_APPLICATION_H

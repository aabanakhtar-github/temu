#include <iostream>
#include <ranges>
#include <unistd.h>
#include <sys/wait.h>
#include "util.h"

enum OutputLevel
{
    ERROR,
    INFO,
    LOG
};

std::vector<char*> makeCArgs(std::vector<std::string> &args)
{
    std::vector<char*> cargs;
    cargs.reserve(args.size());
    for (auto& arg : args)
    {
        cargs.push_back(const_cast<char*>(arg.c_str()));
    }
    return cargs;
}

void eval(const std::string& input)
{
    pid_t pid = fork();
    auto args = splitString(input, ' ');
    args.resize(args.size() + 1);

    if (pid == -1)
    {
        std::cerr << "Failed to fork" << std::endl;
        return;
    }
    else if (pid == 0)
    {
        // execute the child process
        execvp(args[0].c_str(), makeCArgs(args).data());
        perror("execvp");
        std::cerr << "Failed to exec" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        int status;
        wait(&status);
        std::cout << "Finished process with status code: " << status << std::endl;
    }
}

[[noreturn]] int repl()
{
    std::cout << "temu % ";
    for (;;)
    {
        std::string input;
        std::getline(std::cin, input);
        eval(input);
        std::cout << "temu % ";
    }
}

int main()
{
    return repl();
}

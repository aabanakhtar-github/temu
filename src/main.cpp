#include <iostream>
#include <ranges>
#include <unistd.h>
#include <sys/wait.h>
#include "unix_wrappers.h"
#include "util.h"

void eval(const std::string& input)
{
    Pipe stdout_redirect, stdin_redirect;
    if (!stdout_redirect && stdin_redirect)
    {
        std::cout << "Failed to create stdout/stdin pipes." << std::endl;
    }

    CArgv argv(splitString(input, ' '));
    if (argv.size() == 0)
    {
        return;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        return;
    }
    else if (pid == 0)
    {
        reroute(stdout_redirect.write, {STDOUT_FILENO, STDERR_FILENO});
        reroute(stdin_redirect.read, {STDIN_FILENO});

        execvp(argv.asCArgv()[0], argv.asCArgv());
        perror("execvp");
        std::cerr << "Failed to exec" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // handle input
        std::string test;
        std::getline(std::cin, test);
        write(stdin_redirect.write.fd, test.c_str(), test.size());

        char buffer[4096];
        ssize_t sz;
        while ((sz = read(stdout_redirect.read.fd, buffer, 4096)) > 0)
        {
            buffer[sz] = '\0';
            std::cout << buffer << std::flush;
        }

        // wait for process to exit
        int status;
        wait(&status);

        if (WIFEXITED(status))
        {
            const int code = WEXITSTATUS(status);
            std::cout << "\nExited with status: " << code << std::endl;
        }
        else if (WIFSIGNALED(status))
        {
            std::cout << "\nKilled by signal: " << WTERMSIG(status) << std::endl;
        }
    }
}

[[noreturn]] int repl()
{
    for (;;)
    {
        std::cout << "temu % ";
        std::string input;
        std::getline(std::cin, input);
        eval(input);
    }
}

int main()
{
    return repl();
}

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
    // required by execvp
    cargs.push_back(nullptr);
    return cargs;
}

constexpr std::size_t PIPE_READ = 0;
constexpr std::size_t PIPE_WRITE = 1;

void execChildProcess()
{

}

void execParentProcess()
{

}

void eval(const std::string& input)
{
    int stdout_pipe_fd[2];
    // making the pipe failed
    if (pipe(stdout_pipe_fd) == -1)
    {
        // unix way of dealing with systemwide errors
        perror("pipe");
        return;
    }

    auto args = splitString(input, ' ');
    if (args.empty())
    {
        return;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        std::cerr << "Failed to fork" << std::endl;
        // release the resources
        close(stdout_pipe_fd[PIPE_WRITE]);
        close(stdout_pipe_fd[PIPE_READ]);
        return;
    }
    else if (pid == 0)
    {
        // child process
        // disable reading (we don't need), enable writing from stdout, and then close the pipe.
        close(stdout_pipe_fd[PIPE_READ]);
        dup2(stdout_pipe_fd[PIPE_WRITE], STDOUT_FILENO);
        dup2(stdout_pipe_fd[PIPE_WRITE], STDERR_FILENO);
        // accessible through STDOUT_FILENO
        close(stdout_pipe_fd[PIPE_WRITE]);

        execvp(args[0].c_str(), makeCArgs(args).data());
        perror("execvp");
        std::cerr << "Failed to exec" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process
        // release fd, not needed
        close(stdout_pipe_fd[PIPE_WRITE]);

        char buffer[4096];
        ssize_t sz;
        while ((sz = read(stdout_pipe_fd[PIPE_READ], buffer, 4096)) > 0)
        {
            buffer[sz] = '\0';
            std::cout << buffer << std::flush;
        }

        close(stdout_pipe_fd[PIPE_READ]);

        // wait for process to exit
        int status;
        wait(&status);


        if (WIFEXITED(status))
        {
            int code = WEXITSTATUS(status);
            std::cout << "Exited with status: " << code << std::endl;
        }
        else if (WIFSIGNALED(status))
        {
            std::cout << "Killed by signal: " << WTERMSIG(status) << std::endl;
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

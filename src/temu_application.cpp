//
// Created by aabanakhtar on 5/17/25.
//

#include "temu_application.h"
#include <iostream>
#include <memory>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pty.h>
#include "syscall.h"
#include "temu_core.h"
#include "pipe.h"
#include "util.h"

void child(const int master_fd, const std::string& input)
{
    temu::TerminalState state;
    close(master_fd);
    temu::runChildProcess(input);
}

void parent(const int master_fd, const int proc_ID)
{
    temu::Piper piper(master_fd);
    int status;
    for (;;)
    {
        if (const int pid = SYSCALL(waitpid(proc_ID, &status, WNOHANG)); pid == proc_ID)
        {
            break;
        }

        if (piper.pipeInputs())
        {
            break;
        }
    }

    SYSCALL(close(master_fd));
    if (WIFEXITED(status))
    {
        std::cout << "Exited with code: " << WEXITSTATUS(status) << std::endl;
    }
    else if (WIFSIGNALED(status))
    {
        std::cout << "Killed by signal: " << WTERMSIG(status) << std::endl;
    }
    else if (WIFSTOPPED(status))
    {
        std::cout << "Stopped by signal: " << WSTOPSIG(status) << std::endl;
    }
}

void evalWithPTY(const std::string& input)
{
    int master_fd;
    int proc_ID = forkpty(&master_fd, nullptr, nullptr, nullptr);

    if (proc_ID == -1)
    {
        perror("forkpty");
        exit(EXIT_FAILURE);
    }
    else if (proc_ID == 0)
    {
        child(master_fd, input);
    }
    else
    {
        parent(master_fd, proc_ID);
    }
}

void evalPiped(const std::vector<std::string>& input)
{
    int previous_read = -1;
    std::vector<pid_t> pids;

    for (std::size_t i = 0; i < input.size(); ++i)
    {
        int pipefd[2];

        if (i != input.size() - 1 && SYSCALL(pipe(pipefd)) < 0)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = SYSCALL(fork());
        if (pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            if (previous_read != -1)
            {
                redirect(previous_read, {STDIN_FILENO});
                SYSCALL(close(previous_read));
            }

            if (i != input.size() - 1)
            {
                SYSCALL(close(pipefd[PIPE_READ]));
                redirect(pipefd[PIPE_WRITE], {STDOUT_FILENO});
                SYSCALL(close(pipefd[PIPE_WRITE]));
            }

            temu::runChildProcess(input[i]);
        }
        else
        {
            pids.push_back(pid);
            if (previous_read != -1)
            {
                SYSCALL(close(previous_read));
            }

            if (i != input.size() - 1)
            {
                close(pipefd[PIPE_WRITE]);
                previous_read = pipefd[0];
            }
        }
    }

    for (const pid_t pid : pids)
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

int repl()
{
    for (;;)
    {
        bool should_exit = false, should_clear = false;
        char buf[PATH_MAX];
        if (getcwd(buf, PATH_MAX) == nullptr)
        {
            std::cout << "Couldn't determine current directory." << std::endl;
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        std::string prompt = (std::string("temu [") + buf + "] % ");
        char* input = readline(prompt.c_str());
        if (input == nullptr)
        {
            continue;
        }

        add_history(input);
        auto free_str = [](char* str) { free(str); };
        std::unique_ptr<char, decltype(free_str)> input_deleter(input, free_str);

        if (auto redirections = splitString(input, '|'); redirections.size() > 1)
        {
            evalPiped(redirections);
        }
        else if (!temu::interceptBuiltins(input, should_exit, should_clear))
        {
            evalWithPTY(input);
        }

        if (should_exit)
        {
            std::cout << "exiting" << std::endl;
            return 0;
        }

        if (should_clear)
        {
            SYSCALL(system("clear"));
        }
    }
}

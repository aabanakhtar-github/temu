#include <iostream>
#include <ranges>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <pty.h>
#include <fcntl.h>
#include "pipe.h"
#include "syscall.h"
#include "temu_core.h"
#include "util.h"
#include <cstdio>
#include <memory>
#include <readline/readline.h>
#include <readline/history.h>


void child(const int master_fd, const std::string& input)
{
    // establish the terminal state
    temu::TerminalState state;
    // child process
    // we don't need the master fd, handle directly with stdin stdout
    close(master_fd);
    temu::runChildProcess(input);
}

void parent(const int master_fd, const int proc_ID)
{
    // parent process
    temu::Piper piper(master_fd);
    for (;;)
    {
        int status;
        // see if the proc is done
        if (const int pid = SYSCALL(waitpid(proc_ID, &status, WNOHANG)); pid == proc_ID)
        {
            break;
        }

        // reroute stdin/out
        if (piper.pipeInputs())
        {
            break;
        }
    }

    SYSCALL(close(master_fd));

    int status;
    wait(&status);

    if (WIFEXITED(status))
    {
        const int code = WEXITSTATUS(status);
        std::cout << "Exited with code: " << code << std::endl;
    }
    else if (WIFSIGNALED(status))
    {
        const int code = WTERMSIG(status);
        std::cout << "Killed by signal: " << code << std::endl;
    }
}

void evalWithPTY(const std::string& input)
{
    int master_fd;
    int proc_ID = forkpty(&master_fd, nullptr, nullptr, nullptr);
    // disabling echo mode prevents the pty from reprinting inputs (idk its their code)
    // canonical mode is a feature that we dont need because we want to send input instantly (character buffered instead of line buffered)
    // the slave side is still canonical

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

int repl()
{
    for (;;)
    {
        char buf[PATH_MAX];
        if (getcwd(buf, PATH_MAX) == nullptr)
        {
            std::cout << "Couldn't determine current directory." << std::endl;
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        std::string prompt = (std::string("temu [")  + buf + "] % ");
        char* input = readline(prompt.c_str());
        if (input == nullptr)
        {
            continue;
        }

        add_history(input);
        auto free_str = [](char* str) { free(str); };
        std::unique_ptr<char, decltype(free_str)> input_deleter(input, free_str);
        bool should_exit = false, should_clear = false;

        if (!temu::interceptBuiltins(input, should_exit, should_clear))
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

int main()
{
    return repl();
}

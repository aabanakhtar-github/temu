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
        // establish the terminal state
        temu::TerminalState state;
        // child process
        // we don't need the master fd, handle directly with stdin stdout
        close(master_fd);
        temu::runChildProcess(input);
    }
    else
    {
        // parent process
        temu::Piper piper(master_fd);
        temu::ExitHandler handler(proc_ID);
        while (true)
        {
            int status;
            // see if the proc is done
            int pid = SYSCALL(waitpid(proc_ID, &status, WNOHANG));
            if (pid == proc_id)
            {
                break;
            }

            // break if the process is done.
            if (handler.checkForExit())
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
        SYSCALL(wait(&status));

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
}

[[noreturn]] int repl()
{
    for (;;)
    {
        std::cout << "temu % ";
        std::string input;
        std::getline(std::cin, input);
        evalWithPTY(input);
    }
}

int main()
{
    return repl();
}

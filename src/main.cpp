#include <iostream>
#include <ranges>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/poll.h>

#include "pipe.h"
#include "util.h"

void execChildProcess()
{

}

void execParentProcess()
{

}


void eval(const std::string& input)
{
    int stdout_pipe_fd[2];
    int stdin_pipe_fd[2];
    // making the pipe failed
    if (pipe(stdout_pipe_fd) == -1 || pipe(stdin_pipe_fd) == -1)
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
        closePipe(stdout_pipe_fd);
        closePipe(stdin_pipe_fd);
        return;
    }
    else if (pid == 0)
    {
        // child process
        // disable reading (we don't need), enable writing from stdout, and then close the pipe.
        // similarly dont need writing from stdin lol
        close(stdout_pipe_fd[PIPE_READ]);
        close(stdin_pipe_fd[PIPE_WRITE]);
        redirect(stdout_pipe_fd[PIPE_WRITE], {STDOUT_FILENO, STDERR_FILENO});
        redirect(stdin_pipe_fd[PIPE_READ], {STDIN_FILENO});
        close(stdout_pipe_fd[PIPE_WRITE]);
        close(stdin_pipe_fd[PIPE_READ]);

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
        close(stdin_pipe_fd[PIPE_READ]);

        pollfd poll_fds[2] = {{}, {}};
        poll_fds[0].events = POLLIN; // poll child stdout
        poll_fds[0].fd = stdout_pipe_fd[PIPE_READ];
        poll_fds[1].events = POLLIN; // poll our stdin
        poll_fds[1].fd = STDIN_FILENO;

        while (true)
        {
            if (int res = poll(poll_fds, 2, -1); res == -1)
            {
                std::cout << "poll failed" << std::endl;
                perror("poll");
                break;
            }

            // handle stdout from child
            if (poll_fds[0].revents & POLLIN)
            {
                char buf[4096];
                // read while we can
                ssize_t buffer_size;
                while ((buffer_size = read(poll_fds[0].fd, buf, sizeof(buf))) > 0)
                {
                    // null terminate it
                    buf[buffer_size] = '\0';
                    std::cout << buf;
                }
                // closed by client
                if (buffer_size == 0)
                {
                   break;
                }
                else if (buffer_size == -1)
                {
                    perror("failed to read child stdout");
                }
            }
        }

        close(stdout_pipe_fd[PIPE_READ]);

        // wait for process to exit
        int status;
        wait(&status);

        if (WIFEXITED(status))
        {
            int code = WEXITSTATUS(status);
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

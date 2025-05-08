#include <iostream>
#include <ranges>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <pty.h>
#include <fcntl.h>
#include "pipe.h"
#include "util.h"

void execChildProcess()
{
}

void execParentProcess()
{
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
        // child process
        close(master_fd);

        auto args = makeCArgs(splitString(input, ' '));
        execvp(args[0], args.data());
        // execvp should never return
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process

        const int flags = fcntl(master_fd, F_GETFL, 0);
        fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
        pollfd poll_fd{};
        poll_fd.fd = master_fd;
        poll_fd.events = POLLIN | POLLOUT; // for stdout from the proc

        while (true)
        {
            // break if the process is done.
            int status;
            if (const int pid = waitpid(proc_ID, &status, WNOHANG); pid == proc_ID)
            {
                // child exited;
                break;
            }

            if (poll(&poll_fd, 1, -1) == -1)
            {
                perror("poll");
                exit(EXIT_FAILURE);
            }

            // read stdout from the program and print it
            if (poll_fd.revents & POLLIN)
            {
                char buf[4096];
                ssize_t buffer_size;
                // read the stdout
                while ((buffer_size = read(poll_fd.fd, &buf, sizeof(buf))) > 0)
                {
                    buf[buffer_size] = 0;
                    for (ssize_t i = 0; i < buffer_size; ++i) {
                        std::cout << "Char [" << buf[i] << "] ";
                    }
                    std::cout << buf;
                }

                // signal EOF, close the app
                if (buffer_size == 0)
                {
                    break;
                }
                // problem occurred when reading stdout
                if (buffer_size == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO)
                    {
                        // Nothing to read now — fine, continue polling
                        continue;
                    }
                    else
                    {
                        perror("read");
                        break;
                    }
                }
            }
        }

        close(master_fd);

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
        evalWithPTY(input);
    }
}

int main()
{
    return repl();
}

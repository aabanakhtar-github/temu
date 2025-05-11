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

void resetTerminalSettings()
{
    termios terminal;
    tcgetattr(0, &terminal);
    // disable echo mode, and canonical mode
    terminal.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(0, TCSANOW, &terminal);
}

void evalWithPTY(const std::string& input)
{
    int master_fd;
    int proc_ID = forkpty(&master_fd, nullptr, nullptr, nullptr);
    termios terminal{};
    // disabling echo mode prevents the pty from reprinting inputs (idk its their code)
    // canonical mode is a feature that we dont need because we want to send input instantly (character buffered instead of line buffered)
    // the slave side is still canonical
    tcgetattr(master_fd, &terminal); // get current settings
    terminal.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(master_fd, TCSANOW, &terminal);
    atexit(resetTerminalSettings);

    if (proc_ID == -1)
    {
        perror("forkpty");
        exit(EXIT_FAILURE);
    }
    else if (proc_ID == 0)
    {
        // child process
        close(master_fd);
        auto input_vec = splitString(input, ' ');
        auto args = makeCArgs(input_vec);

        execvp((input_vec[0]).c_str(), args.data());
        // execvp should never return
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process

        const int flags = fcntl(master_fd, F_GETFL, 0);
        fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
        fcntl(master_fd, F_SETFL, O_NONBLOCK);
        pollfd poll_fds[2];
        poll_fds[0].fd = master_fd;
        poll_fds[0].events = POLLIN | POLLOUT; // for stdout from the proc
        poll_fds[1].fd = STDIN_FILENO;
        poll_fds[1].events = POLLIN;
        while (true)
        {
            // break if the process is done.
            int status;
            if (const int pid = waitpid(proc_ID, &status, WNOHANG); pid == proc_ID)
            {
                // child exited;
                break;
            }

            if (poll(poll_fds, 2, -1) == -1)
            {
                perror("poll");
                exit(EXIT_FAILURE);
            }

            // read stdout from the program and print it
            if (poll_fds[0].revents & POLLIN)
            {
                char buf[4096];
                ssize_t buffer_size;
                // read the stdout
                while ((buffer_size = read(master_fd, &buf, sizeof(buf))) > 0)
                {
                    std::cout.write(buf, buffer_size);
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

            if (poll_fds[1].revents & POLLIN)
            {
                char read_buf[1024];
                ssize_t count = read(STDIN_FILENO, read_buf, sizeof(read_buf));
                // if theres input AND we can write to the pty without blocking
                if (count > 0 && (poll_fds[0].revents & POLLOUT))
                {
                    write(master_fd, read_buf, count);
                }
                else if (count == 0)
                {
                    close(STDIN_FILENO);
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

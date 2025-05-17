//
// Created by aabanakhtar on 5/11/25.
//

#include "temu_core.h"

#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>

#include "syscall.h"
#include "util.h"

namespace temu
{
    TerminalState::TerminalState()
    {
        // get terminal attrs
        termios terminal_config{};
        SYSCALL(tcgetattr(0, &terminal_config));
        terminal_config.c_lflag &= ~DISABLE_FLAGS;
        SYSCALL(tcsetattr(STDIN_FILENO, TCSANOW, &terminal_config));
    }

    TerminalState::~TerminalState()
    {
        termios terminal_config{};
        SYSCALL(tcgetattr(STDIN_FILENO, &terminal_config));
        terminal_config.c_lflag &= ~DISABLE_FLAGS;
        SYSCALL(tcsetattr(STDIN_FILENO, TCSANOW, &terminal_config));
    }

    Piper::Piper(const int master_fd)
        :
        master_fd(master_fd),
        watch{
            {.fd = master_fd, .events = MASTER_POLL_FLAGS},
            {.fd = STDIN_FILENO, .events = STDIN_POLL_FLAGS},
        }
    {
        const int flags = SYSCALL(fcntl(master_fd, F_GETFL, 0));
        SYSCALL(fcntl(master_fd, F_SETFL, flags | O_NONBLOCK)); // make it nonblocking
    }

    bool Piper::pipeInputs()
    {
        if (SYSCALL(poll(watch, 2, -1)) == -1)
        {
            perror("poll");
            return true;
        }

        // read stdout from the program and print it
        if (watch[MASTER_FD].revents & POLLIN)
        {
            char buf[4096];
            ssize_t buffer_size;
            // read the stdout
            while ((buffer_size =read(master_fd, &buf, sizeof(buf))) > 0)
            {
                std::cout.write(buf, buffer_size);
            }
            // signal EOF, close the app
            if (buffer_size == 0)
            {
                return true;
            }
            // problem occurred when reading stdout
            if (buffer_size == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EIO)
                {
                    // Nothing to read now — fine, continue polling
                    return false;;
                }
                else
                {
                    perror("read");
                    return true;
                }
            }
        }

        if (watch[STDIN_FD].revents & POLLIN)
        {
            char read_buf[1024];
            // if there's input AND we can write to the pty without blocking
            if (const ssize_t count = read(STDIN_FILENO, read_buf, sizeof(read_buf)); count > 0 && (watch[MASTER_FD].
                revents & POLLOUT))
            {
                SYSCALL(write(master_fd, read_buf, count));
            }
            else if (count == 0)
            {
                // signal eof to the child process
                SYSCALL(close(STDIN_FILENO));
            }
        }

        return false;
    }

    void runChildProcess(const std::string& args)
    {
        auto shell_command = args;
        const auto input_vec = splitString(args, ' ');
        const auto argv = makeCArgs(input_vec);
        // run the app
        execvp((input_vec[0]).c_str(), argv.data());
        // execvp should never return
        perror("execvp");
        exit(EXIT_FAILURE);
    }


    bool interceptBuiltins(const std::string& input, bool& out_exit, bool& out_clear)
    {
        if (input == "exit")
        {
            out_exit = true;
            return true;
        }
        // check for emtpy string
        auto copy = input;
        if (copy.empty())
        {
            // nothing to parse
            return false;
        }
        // handle more complex builtins
        auto argv = splitString(copy, ' ');
        std::for_each(argv.begin(), argv.end(), [&](auto& elem)
        {
            std::erase_if(elem, ::isspace);
        });
        if (argv[0] == "cd")
        {
            if (!(argv.size() == 2))
            {
                std::cout << "Usage: cd <path>" << std::endl;
                return true;
            }
            // dir change
            if (SYSCALL(chdir(argv[1].c_str())) == ENOTDIR)
            {
                std::cout << "Path is not a directory." << std::endl;
            }

            return true;
        }
        else if (argv[0] == "clear")
        {
            out_clear = true;
            return true;
        }

        return false;
    }
}

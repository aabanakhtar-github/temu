//
// Created by aabanakhtar on 5/10/25.
//

#ifndef TEMU_CORE_H
#define TEMU_CORE_H

#include <string>
#include <termios.h>
#include <sys/poll.h>

namespace temu
{
    struct TerminalState
    {
        TerminalState();
        ~TerminalState();
        // disable echo and canonical mode, so we don't get looped inputs, and we send our stdin instantly
        // doesn't affect pty slave's config
        constexpr static int DISABLE_FLAGS = ECHO | ICANON;
    };

    // pipes the process stdin and stdout
    struct Piper
    {
        explicit Piper(int master_fd);

        // returns whether the application should close
        bool pipeInputs();

        pollfd watch[2];
        int master_fd;
        constexpr static int MASTER_FD = 0;
        constexpr static int STDIN_FD = 1;
        // watch the readability and writability both to aviod blocking
        constexpr static int MASTER_POLL_FLAGS = POLLIN | POLLOUT;
        // check if we have input ready in stdin
        constexpr static int STDIN_POLL_FLAGS = POLLIN;
    };

    void runChildProcess(const std::string& args);
    bool interceptBuiltins(const std::string& input, bool& out_exit, bool& out_clear);
}

#endif //TEMU_CORE_H

//
// Created by aabanakhtar on 5/6/25.
//

#ifndef PIPE_H
#define PIPE_H

#include <cstddef>
#include <vector>
#include <unistd.h>

constexpr std::size_t PIPE_READ = 0;
constexpr std::size_t PIPE_WRITE = 1;

inline void closePipe(const int pipe[2])
{
    close(pipe[PIPE_READ]);
    close(pipe[PIPE_WRITE]);
}

inline void redirect(const int pipe, const std::vector<int>& fds)
{
    for (const auto fd : fds)
    {
        dup2(pipe, fd);
    }
}

#endif //PIPE_H

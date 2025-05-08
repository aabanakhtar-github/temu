//
// Created by aabanakhtar on 5/3/25.
//

#include "unix_wrappers.h"


Pipe::Pipe()
    : read(), write()
{
    int fds[2];
    if (pipe(fds) == -1)
    {
        perror("pipe");
        return;
    }

    read = FileDescriptor(fds[0]);
    write = FileDescriptor(fds[1]);
}

CArgv::CArgv(const std::vector<std::string>& args)
    : cArgv()
{
    cArgv.reserve(args.size());
    for (auto& arg : args)
    {
        cArgv.push_back(const_cast<char*>(arg.c_str()));
    }
    // required by execvp
    cArgv.push_back(nullptr);
}


char** CArgv::asCArgv()
{
   return cArgv.data();
}

void reroute(const FileDescriptor& source, const std::vector<int>& fds)
{
    for (const auto fd : fds)
    {
        dup2(source.fd, fd);
    }
    close(source.fd);
}
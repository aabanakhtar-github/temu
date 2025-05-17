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

        // Create pipe if not last command
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
            // CHILD
            if (previous_read != -1)
            {
                // rewire prev stdout into our stdin
                redirect(previous_read, {STDIN_FILENO});
                // unneeded anymore
                SYSCALL(close(previous_read));
            }

            if (i != input.size() - 1)
            {
                SYSCALL(close(pipefd[PIPE_READ])); // close read end, since we are writing only
                redirect(pipefd[PIPE_WRITE], {STDOUT_FILENO}); // rewire stdout to write end
                SYSCALL(close(pipefd[PIPE_WRITE]));
            }

            temu::runChildProcess(input[i]);
        }
        else
        {
            // PARENT
            pids.push_back(pid);
            if (previous_read != -1)
            {
                // get rid of this fd
                SYSCALL(close(previous_read));
            }

            if (i != input.size() - 1)
            {
                close(pipefd[PIPE_WRITE]); // close write end, we don't need it
                previous_read = pipefd[0]; // next child reads from this
            }
        }
    }
    // wait for all the processes to exit
    for (const pid_t pid : pids)
    {
        int status;
        waitpid(pid, &status, 0);
    }
}
bool should_exit = false, should_clear = false;

int repl();

void signalHandler(const int sig_num)
{
    std::cout << "Killed by interrupt." << std::endl;
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


int main()
{
    return repl();
}

//
// Created by aabanakhtar on 5/11/25.
//

#ifndef SYSCALL_H
#define SYSCALL_H

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define SYSCALL(expr)                         \
([&]() -> decltype(expr) {                          \
auto __res = (expr);                            \
if (__res == -1) {                              \
std::fprintf(stderr, "%s failed: %s\n",     \
#expr, std::strerror(errno));  \
std::exit(EXIT_FAILURE);                    \
}                                               \
return __res;                                   \
})()

#endif //SYSCALL_H

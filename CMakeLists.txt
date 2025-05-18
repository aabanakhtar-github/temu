cmake_minimum_required(VERSION 3.5)
project(temu)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_executable(temu
        src/main.cpp
        src/main.cpp
        src/util.h
        src/util.cpp
        src/pipe.h
        src/temu_core.h
        src/temu_core.cpp
        src/syscall.h
        src/temu_application.cpp
        src/temu_application.h
)

target_link_libraries(temu readline)
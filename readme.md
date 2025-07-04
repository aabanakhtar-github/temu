# Temu - A  Terminal Emulator in C++

**Temu** is a small terminal shell written in C++ that runs interactive programs using POSIX PTYs and supports basic command piping, built-ins, and running your own applications. 

---
## Dependencies
You will need the ```readline``` package installed on your system

## ✨ Features

* Prompt with working directory:

  ```
  temu [/home/user] %
  ```
* Runs interactive programs like `cat`, `nano`, and `bash`
* Supports pipelines like `ls | grep .cpp`
* Built-in commands: `cd`, `exit`, `clear`
* Remembers command history with arrow keys

## TODO:
Ctrl+C handling.

---

## 🛠 How to Build

You need a Linux system with a C++20 compiler and CMake.

```bash
git clone  https://github.com/aabanakhtar-github/temu.git
cd temu
mkdir build && cd build
cmake ..
make
./temu
```

---

## 🔁 Example Session

```bash
temu [/home/aabanakhtar/temu/cmake-build-debug] % cat
hello world
hello world
^C
Killed by signal: 2
temu [/home/aabanakhtar/temu/cmake-build-debug] % ls -l | grep cpp | wc -l
4
```

//
// Created by aabanakhtar on 5/10/25.
//

#ifndef TEMU_CORE_H
#define TEMU_CORE_H

void disableEchoCanonicalMode();
void reenableEchoCanonicalMode();

void runChildProcess();
void readChildStdout();
void handleStdin();

void handleExit();

#endif //TEMU_CORE_H

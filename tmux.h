#pragma once
#include <stdlib.h>
#define MAX_CMD 32

int tmux_call(char *buffer, size_t bufsize, int nargs, ...);
int tmux_start();
int tmux_ps();

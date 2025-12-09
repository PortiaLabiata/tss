#pragma once
#include <stdlib.h>
#define MAX_CMD 32
#define MAX_NAME 128

struct t_session_s {
	char *name[MAX_NAME];
};

struct t_window_s {
	char *name;
};

struct t_pane_s {
	char *name;
};

int tmux_call(char *buffer, size_t bufsize, int nargs, ...);
int tmux_start();
int tmux_has_session(const char *sname);

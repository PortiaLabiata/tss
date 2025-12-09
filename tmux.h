#pragma once
#include <stdlib.h>
#include "apps.h"

#define MAX_CMD 32
#define MAX_NAME 128
#define MAX_PATH 1024

struct t_window_s {
	struct t_window_s *prev;
	int idx;
	char name[MAX_NAME];
	char path[MAX_PATH];
};

int tmux_call(char *buffer, size_t bufsize, int nargs, ...);
int tmux_start();
int tmux_has_session(const char *sname);
struct t_window_s *tmux_get_windows(const char *sname);
int tmux_save_windows(const char *fpath, struct t_window_s *last);
void t_windows_free(struct t_window_s *last);

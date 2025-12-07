#pragma once
#include <stdlib.h>

typedef int (*app_cb_t)(int argc, char **argv);
struct session_s {
	struct session_s *next;
	size_t name_size;
	char name[];
};

app_cb_t find_app_cb(char *name);
int app_ls(int argc, char **argv);

struct session_s *session_create(const char *name);
struct session_s *read_sessions();
void session_free(struct session_s *head);
int app_ls(int argc, char **argv);
int app_ps(int argc, char **argv);

#pragma once
#include <stdlib.h>

typedef int (*app_cb_t)(int argc, char **argv);
struct session_s {
	struct session_s *next;
	size_t name_size;
	char name[];
};

struct cb_map_s {
	const char *name;
	app_cb_t callback;
	size_t min_args;
	const char *help;
};

struct cb_map_s *find_app(char *name);
const char *_getenv_path();

struct session_s *session_create(const char *name);
struct session_s *read_sessions();
void session_free(struct session_s *head);

int app_ls(int argc, char **argv);
int app_ps(int argc, char **argv);
int app_run(int argc, char **argv);
int app_new(int argc, char **argv);
int app_edit(int argc, char **argv);
int app_help(int argc, char **argv);
int app_rm(int argc, char **argv);
int app_save(int argc, char **argv);

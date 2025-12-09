#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "common.h"
#include "tmux.h"

static const char *_window_format = "#{window_index}\t#{window_name}\t#{pane_path}";

// Calls a specified set of tmux commands and writes stdout to buffer
int tmux_call(char *buffer, size_t bufsize, int nargs, ...) {
	va_list args;
	va_start(args, nargs);

	// Create pipe to redirect stdout
	int pipefd[2];
	if (pipe(pipefd) == -1) {
		err_printf("Pipe failed!\n");
		exit(-ERR_PROC);
	}

	// Create subprocess
	pid_t wpid, pid = fork();	
	int status = -1;
	switch (pid) {
		case -1:
			// Could not create subprocess, exit
			if (buffer != NULL) {
				close(pipefd[0]); close(pipefd[1]);
			}
			err_printf("Could not fork process, exiting\n");
			exit(-ERR_PROC);
		case 0: {
			// Redirect stdout to pipe
			close(pipefd[0]);
			dup2(pipefd[1], STDOUT_FILENO);
			close(pipefd[1]);

			// Make commands list, including ending and start
			char *new_cmd[nargs + 2];
			new_cmd[0] = "tmux";
			for (int i = 1; i <= nargs; i++) {
				new_cmd[i] = va_arg(args, char*);
			}

			new_cmd[nargs+1] = (char*)0;
			execvp(new_cmd[0], new_cmd);
			_exit(127);
		}
		default:
			break;
	}
	// Parent process
	size_t pos = 0;
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	while ((wpid = waitpid(pid, &status, WNOHANG)) == 0) {
		// Read stdout from pipe to buffer, until process
		// exits
		if (!buffer) continue;
		ssize_t size = read(pipefd[0], buffer+pos, 
						bufsize - pos - 1);
		if (size < 0) continue;
		pos += size;
	}
	if (buffer != NULL) 
		buffer[pos] = 0;

	if (wpid == -1) {
		err_printf("wpid errored!\n");
		exit(-ERR_PROC);
	}
	return WEXITSTATUS(status);
}

int tmux_start() {
	return tmux_call(NULL, 0, 1, "start-server");
}

int tmux_has_session(const char *sname) {
	return tmux_call(NULL, 0, 3, "has", "-t", sname);
}

char *_tmux_get_windows(const char *sname) {
	#define _BUFSIZE 4096
	static char buffer[_BUFSIZE] = {0};
	tmux_call(buffer, _BUFSIZE, 5, "list-windows", "-F", _window_format, \
					"-t", sname);
	return buffer;
}

enum wstr_idx_e {
	WSTR_IDX_IDX = 0,
	WSTR_IDX_NAME,
	WSTR_IDX_PATH,
	WSTR_IDX_MAX
};

struct t_window_s *t_window_parse(char *_str) {
	struct t_window_s *res = malloc(sizeof(struct t_window_s));	
	memset(res, 0, sizeof(struct t_window_s));

	char *str, *token, *sptr;
	int i = 0;
	for (str = _str; ; i++, str = NULL) {
		token = strtok_r(str, "\t", &sptr);
		if (!token)
			break;
		switch ((enum wstr_idx_e)i) {
			case WSTR_IDX_IDX:
				res->idx = atoi(token);
				break;
			case WSTR_IDX_NAME:
				strncpy(res->name, token, MAX_NAME);
				break;
			case WSTR_IDX_PATH:
				strncpy(res->path, token, MAX_PATH);
				break;
			default:
				break;
		}
	}
	if (i - WSTR_IDX_MAX > 1) {
		free(res);
		return NULL;
	}
	return res;
}

void t_windows_free(struct t_window_s *last) {
	struct t_window_s *prev = last;
	while (last) {
		prev = last;
		last = prev->prev;
		free(prev);
	}
}

struct t_window_s *tmux_get_windows(const char *sname) {
	char *windows = _tmux_get_windows(sname);

	char *str1; 
	char *token;
	char *sptr1;
	struct t_window_s *prev, *swind;
	prev = swind = NULL;

	for (str1 = windows; ; str1 = NULL) {
		token = strtok_r(str1, "\n", &sptr1);
		if (!token) 
			break;
		swind = t_window_parse(token);
		if (!swind) {
			t_windows_free(swind);
			return NULL;
		}
		swind->prev = prev;
		prev = swind;
	}
	return swind;
}

const char *_make_wpath(const char *name) {
	static char path[MAX_PATH] = {0};
	const char *env = _getenv_path();
	snprintf(path, MAX_PATH, "%s/%s.conf", env, name);	
	return path;
}

int _tmux_save_window(FILE *file, struct t_window_s *wind) {
	return fprintf(file, "new-window -n %s -c %s -t %d\n", wind->name, wind->path, wind->idx) > 0;
}

struct t_window_s *_invert_windows(struct t_window_s *last) {
	struct t_window_s *prev, *curr, *next;	
	prev = next = NULL; curr = last;

	while (curr) {
		prev = curr->prev;
		curr->prev = next;
		next = curr; curr = prev;
	}
	return next;
}

int tmux_save_windows(const char *fname, struct t_window_s *last) {
	struct t_window_s *prev = last;

	FILE *file = fopen(_make_wpath(fname), "w");
	if (!file) {
		return -1;
	}
	fprintf(file, "new -s %s\n", fname);
	while (last) {
		if (last->idx != 1) {
			_tmux_save_window(file, last);	
		} else {
			fprintf(file, "renamew -t 1 %s\n", last->name);
		}
		last = prev->prev;
		prev = last;
	}
	fclose(file);
	return 0;
}

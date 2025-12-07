#include <stdarg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "common.h"
#include "tmux.h"

int tmux_call(char *buffer, size_t bufsize, int nargs, ...) {
	va_list args;
	va_start(args, nargs);

	int pipefd[2];
	if (pipe(pipefd) == -1) {
		err_printf("Pipe failed!\n");
		exit(-ERR_PROC);
	}

	pid_t wpid, pid = fork();	
	int status = -1;
	switch (pid) {
		case -1:
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

			char *new_cmd[nargs + 2];
			new_cmd[0] = "/usr/bin/tmux";
			for (int i = 1; i <= nargs; i++) {
				new_cmd[i] = va_arg(args, char*);
			}

			new_cmd[nargs+1] = (char*)0;
			execv(new_cmd[0], new_cmd);
			_exit(127);
		}
		default:
			break;
	}
	size_t pos = 0;
	fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
	while ((wpid = waitpid(pid, &status, WNOHANG)) == 0) {
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

#include <getopt.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "apps.h"
#include "tmux.h"

#define MAX_NAME_LEN 258
#define BUFSIZE 4096

static const char *_getenv_path();
static char buffer[BUFSIZE] = {0};

static struct {
	const char *name;
	app_cb_t callback;
} _cb_map[] = {
	{"run", &app_run},
	{"ls", &app_ls},
	{"ps", &app_ps},
	{"new", &app_new},
};

static const char *_getenv_path() {
	const char *_res = getenv("TMUX_SESSIONS");
	if (_res == NULL) {
		err_printf("TMUX_SESSIONS variable unset!\n");	
		exit(-ERR_ENV);
	}
	return _res;
}

app_cb_t find_app_cb(char *name) {
	for (size_t i = 0; i < sizeof(_cb_map); i++) {
		if (streq(_cb_map[i].name, name)) {
			return _cb_map[i].callback;
		}
	}
	return NULL;
}

[[maybe_unused]]
static int _putn_name(const char *name, int size) {
	int i = 0;
	for (; i < size && name[i] != '\0'; i++) {
		if (name[i] == '.') {
			putchar('\n');
			break;
		}
		putchar(name[i]);
	}
	return i;
}

static int _invalid_file(struct dirent *file) {
	return streq(file->d_name, ".") || streq(file->d_name, "..") \
					|| streq(file->d_name, "build.sh") \
					|| streq(file->d_name, "template.sh") \
					|| file->d_type != DT_REG;
}

int strchr_ext(const char *str) {
	return strchr(str, '.') - str;
}

int streq_ext(const char *stra, const char *strb) {
	int n = min(strchr_ext(stra), strchr_ext(strb));		
	return strncmp(stra, strb, n) == 0;
}

int filter_any(const char *name, void *args) { 
	(void)name; (void)args;
	return 1; 
}
int filter_exact(const char *name, void *args) {
	return streq_ext(name, (const char*)args);
}

struct session_s *session_create(const char *name) {
	size_t size = strlen(name);
	struct session_s *res = malloc(sizeof(struct session_s) + \
					size+1);
	if (!res) {
		err_printf("Could not allocate session\n");
		exit(-ERR_MEM);
	}
	res->name_size = size;
	strncpy(res->name, name, size);
	*strchr(res->name, '.') = 0;
	return res;
}

struct session_s *session_push(struct session_s *ptr, const char *name) {
	ptr->next = session_create(name);
	return ptr->next;
}

struct session_s *read_sessions() {
	DIR *dir = opendir(_getenv_path());
	struct dirent *file = readdir(dir);

	if (_invalid_file(file)) file = readdir(dir);

	struct session_s *head, *ptr;
	head = ptr = session_create(file->d_name);

	while ((file = readdir(dir)) != NULL) {
		if (_invalid_file(file)) continue;
		ptr = session_push(ptr, file->d_name);
	}
	return head;
}

void session_free(struct session_s *head) {
	struct session_s *next = head;
	while (next) {
		next = next->next;
		free(head);
		head = next;
	}
}

int app_ls(int argc, char **argv) {
	(void)argc; (void)argv;
	int (*filter)(const char*, void*) = &filter_any;

	if (argc > 1) {
		filter = &filter_exact;	
	}

	struct session_s *head, *ptr;
	head = ptr = read_sessions();
	while (ptr) {
		if (filter(ptr->name, argv[1])) {
			printf("%s\n", ptr->name);
		}
		ptr = ptr->next;
	}
	session_free(head);
	
	return 0;
}

static void _print_session(const char *name, struct session_s *head) {
	struct session_s *ptr = head;
	while (ptr) {
		if (streq(name, ptr->name)) {
			printf("%s\n", ptr->name);
		}
		ptr = ptr->next;
	}
}

int app_ps(int argc, char **argv) {
	(void)argc; (void)argv;
	int status = tmux_call(buffer, BUFSIZE, 3, "list-sessions", "-F", "#{session_name}");
	if (status) return status;

	struct session_s *head, *ptr;
	head = ptr = read_sessions();
	
	char *session = strtok(buffer, "\n");
	_print_session(session, head);

	while ((session = strtok(NULL, "\n")) != NULL) {
		_print_session(session, head);	
	}

	session_free(head);
	return 0;
}

int app_run(int argc, char **argv) {
	if (argc < 2) {
		err_printf("run: not enough arguments");
		exit(-ERR_SYN);
	}

	struct session_s *head, *ptr;
	head = ptr = read_sessions();
	
	while (ptr) {
		if (streq(ptr->name, argv[1])) {
			char s[4096] = {0};
			snprintf(s, 4096, "%s/%s.sh", _getenv_path(), ptr->name);
			session_free(head);

			execl(s, s, (char*)0);

			err_printf("run: couldn't launch process\n");
			exit(-ERR_PROC);
		}
		ptr = ptr->next;
	}
	exit(-ERR_PROC);
}

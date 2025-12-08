#include <getopt.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.h"
#include "apps.h"
#include "tmux.h"

#define MAX_NAME_LEN 258
#define BUFSIZE 4096

static const char *_getenv_path();
// Buffer for subprocess stdin reading
static char buffer[BUFSIZE] = {0};

// Mapping between command name and callback for it
static struct cb_map_s _cb_map[] = {
	{"run", &app_run, 1},
	{"ls", &app_ls, 0},
	{"ps", &app_ps, 0},
	{"new", &app_new, 1},
	{"rm", &app_rm, 1},
	{"edit", &app_edit, 1},
	{"help", &app_help, 0},
};

// Reads $TMUX_SESSIONS env variable and exits if it is unset
static const char *_getenv_path() {
	const char *_res = getenv("TMUX_SESSIONS");
	if (_res == NULL) {
		err_printf("TMUX_SESSIONS variable unset!\n");	
		exit(-ERR_ENV);
	}
	return _res;
}

// Lookup callback by command name
struct cb_map_s *find_app(char *name) {
	for (size_t i = 0; i < sizeof(_cb_map)/sizeof(struct cb_map_s); i++) {
		if (streq(_cb_map[i].name, name)) {
			return &_cb_map[i];
		}
	}
	return NULL;
}

// Print name, stripping extension
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

// Is given file a session script?
static int _invalid_file(struct dirent *file) {
	return streq(file->d_name, ".") || streq(file->d_name, "..") \
					|| streq(file->d_name, "build.sh") \
					|| streq(file->d_name, "template.sh") \
					|| file->d_type != DT_REG;
}

// Find position, where extension separator begins
int strchr_ext(const char *str) {
	return strchr(str, '.') - str;
}

// Compare two strings excluding extensions
int streq_ext(const char *stra, const char *strb) {
	int n = min(strchr_ext(stra), strchr_ext(strb));		
	return strncmp(stra, strb, n) == 0;
}

// Filter functions for ls and ps commands
int filter_any(const char *name, void *args) { 
	(void)name; (void)args;
	return 1; 
}
int filter_exact(const char *name, void *args) {
	return streq_ext(name, (const char*)args);
}

// Creates session_s struct - linked list entry
struct session_s *session_create(const char *name) {
	size_t size = strlen(name);
	struct session_s *res = malloc(sizeof(struct session_s) + \
					size+1);
	if (!res) {
		err_printf("Could not allocate session\n");
		exit(-ERR_MEM);
	}
	res->next = NULL;

	res->name_size = size;
	strncpy(res->name, name, size);
	// Strip extension
	*strchr(res->name, '.') = 0;
	return res;
}

struct session_s *session_push(struct session_s *ptr, const char *name) {
	ptr->next = session_create(name);
	return ptr->next;
}

// Returns a head of linked list of session_s structs
struct session_s *read_sessions() {
	// Open dir with sessions
	DIR *dir = opendir(_getenv_path());
	struct dirent *file = readdir(dir);

	// Edge case: if first file is invalid, we need to skip it
	if (_invalid_file(file)) file = readdir(dir);

	struct session_s *head, *ptr;
	head = ptr = session_create(file->d_name);

	// Creates linked list
	while ((file = readdir(dir)) != NULL) {
		if (_invalid_file(file)) continue;
		ptr = session_push(ptr, file->d_name);
	}
	return head;
}

// Frees linked list entries
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
	// Filter needed for "ps <name>" call, if no name
	// passed, we use "any" filter and print all values
	int (*filter)(const char*, void*) = &filter_any;

	// If <name> passed, we only print it, if it exists
	if (argc > 0) {
		filter = &filter_exact;	
	}

	struct session_s *head, *ptr;
	head = ptr = read_sessions();
	while (ptr) {
		if (filter(ptr->name, argv[0])) {
			printf("%s\n", ptr->name);
		}
		ptr = ptr->next;
	}
	session_free(head);
	
	return 0;
}

// Prints session if it is in linked list passed by head
// (which means, it is in $TMUX_SESSIONS dir)
static void _print_session(const char *name, struct session_s *head) {
	struct session_s *ptr = head;
	while (ptr) {
		// !!! Compares by name !!!
		if (streq(name, ptr->name)) {
			printf("%s\n", ptr->name);
		}
		ptr = ptr->next;
	}
}

int app_ps(int argc, char **argv) {
	(void)argc; (void)argv;
	// List sessions and put stdout to buffer
	int status = tmux_call(buffer, BUFSIZE, 3, "list-sessions", "-F", "#{session_name}");
	if (status) return status;

	int (*filter)(const char*, void*) = &filter_any;

	// If <name> passed, we only print it, if it exists
	if (argc > 0) {
		filter = &filter_exact;	
	}

	struct session_s *head, *ptr;
	head = ptr = read_sessions();
	
	char *session = strtok(buffer, "\n");
	// Edge case: we need to check first session
	_print_session(session, head);

	// Print sessions
	while ((session = strtok(NULL, "\n")) != NULL) {
		if (filter(session, argv[0])) {
			_print_session(session, head);	
		}
	}

	session_free(head);
	return 0;
}

struct session_s *_lookup_session(struct session_s *head, const char *name) {
	struct session_s *ptr = head;
	while (ptr) {
		if (streq(ptr->name, name)) {
			break;	
		}
		ptr = ptr->next;
	}
	return ptr;
}

const char *_make_spath(const char *name) {
	#define _BUFSIZE 4096
	static char s[_BUFSIZE] = {0};
	snprintf(s, _BUFSIZE, "%s/%s.sh", _getenv_path(), name);
	return s;
}

int app_run(int argc, char **argv) {
	(void)argc; (void)argv;
	struct session_s *head, *ptr;
	head = read_sessions();
	ptr = _lookup_session(head, argv[0]);

	if (!ptr) {
		err_printf("run: could not find session %s\n", argv[0]);
		exit(-ERR_FS);
	}
	
	// Make script full path
	const char *full_path = _make_spath(ptr->name);
	session_free(head);
	// Execute subprocess
	execl(full_path, full_path, (char*)0);

	err_printf("run: couldn't launch process\n");
	exit(-ERR_PROC);
}

// TODO: DRY in argc check!
int app_new(int argc, char **argv) {
	(void)argc; (void)argv;
	const char *full_path = _make_spath(argv[0]);

	// Create session script and allow it's execution 
	FILE *f = fopen(full_path, "a");
	chmod(full_path, S_IRWXU);

	if (!f) {
		err_printf("Could not create file %s\n", full_path);
		exit(-ERR_FS);
	}
	fclose(f);
	return 0;
}

// TODO: open in designated session
int app_edit(int argc, char **argv) {
	(void)argc; (void)argv;
	struct session_s *head, *ptr;
	head = read_sessions();

	ptr = _lookup_session(head, argv[0]);
	if (!ptr) {
		err_printf("edit: could not find session %s\n", argv[0]);
		exit(-ERR_FS);
	}

	const char *editor = getenv("EDITOR");

	if (!editor) {
		err_printf("$EDITOR variable unset!\n");
		exit(-ERR_ENV);
	}

	const char *full_path = _make_spath(argv[0]);
	printf("%s\n", full_path);
	session_free(head);

	execlp(editor, editor, full_path, (char*)0);

	err_printf("Could not open process %d\n", errno);
	exit(-ERR_FS);
}

int app_rm(int argc, char **argv) {
	(void)argc; (void)argv;
	struct session_s *head, *ptr;
	head = read_sessions();
	ptr = _lookup_session(head, argv[0]);
	if (!ptr) {
		err_printf("Could not find session %s\n", argv[0]);
		exit(-ERR_FS);
	}
	session_free(head);

	const char *full_path = _make_spath(argv[0]);
	if (remove(full_path) < 0) {
		err_printf("Could not remove file %s\n", full_path);
		exit(-ERR_FS);
	}
	return 0;
}

int app_help(int argc, char **argv) {
	(void)argc; (void)argv;
	puts("TSS - a simple tmux session resurrector.");
	puts("Usage: tss <command> <args>");
	puts("ls \t - \t list all session scripts in $TMUX_SESSIONS directory;");
	puts("ps \t - \t list all active TSS sessions;");
	puts("new \t - \t create new session script;");
	puts("edit \t - \t edit session script;");
	puts("run \t - \t run session script.");
	return 0;
}

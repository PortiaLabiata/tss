#include "common.h"
#include "apps.h"

char buf[4096];

// TODO: session info
int main(int argc, char **argv) {
	if (argc < 2) {
		err_printf("Need app name!\n");
		exit(-ERR_SYN);
	}

	struct cb_map_s *app = find_app(argv[1]);
	if (!app) {
		err_printf("Invalid app name: %s\n", argv[1]);
		exit(-ERR_SYN);
	}
	if (argc - 2 < (int)app->min_args) {
		err_printf("Not enough arguments to call app: need at least %lu\n", app->min_args);
		exit(-ERR_SYN);
	}

	app->callback(argc-2, argv+2);
	return 0;
}

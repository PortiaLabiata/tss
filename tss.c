#include "common.h"
#include "apps.h"

char buf[4096];

int main(int argc, char **argv) {
	if (argc < 2) {
		err_printf("Need app name!\n");
		exit(-ERR_SYN);
	}

	app_cb_t cb = find_app_cb(argv[1]);
	if (!cb) {
		err_printf("Invalid app name: %s\n", argv[1]);
		exit(-ERR_SYN);
	}

	cb(argc-2, &argv[2]);
	return 0;
}

#include "apps.h"

char buf[4096];

int main(int argc, char **argv) {
	(void)argc; (void)argv;
	app_ps(argc, argv);	

	return 0;
}

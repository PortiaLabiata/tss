#pragma once
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

enum errors_t {
	ERR_OK = 0,
	ERR_ENV,
	ERR_PROC,
	ERR_MEM,
	ERR_SYN,
	ERR_FS,
};

#define streq(__a__, __b__) (strcmp(__a__, __b__) == 0)
#define err_printf(...) fprintf(stderr, __VA_ARGS__)
#define dbg_printf(...) fprintf(stdout, __VA_ARGS__)
#define max(a, b) (a > b ? a : b)
#define min(a, b) (a > b ? b : a)

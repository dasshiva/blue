#include "utils.h"
#include <string.h>

int main(int argc, const char** argv) {
	argv++;
	while (*argv) {
		printf("#define %s %luUL\n", *argv, Hash(*argv, strlen(*argv)));
		argv++;
	}
	return 0;
}

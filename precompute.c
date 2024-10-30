#include "utils.h"
#include <string.h>

int main(int argc, const char** argv) {
	argv++;
	printf("#include \"types.h\"\n");
	while (*argv) {
		printf("const u64 %s = %lu;\n", *argv, Hash(*argv, strlen(*argv)));
		argv++;
	}
	return 0;
}

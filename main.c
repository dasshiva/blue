#include "oclass.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

void handler (u8 code) {
	switch (code) {
		case ERROR_TRUNCATED_FILE: 
			printf("Class file is truncated\n");
			break;
		case ERROR_CLASS_FILE_FORMAT:
			printf("Class file is not in right format\n");
			break;
		case ERROR_CLASS_FILE_VERSION:
			printf("Unsupported class file version\n");
			break;
		case ERROR_UTF8_ENCODING:
			printf("Invalid UTF-8 encoded string\n");
			break;
		default:
			printf("Unknown error\n");
	}

}

int main(int argc, const char** argv) {
	if (argc != 2)
		return 2;
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) 
		return 3;
	struct stat st;
	stat(argv[1], &st);
	u8* mem = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED)
		return 4;
	struct ClassFile* class = InitClass(mem, st.st_size, handler);
	printf("Could parse file - %d", ParseFile(class, MAX_VERSION));
	return 0;
}

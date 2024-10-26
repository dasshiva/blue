#include "oclass.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, const char** argv) {
	if (argc != 2)
		return 2;
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) 
		return 3;
	struct stat st;
	stat(argv[1], &st);
	u8* mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED)
		return 4;
	struct ClassFile* class = InitClass(mem, st.st_size);
	printf("Could parse file - %d", ParseFile(class, JDK_1_7));
	return 0;
}

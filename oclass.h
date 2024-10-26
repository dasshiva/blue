#ifndef __OCLASS_H__
#define __OCLASS_H__

#include "types.h"
struct ClassFile;
struct ClassFile* InitClass (u8* File, u64 Length);
int ParseFile (struct ClassFile* file, u16 version);

enum ClassFileVersion {
	JDK_1_0 = 45,
	JDK_1_1 = 45, // actually 45.3 but they have same major version
	JDK_1_2,
	JDK_1_3,
	JDK_1_4,
	JDK_1_5,
	JDK_1_6,
	JDK_1_7,
	JDK_VERMAX
};

#define JAVA_MAGIC 0xCAFEBABE
#define MAX_VERSION  (JDK_VERMAX - 1)
#endif

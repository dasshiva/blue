#ifndef __OCLASS_H__
#define __OCLASS_H__

#include "types.h"
struct ClassFile;
struct ClassFile* InitClass (u8* File, u64 Length, void (*Handler) (u8));
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
	JDK_1_8,
	JDK_VERMAX
};

enum Errors {
	ERROR_TRUNCATED_FILE,
	ERROR_CLASS_FILE_FORMAT,
	ERROR_CLASS_FILE_VERSION
};

#define JAVA_MAGIC 0xCAFEBABE
#define MAX_VERSION  (JDK_VERMAX - 1)

#define ConstantClass          7
#define ConstantField          9
#define ConstantMethod         10
#define ConstantInterface      11
#define ConstantString         8
#define ConstantInteger        3
#define ConstantFloat          4
#define ConstantLong           5
#define ConstantDouble         6
#define ConstantNameType       12
#define ConstantUTF8           1
#define ConstantMethodHandle   15
#define ConstantMethodType     16
#define ConstantInvokeDynamic  18
#endif

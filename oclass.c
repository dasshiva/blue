#include <stdlib.h>
#include "oclass.h"
#include <stdio.h>
#include <math.h>

#define log(...) printf(__VA_ARGS__);
struct ConstantPool {
};

struct Field {
};

struct Method {
};

struct Attributes {
};

struct ClassFile {
	u16 Minor;
	u16 Major;
	u16 ConstantPoolCount;
	struct ConstantPool* ConstantPool;
	u16 Access;
	u16 This;
	u16 Super;
	u16 InterfacesCount;
	u16* Interfaces;
	u16 FieldsCount;
	struct Field* Fields;
	u16 MethodsCount;
	struct Methods* Methods;
	u16 AttributesCount;
	struct Attributes* Attributes;
	u64 Length;
	u64 Offset;
	u8* File;
};

struct ClassFile* InitClass (u8* File, u64 Length) {
	if (!File || !Length) 
		return ((void*)-1);

	struct ClassFile* class = malloc(sizeof(struct ClassFile));
	if (!class) 
		return NULL;

	class->File = File;
	class->Length = Length;
	class->Offset = 0;

	return class;
}

static u8 ReadU8 (struct ClassFile* file) {
	return file->File[file->Offset++];
}

static u16 ReadU16 (struct ClassFile* file) {
	u16 r1 = ((u16)file->File[file->Offset]  << 8) | 
		file->File[file->Offset + 1];
	file->Offset += 2;
	return r1;
}

static u32 ReadU32 (struct ClassFile* file) {
	u32 r1 = ReadU16(file);
	u32 r2 = ReadU16(file);
	return (r1 << 16) | r2;
}

static u64 ReadU64 (struct ClassFile* file) {
	u64 r1 = ReadU32(file);
	u64 r2 = ReadU32(file);
	return (r1 << 32) | r2;
}

#define check_index(index, length) if (!index || index > length - 1) {\
	log("Invalid constant pool index = %d\n", index); \
	return -4; \
}

int ParseFile (struct ClassFile* file, u16 version) {
	if (!file) 
		return -1;
	if (version > MAX_VERSION) 
		return 0;
	if (ReadU32(file) != JAVA_MAGIC) 
		return -2;
	
	file->Minor = ReadU16(file); // skip minor version
	file->Major = ReadU16(file);
	if (file->Major > MAX_VERSION || file->Major > version)
		return -3;
	log("Class file version %d.%d\n", file->Major, file->Minor);

	file->ConstantPoolCount = ReadU16(file);
	log("Constant pool length = %d\n", file->ConstantPoolCount);
	u64 cp_begin = file->Offset;
	for (u8 index = 1; index < file->ConstantPoolCount; index++) {
		u8 tag = ReadU8(file);
		log("Parsing tag = %d at index = %d\n", tag, index);
		switch (tag) {
			case 1: // Utf8
				u16 length = ReadU16(file);
				if (file->Offset + length >= file->Length) {
					log("Utf8 String too long\n");
					return -4;
				}
				u8* utf8_string = file->File + file->Offset;
				file->Offset += length;
				break;

			case 3: // Integer
				int ele = (int) ReadU32(file);
				break;
			
			case 4: // Float
				int bits = (int) ReadU32(file);
				float res = 0.0f;
				if (bits == 0x7f800000)
					res = INFINITY;
				else if (bits == 0xff800000)
					res = -INFINITY;
				else if ((bits > 0x7f800000 && bits < 0x7fffffff) ||
						(bits > 0xff800001 && bits < 0xffffffff))
					res = NAN;
				else {
					int s = ((bits >> 31) == 0) ? 1 : -1;
					int e = ((bits >> 23) & 0xff);
					int m = (e == 0) ? (bits & 0x7fffff) << 1 :
						(bits & 0x7fffff) | 0x800000;
					res = s * m * pow(2, e - 150);
				}
				break;

			case 5: // Long
				int high = (int) ReadU32(file);
				int low = (int) ReadU32(file);
				long ret = ((long) high << 32) | low;
				index += 1;
				break;

			case 6: // Double
				int dhigh = (int) ReadU32(file);
                                int dlow = (int) ReadU32(file);
                                long dbits = ((long) dhigh << 32) | dlow;
				double dres = 0.0;

				if (dbits == 0x7ff0000000000000L) 
					dres = INFINITY;
				else if (dbits == 0xfff0000000000000L)
					dres = -INFINITY;
				else if ((dbits > 0x7ff0000000000000L && 
						dbits < 0x7fffffffffffffffL) ||
						(dbits > 0xfff0000000000000L &&
						 dbits < 0xffffffffffffffffL))
					dres = NAN;
				else {
					int s = ((dbits >> 63) == 0) ? 1 : -1;
					int e = (int)((dbits >> 52) & 0x7ffL);
					long m = (e == 0) ? (dbits & 0xfffffffffffffL) << 1 :
						(dbits & 0xfffffffffffffL) | 0x10000000000000L;
					dres = s * m * pow(2, e - 1075);
				}
				index += 1;
				break;

			case 7: // Class
				u16 class = ReadU16(file);
				check_index(class, file->ConstantPoolCount);
				break;

			case 8: // String
				u16 string = ReadU16(file);
				check_index(string, file->ConstantPoolCount);
				break;

			case 9:  // FieldRef
			case 10: // MethodRef
			case 11: // InterfaceMethodRef
				u16 base_class = ReadU16(file);
				u16 name_type = ReadU16(file);
				check_index(base_class, file->ConstantPoolCount);
				check_index(name_type, file->ConstantPoolCount);
				break;

			case 12: // NameAndType
				u16 name = ReadU16(file);
				u16 type = ReadU16(file);
				check_index(name, file->ConstantPoolCount);
				check_index(type, file->ConstantPoolCount);
				break;

			case 15: // MethodHandle
				 u8 ref_kind = ReadU8(file);
				 u16 ref_index = ReadU8(file);
				 check_index(ref_index, file->ConstantPoolCount);
				 break;

			case 16: // MethodType
				 u16 desc = ReadU16(file);
				 check_index(desc, file->ConstantPoolCount);
				 break;

			case 17: // InvokeDynamic
				 u16 bootstrap = ReadU16(file);
				 u16 nt_index = ReadU16(file);
				 check_index(nt_index, file->ConstantPoolCount);
				 break;

			default: 
				log("Unknown constant pool tag = %d\n", tag);
				return -4;
		}
	}
	u64 cp_end = file->Offset;
	log("Constant Pool Length = %ld\n", cp_end - cp_begin + 1);

	file->Access = ReadU16(file);

	file->This = ReadU16(file);
	check_index(file->This, file->ConstantPoolCount);
	file->Super = ReadU16(file);
	check_index(file->Super, file->ConstantPoolCount);

	file->InterfacesCount = ReadU16(file);
	if (file->Offset + file->InterfacesCount * sizeof(u16) >= file->Length) {
		log("There are more interfaces than the file can possibly have\n");
		return -4;
	}
	file->Interfaces = (u16*)(file->File + file->Offset);
	log("Number of interfaces = %d\n", file->InterfacesCount);

	file->FieldsCount = ReadU16(file);
	if (file->Offset + file->FieldsCount * sizeof(u16) >= file->Length) {
                log("There are more fields than the file can possibly have\n");
                return -4;
        }
        file->Fields = (struct Field*)(file->File + file->Offset);
	log("Number of fields = %d\n", file->FieldsCount);

	file->MethodsCount = ReadU16(file);
	if (file->Offset + file->MethodsCount * sizeof(u16) >= file->Length) {
                log("There are more methods than the file can possibly have\n");
                return -4;
        }
        file->Methods = (struct Methods*)(file->File + file->Offset);
	log("Number of methods = %d\n", file->MethodsCount);

	return 1;
}

#include <stdlib.h>
#include <math.h>
#include "oclass.h"
#include "strtab.h"
#include "utils.h"
#include <stdio.h>
#include "precomp.h"

struct offset {
	u32 low;
	u32 high;
	u32 flags;
};

struct Attributes {
	u8 tag; // Identifies the attribute
};

struct _MethodOrField {
	u16 Access;
	u16 Name;
	u16 Descriptor;
	u16 AttributeCount;
	struct Attributes* Attributes;
	struct offset* AttributeMap;
};

typedef struct _MethodOrField Field;
typedef struct _MethodOrField Method;

struct ClassFile {
	u16 Minor;
	u16 Major;
	u16 ConstantPoolCount;
	struct ConstantPool* ConstantPool;
	struct offset* ConstantPoolMap;
	u16 Access;
	u16 This;
	u16 Super;
	u16 InterfacesCount;
	u16* Interfaces;
	u16 FieldsCount;
	Field* Fields;
	u16 MethodsCount;
	Method* Methods;
	u16 AttributesCount;
	struct Attributes* Attributes;
	struct offset* AttributesMap;
	u64 Length;
	u64 Offset;
	u8* File;
	struct StringTable* strtable;
	struct ConstantTable* constable;
	void (*Handler) (u8);
};

struct ClassFile* InitClass (u8* File, u64 Length, void (*Handler) (u8)) {
	if (!File || !Length || !Handler) 
		return ((void*)-1);

	struct ClassFile* class = malloc(sizeof(struct ClassFile));
	if (!class) 
		return NULL;

	class->File = File;
	class->Length = Length;
	class->Offset = 0;
	class->Handler = Handler;

	return class;
}

static u8 ReadU8 (struct ClassFile* file) {
	if (file->Offset >= file->Length) {
		log("Read to offset greater than size of file\n");
		file->Handler(ERROR_TRUNCATED_FILE);
		return 0xFF;
	}

	return file->File[file->Offset++];
}

static u16 ReadU16 (struct ClassFile* file) {
	if (file->Offset >= file->Length || file->Offset + 1 >= file->Length) {
		log("Read to offset greater than size of file \n");
		file->Handler(ERROR_TRUNCATED_FILE);
		return 0xFFFF;
	}

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

#define check_index(index, length, file) if (!index || index > length - 1) {\
	log("Invalid constant pool index = %d\n", index); \
	file->Handler(ERROR_CLASS_FILE_FORMAT); \
	return -1; \
}

#define validate_index(index, off, expected, this, file) if (off[index].flags != expected) {\
	log("Invalid constant pool index reference at index %d\n", this); \
	file->Handler(ERROR_CLASS_FILE_FORMAT); \
	return -1; \
}

#define fix_endian(file, off, type, val) \
	*((type*)(file + off - sizeof(type))) = val;


static int ParseConstantPool (struct ClassFile* file) {
	file->ConstantPoolCount = ReadU16(file);
	log("Constant pool length = %d\n", file->ConstantPoolCount);

	u32 cp_start = file->Offset;
	file->strtable = MakeStringTable();
	file->constable = MakeConstantTable();
	file->ConstantPoolMap = malloc(sizeof(struct offset) * file->ConstantPoolCount);

	for (u8 index = 1; index < file->ConstantPoolCount; index++) {
		u8 tag = ReadU8(file);
		file->ConstantPoolMap[index].low = file->Offset;
		file->ConstantPoolMap[index].flags = tag;
		log("Parsing tag = %d at index = %d\n", tag, index);
		switch (tag) {
			case 1: // Utf8
				u16 length = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, length);
				if (file->Offset + length >= file->Length) {
					log("Utf8 String too long\n");
					return -4;
				}
				u8* utf8_string = file->File + file->Offset;
				if (!AppendString(file->strtable, utf8_string, length, index)) {
					file->Handler(ERROR_UTF8_ENCODING);
					return 0;
				}
				//printf("0x%lx", GetStringHash(file->strtable, index));
				file->Offset += length;
				break;

			case 3: // Integer
				int ele = (int) ReadU32(file);
				fix_endian(file->File, file->Offset, u32, ele);
				AppendConstant(file->constable, index, 32, ele);
				break;
			
			case 4: // Float
				int bits = (int) ReadU32(file);
				AppendConstant(file->constable, index, 32, bits);
				break;

			case 5: // Long
				int high = (int) ReadU32(file);
				int low = (int) ReadU32(file);
				long ret = ((long) high << 32) | low;
				file->ConstantPoolMap[index].high = file->Offset - 1;
				index += 1;
				file->ConstantPoolMap[index].flags = 0;
				fix_endian(file->File, file->Offset, u64, ret);
				AppendConstant(file->constable, index, 64, ret);
				continue;

			case 6: // Double
				int dhigh = (int) ReadU32(file);
                                int dlow = (int) ReadU32(file);
                                long dbits = ((long) dhigh << 32) | dlow;
				file->ConstantPoolMap[index].high = file->Offset - 1;
				index += 1;
                                file->ConstantPoolMap[index].flags = 0;
				AppendConstant(file->constable, index, 64, dbits);
				continue;

			case 7: // Class
				u16 class = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, class);
				check_index(class, file->ConstantPoolCount, file);
				break;

			case 8: // String
				u16 string = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, string);
				check_index(string, file->ConstantPoolCount, file);
				break;

			case 9:  // FieldRef
			case 10: // MethodRef
			case 11: // InterfaceMethodRef
				u16 base_class = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, base_class);
				u16 name_type = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, name_type);
				check_index(base_class, file->ConstantPoolCount, file);
				check_index(name_type, file->ConstantPoolCount, file);
				break;

			case 12: // NameAndType
				u16 name = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, name);
				u16 type = ReadU16(file);
				fix_endian(file->File, file->Offset, u16, type);
				check_index(name, file->ConstantPoolCount, file);
				check_index(type, file->ConstantPoolCount, file);
				break;

			case 15: // MethodHandle
				 u8 ref_kind = ReadU8(file);
				 if (ref_kind < 1 || ref_kind > 9) {
					 log("Reference kind is unknown\n");
					 file->Handler(ERROR_CLASS_FILE_FORMAT);
					 return -4;
				 }
				 u16 ref_index = ReadU16(file);
				 fix_endian(file->File, file->Offset, u16, ref_index);
				 check_index(ref_index, file->ConstantPoolCount, file);
				 break;

			case 16: // MethodType
				 u16 desc = ReadU16(file);
				 fix_endian(file->File, file->Offset, u16, desc);
				 check_index(desc, file->ConstantPoolCount, file);
				 break;

			case 17: // InvokeDynamic
				 u16 bootstrap = ReadU16(file);
				 fix_endian(file->File, file->Offset, u16, bootstrap);
				 u16 nt_index = ReadU16(file);
				 fix_endian(file->File, file->Offset, u16, nt_index);
				 check_index(nt_index, file->ConstantPoolCount, file);
				 break;

			default: 
				log("Unknown constant pool tag = %d\n", tag);
				return -1;
		}

		file->ConstantPoolMap[index].high = file->Offset - 1;
	}

	return 1;

}

static int ValidateConstantPool (struct ClassFile* file) {
	for (int i = 1; i < file->ConstantPoolCount; i++) {
		switch (file->ConstantPoolMap[i].flags) {
			case 7:
			case 8:
			case 16:
			{
				u16 cl = *(((u16*)(file->File + file->ConstantPoolMap[i].low)));
				validate_index(cl, file->ConstantPoolMap, ConstantUTF8, i, file);
				break;
			}
			case 9:
			case 10:
			case 11: 
			{
				u16 cl = *(((u16*)(file->File + file->ConstantPoolMap[i].low)));
				u16 name = *(((u16*)(file->File + file->ConstantPoolMap[i].low + 2)));
				validate_index(cl, file->ConstantPoolMap, ConstantClass, i, file);
				validate_index(name, file->ConstantPoolMap, ConstantNameType, i, file);
				break;
			}
			case 12:
			{
				u16 name = *(((u16*)(file->File + file->ConstantPoolMap[i].low)));
                                u16 desc = *(((u16*)(file->File + file->ConstantPoolMap[i].low + 2)));
                                validate_index(name, file->ConstantPoolMap, ConstantUTF8, i, file);
                                validate_index(desc, file->ConstantPoolMap, ConstantUTF8, i, file);
                                break;
			}
			case 15:
			{
				u16 kind = *(((u16*)(file->File + file->ConstantPoolMap[i].low)));
                                u16 index = *(((u16*)(file->File + file->ConstantPoolMap[i].low + 2)));
				if (kind >= 1 && kind <= 4) 
					validate_index(kind, file->ConstantPoolMap, ConstantField, i, file)
				else if (kind >= 5 && kind <= 8)
					validate_index(kind, file->ConstantPoolMap, ConstantMethod, i, file)
				else
					validate_index(kind, file->ConstantPoolMap, ConstantInterface, i, file)
				break;
			}
			case 17:
			{
				u16 idx = *(((u16*)(file->File + file->ConstantPoolMap[i].low)));
                                u16 name = *(((u16*)(file->File + file->ConstantPoolMap[i].low + 2)));
                                validate_index(name, file->ConstantPoolMap, ConstantNameType, i, file);
				break;
			}
			default: // Nothing to do if not the above tags

		}
	}

	return 1;
}

static int ParseInterfaces (struct ClassFile* file) {
	file->InterfacesCount = ReadU16(file);
	if (file->Offset + file->InterfacesCount * sizeof(u16) >= file->Length) {
		log("There are more interfaces than the file can possibly have\n");
		file->Handler(ERROR_CLASS_FILE_FORMAT);
		return -1;
	}

	file->Interfaces = (u16*)(file->File + file->Offset);
	log("Number of interfaces = %d\n", file->InterfacesCount);
	for (int i = 0; i < file->InterfacesCount; i++) {
		u32 idx = ReadU16(file);
		check_index(idx, file->ConstantPoolCount, file);
		if (file->ConstantPoolMap[idx].flags != ConstantClass) {
			log("interface index is not a class\n");
			file->Handler(ERROR_CLASS_FILE_FORMAT);
			return -1;
		}
	}

	return 1;
}

static int ParseFields (struct ClassFile* file) {
	file->FieldsCount = ReadU16(file);
	if (file->Offset + file->FieldsCount * sizeof(u16) >= file->Length) {
                log("There are more fields than the file can possibly have\n");
		file->Handler(ERROR_CLASS_FILE_FORMAT);
                return -1;
        }

        file->Fields = (Field*)(file->File + file->Offset);
	log("Number of fields = %d\n", file->FieldsCount);

	for (int i = 0; i < file->FieldsCount; i++) {
		Field this = file->Fields[i];
		this.Access = ReadU16(file);
		this.Name = ReadU16(file);
		this.Descriptor = ReadU16(file);
		check_index(this.Name, file->ConstantPoolCount, file);
		check_index(this.Descriptor, file->ConstantPoolCount, file);
		validate_index(this.Name, file->ConstantPoolMap, ConstantUTF8, i, file);
		validate_index(this.Descriptor, file->ConstantPoolMap, ConstantUTF8, i, file);
		this.AttributeCount = ReadU16(file);
		this.AttributeMap = malloc(sizeof(struct offset) * 
				this.AttributeCount);

		for (int j = 0; j < this.AttributeCount; j++) {
			u16 name = ReadU16(file);
			check_index(name, file->ConstantPoolCount, file);
			validate_index(name, file->ConstantPoolMap, ConstantUTF8, i, file);
			u64 hash = GetStringHash(file->strtable, name);
			u32 size = ReadU32(file);
			if (file->Offset + size >= file->Length) {
				log("Attribute too large to fit\n");
				file->Handler(ERROR_CLASS_FILE_FORMAT);
				return -1;
			}

			switch (hash) {
				case Synthetic:
				case Deprecated: break;
				// TODO: Implement these properly
				case ConstantValue: {
					u16 index = ReadU16(file);
					check_index(index, file->ConstantPoolCount, file);
					break;
				}

				case Signature: {
					u16 index = ReadU16(file);
					check_index(index, file->ConstantPoolCount, file);
					break;
				}
				default: {
					log("Unknown field attribute %s\n", GetString(file->strtable, name));
					file->Offset += size;
				}
			}
		}
	}

	return 1;
}

static int ParseMethods (struct ClassFile* file) {
	file->MethodsCount = ReadU16(file);
	if (file->Offset + file->MethodsCount * sizeof(u16) >= file->Length) {
                log("There are more methods than the file can possibly have\n");
                return -1;
        }
        file->Methods = (Method*)(file->File + file->Offset);
	log("Number of methods = %d\n", file->MethodsCount); 

	for (int i = 0; i < file->MethodsCount; i++) {
		Method this = file->Methods[i];
		this.Access = ReadU16(file);
		this.Name = ReadU16(file);
		this.Descriptor = ReadU16(file);
		check_index(this.Name, file->ConstantPoolCount, file);
		check_index(this.Descriptor, file->ConstantPoolCount, file);
		validate_index(this.Name, file->ConstantPoolMap, ConstantUTF8, i, file);
		validate_index(this.Descriptor, file->ConstantPoolMap, ConstantUTF8, i, file);
		this.AttributeCount = ReadU16(file);
		this.AttributeMap = malloc(sizeof(struct offset) * 
				this.AttributeCount);

		for (int j = 0; j < this.AttributeCount; j++) {
			printf("%s\n", GetString(file->strtable, this.Name));
			u16 name = ReadU16(file);
			check_index(name, file->ConstantPoolCount, file);
			validate_index(name, file->ConstantPoolMap, ConstantUTF8, i, file);
			u64 hash = GetStringHash(file->strtable, name);
			u32 size = ReadU32(file);
			if (file->Offset + size >= file->Length) {
				log("Attribute too large to fit\n");
				file->Handler(ERROR_CLASS_FILE_FORMAT);
				return -1;
			}

			switch (hash) {
				case Synthetic:
				case Deprecated: break;
				// TODO: Implement these properly
				case Signature: {
					u16 index = ReadU16(file);
					check_index(index, file->ConstantPoolCount, file);
					break;
				}
				default: {
					log("Unknown method attribute %s\n", GetString(file->strtable, name));
					file->Offset += size;
				}
			}
		}
	}

	return 1;
}

int ParseAttributes (struct ClassFile* file) {
	file->AttributesCount = ReadU16(file);
	file->Attributes = (struct Attributes*)(file->File + file->Offset);
	file->AttributesMap = malloc(sizeof(struct offset) * file->AttributesCount);
	
	for (int i = 0; i < file->AttributesCount; i++) {
		u16 name = ReadU16(file);
		check_index(name, file->ConstantPoolCount, file);
		validate_index(name, file->ConstantPoolMap, ConstantUTF8, i, file);
		u64 hash = GetStringHash(file->strtable, name);
		u32 size = ReadU32(file);

		if (file->Offset + size > file->Length) {
			//printf("%ld %ld\n", file->Offset + 2, file->Length);
			log("Attribute too large to fit\n");
			file->Handler(ERROR_CLASS_FILE_FORMAT);
			return -1;
		}

		switch (hash) {
			case Synthetic:
			case Deprecated: break;
			// TODO: Implement Signature properly
			case Signature: {
				u16 index = ReadU16(file);
				check_index(index, file->ConstantPoolCount, file);
				break;
			}
			case SourceFile: {
				u16 source = ReadU16(file);
				check_index(source, file->ConstantPoolCount, file);
				validate_index(source, file->ConstantPoolMap, ConstantUTF8, i, file);
				break;
			}
			default: {
				log("Unknown method attribute %s\n", GetString(file->strtable, name));
				file->Offset += size;
			}
		}
	}
		
	return 1;
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
	if (file->Major > MAX_VERSION || file->Major > version) {
		file->Handler(ERROR_CLASS_FILE_VERSION);
		return -3;
	}
	log("Class file version %d.%d\n", file->Major, file->Minor);

	if (ParseConstantPool(file) < 0) 
		return 0;

	if (ValidateConstantPool(file) < 0) 
		return 0;

	file->Access = ReadU16(file);
	file->This = ReadU16(file);
	check_index(file->This, file->ConstantPoolCount, file);
	file->Super = ReadU16(file);
	check_index(file->Super, file->ConstantPoolCount, file);

	if (ParseInterfaces(file) < 0)
		return 0;

	if (ParseFields(file) < 0) 
		return 0;

	if (ParseMethods(file) < 0)
		return 0;

	if (ParseAttributes(file) < 0)
		return 0;

	return 1;
}

#ifndef __STRTAB_H__
#define __STRTAB_H__

#include "types.h"

struct StringTable {
	u16 PoolIndex; // Constant Pool Index of String
	u64 Hash;  // FNV-1a Hash of String
	u8* String; // Null Terminated UTF8 String
	struct StringTable* next;
};
struct StringTable* MakeStringTable ();
int AppendString (struct StringTable* table, u8* string, u16 length, u16 pool_index);
u64 GetStringHash (struct StringTable* table, u16 index);
u8* GetString (struct StringTable* table, u16 index);
void FreeStringTable (struct StringTable* table);

struct ConstantTable {
	u16 PoolIndex;
	u64 data;
};
struct ConstantTable* MakeConstantTable ();
void AppendConstant (struct ConstantTable* table, u16 index);
void GetConstant (struct ConstantTable* table, u16 index);
void FreeConstantTable (struct ConstantTable* table);

#endif

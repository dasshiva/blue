#include <stdlib.h>
#include "hash.h"
#include "strtab.h"

struct StringTable* MakeStringTable () {
	struct StringTable* ret = malloc(sizeof(struct StringTable));
	ret->Hash = 0;
	ret->PoolIndex = 0;
	ret->String = 0;
	ret->next = NULL;
	return ret;
}

static u8* ConvertToUTF8 (u8* string, u16 length) {
	return NULL;
}

void AppendString (struct StringTable* table, u8* string, u16 length, u16 idx) {
	// Modified UTF-8 strings are either longer or equal in size
	// with an ordinary UTF-8 string (not counting the nul character)
	//
	// If the given string consists only of ASCII letters, its size is
	// equal to that of the ascii string we will mak her (and +1 for the
	// nul character). If the string has code points other than ASCII,
	// modified UTF-8 will use 3 bytes for codepoints betwenn U+0080 
	// to U+FFFF and 6 bytes for all others while UTF-8 never uses 
	// more than 4 bytes for a single code point. This allows the above
	// assumption to be true so we get away with mallocing length + 1
	// below. In case the string uses multibyte code points some space
	// is wasted on account of this but it should not hurt our memory
	// footprint that much
	
	if (!table) // Should never happen
		return;
	
	struct StringTable* append = NULL;
	if (!table->PoolIndex) {
		table->PoolIndex = idx;
		append = table;
	}
	else {
		while (!table->next) {
			table = table->next;
		}
		table->next = malloc(sizeof(struct StringTable));
		table->next->PoolIndex = idx;
		append = table->next;
	}

	append->next = NULL;
	append->String = ConvertToUTF8(string, length);
	append->Hash = Hash(append->String, length);
}

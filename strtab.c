#include <stdlib.h>
#include "utils.h"
#include "strtab.h"

struct StringTable* MakeStringTable () {
	struct StringTable* ret = malloc(sizeof(struct StringTable));
	ret->Hash = 0;
	ret->PoolIndex = 0;
	ret->String = 0;
	ret->next = NULL;
	return ret;
}

#define check_index(x, l, error) if ((x) >= (l)) { error = 1; break; }
static u8* ConvertToUTF8 (u8* string, u16 length) {
	u8* ret = malloc(sizeof(u8) * length + 1);
	u8 encoding_error = 0;
	// Remember that we haven't fixed endians for string in ParseFile
	// this means that the high bytes are first i.e we will see the
	// continuation bytes before the intial byte
	// While this doesn't effect only ASCII strings we need to be careful 
	// while parsing multibyte characters as only they pose a threat
	// Use check_index if you need to access any characters after the
	// current index to prevent buffer overflows
	//
	// Note that be careful while using check_index in nested loops as
	// it will only "break" out of the innermost loop
	u16 ptr = 0;
	for (u16 index = 0; index < length; index++) {
		u8 chr = string[index];

		// chr may not be null or in between 0xf0-0xff from the spec
		if (!chr || chr >= 0xf0) {
			encoding_error = 1;
			break;
		}

		// First up pure ASCII
		if (!(chr & (1 << 7))) {
			ret[ptr++] = chr;
			continue;
		}
		// Next Unicode 2 byte characters
		else if (chr >> 5 == 6) {
			u8 x = (chr & 0x1f) << 6;
			check_index(index + 1, length, encoding_error);
			u8 y = string[++index] & 0x3f;
			*((u16*)(ret + ptr)) = (u16)x + (u16)y;
			ptr += 2;
		}
		// Next Unicode 3 byte characters
		else if (chr >> 4 == 14) {
			u8 x = ((u8)chr & 0xf) << 12;
			check_index(index + 1, length, encoding_error);
			check_index(index + 2, length, encoding_error);
			u8 y = (string[++index] & 0x3f) << 6;
			u8 z = string[++index] & 0x3f;
			 *((u16*)(ret + ptr)) = (u16)x + (u16)y + (u16)z;
			 ptr += 2;
		}
		// JVM's own "two-times-three-byte format"
		else if (chr == 0xED) {
			check_index(index + 1, length, encoding_error);
			check_index(index + 2, length, encoding_error);
			check_index(index + 3, length, encoding_error);
			check_index(index + 4, length, encoding_error);
			check_index(index + 5, length, encoding_error);
			u8 v = string[++index];
			u8 w = string[++index];
			u8 x = string[++index];
			u8 y = string[++index];
			u8 z = string[++index];
			if (chr != x || (v >> 4 != 0b1010) || (w >> 6 != 0b10)
				|| (y >> 4 != 0b1011) || (z >> 6 != 0b10)) {
				encoding_error = 1;
				break;
			}
			u32 val = 0x10000 + ((v & 0x0f) << 16) + ((w & 0x3f) << 10)
			       	+ ((y & 0x0f) << 6) + (z & 0x3f);
			*((u32*)(ret + ptr)) = val;
			ptr += 4;
		}

		else {
			encoding_error = 1;
			break;
		}

	}

	if (encoding_error) {
		free(ret);
		return NULL;
	}

	ret[ptr] = '\0';
	return ret;
}

int AppendString (struct StringTable* table, u8* string, u16 length, u16 idx) {
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
		return 0;
	
	struct StringTable* append = NULL;
	if (!table->PoolIndex) {
		table->PoolIndex = idx;
		append = table;
	}
	else {
		while (table->next) {
			table = table->next;
		}
		table->next = malloc(sizeof(struct StringTable));
		table->next->PoolIndex = idx;
		append = table->next;
	}

	append->next = NULL;
	append->String = ConvertToUTF8(string, length);
	if (!append->String) {
		log("Failed to encode string\n");
		return 0;
	}
	append->Hash = Hash(append->String, length);
	return 1;
}

u64 GetStringHash (struct StringTable* table, u16 index) {
	while (table->next) {
		if (table->PoolIndex == index) 
			return table->Hash;
		table = table->next;
	}

	if (table->String) 
		return table->Hash;
	return 0;
}

u8* GetString (struct StringTable* table, u16 index) {
	while (table->next) {
		if (table->PoolIndex == index)
			return table->String;
		table = table->next;
	}

	if (table->String) // only 1 element
		return table->String;
	return NULL;
}

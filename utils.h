#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include "types.h"
static inline u64 Hash (u8* byte, u64 size) {
	u64 ret = 0xcbf29ce484222325; // FNV_Offset_basis
        u64 fnv_prime = 0x100000001b3; // FNV_prime                                                            
	while (size > 0) {
                ret = ret ^ byte[size - 1];
                ret *= fnv_prime;
                size--;                        
        }
        return ret;
}

#ifdef DEBUG
#define log(...) printf("LOG:" __VA_ARGS__);
#else
#define log(...)
#endif

#endif

// REQUIRES: arm-target-arch || armv6m-target-arch
// RUN: %clang_builtins %s %librt -o %t && %run %t

#include "int_lib.h"
#include <stdio.h>

#if __arm__
// Based on udivmodsi4_test.c

extern du_int __aeabi_uidivmod(su_int a, su_int b);

int test__aeabi_uidivmod(su_int a, su_int b,
						su_int expected_result, su_int expected_rem)
{
    du_int ret = __aeabi_uidivmod(a, b);
    // __aeabi_uidivmod actually returns a struct { quotient; remainder; }
    // using value_in_regs calling convention. Due to the ABI rules, struct
    // fields come in the same order regardless of endianness. However since
    // the result is received here as a 64-bit integer, in which endianness
    // does matter, the position of each component (quotient and remainder)
    // varies depending on endianness.
#  if _YUGA_BIG_ENDIAN
    su_int rem = ret & 0xFFFFFFFF;
    si_int result = ret >> 32;
#  else
    su_int rem = ret >> 32;
    si_int result = ret & 0xFFFFFFFF;
#  endif

    if (result != expected_result) {
        printf("error in __aeabi_uidivmod: %u / %u = %u, expected %u\n",
               a, b, result, expected_result);
		return 1;
	}
    if (rem != expected_rem) {
        printf("error in __aeabi_uidivmod: %u mod %u = %u, expected %u\n",
               a, b, rem, expected_rem);
		return 1;
	}

    return 0;
}
#endif


int main()
{
#if __arm__
    if (test__aeabi_uidivmod(0, 1, 0, 0))
        return 1;

    if (test__aeabi_uidivmod(2, 1, 2, 0))
        return 1;

	if (test__aeabi_uidivmod(19, 5, 3, 4))
        return 1;

	if (test__aeabi_uidivmod(0x80000000, 8, 0x10000000, 0))
        return 1;
 
 	if (test__aeabi_uidivmod(0x80000003, 8, 0x10000000, 3))
        return 1;
#else
    printf("skipped\n");
#endif

	return 0;
}

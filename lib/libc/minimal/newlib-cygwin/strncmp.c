/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 *   FUNCTION
 * <<strncmp>>---character string compare
 *
 * INDEX
 * strncmp
 *
 * SYNOPSIS
 * #include <string.h>
 * int strncmp(const char *<[a]>, const char * <[b]>, size_t <[length]>);
 *
 * DESCRIPTION
 * <<strncmp>> compares up to <[length]> characters
 * from the string at <[a]> to the string at <[b]>.
 *
 * RETURNS
 * If <<*<[a]>>> sorts lexicographically after <<*<[b]>>>,
 * <<strncmp>> returns a number greater than zero.  If the two
 * strings are equivalent, <<strncmp>> returns zero.  If <<*<[a]>>>
 * sorts lexicographically before <<*<[b]>>>, <<strncmp>> returns a
 * number less than zero.
 *
 * PORTABILITY
 * <<strncmp>> is ANSI C.
 *
 * <<strncmp>> requires no supporting OS subroutines.
 *
 * QUICKREF
 * strncmp ansi pure
 */

#include <string.h>
#include <limits.h>

/** Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) (((long)X & (sizeof(long) - 1)) | ((long)Y & (sizeof(long) - 1)))

/** DETECTNULL returns nonzero if (long)X contains a NULL byte. */
#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X)-0x01010101) & ~(X)&0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
#define DETECTNULL(X) (((X)-0x0101010101010101) & ~(X)&0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

int strncmp(const char *s1, const char *s2, size_t n)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
	if (n == 0) {
		return 0;
	}

	while (n-- != 0 && *s1 == *s2) {
		if (n == 0 || *s1 == '\0') {
			break;
		}
		s1++;
		s2++;
	}

	return (*(unsigned char *)s1) - (*(unsigned char *)s2);
#else
	unsigned long *a1;
	unsigned long *a2;

	if (n == 0) {
		return 0;
	}

	/** If s1 or s2 are unaligned, then compare bytes. */
	if (!UNALIGNED(s1, s2)) {
		/** If s1 and s2 are word-aligned, compare them a word at a time. */
		a1 = (unsigned long *)s1;
		a2 = (unsigned long *)s2;
		while (n >= sizeof(long) && *a1 == *a2) {
			n -= sizeof(long);

			/**
			 * If we've run out of bytes or hit a null, return zero
			 * since we already know *a1 == *a2.
			 */
			if (n == 0 || DETECTNULL(*a1)) {
				return 0;
			}

			a1++;
			a2++;
		}

		/** A difference was detected in last few bytes of s1, so search bytewise */
		s1 = (char *)a1;
		s2 = (char *)a2;
	}

	while (n-- > 0 && *s1 == *s2) {
		/**
		 * If we've run out of bytes or hit a null, return zero
		 * since we already know *s1 == *s2.
		 */
		if (n == 0 || *s1 == '\0') {
			return 0;
		}
		s1++;
		s2++;
	}

	return (*(unsigned char *)s1) - (*(unsigned char *)s2);
#endif /* not PREFER_SIZE_OVER_SPEED */
}

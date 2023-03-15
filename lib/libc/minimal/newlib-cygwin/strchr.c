/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * FUNCTION
 *	<<strchr>>---search for character in string
 *
 * INDEX
 *	strchr
 *
 * SYNOPSIS
 *	#include <string.h>
 *	char * strchr(const char *<[string]>, int <[c]>);
 *
 * DESCRIPTION
 *	This function finds the first occurrence of <[c]> (converted to
 *	a char) in the string pointed to by <[string]> (including the
 *	terminating null character).
 *
 * RETURNS
 *	Returns a pointer to the located character, or a null pointer
 *	if <[c]> does not occur in <[string]>.
 *
 * PORTABILITY
 * <<strchr>> is ANSI C.
 *
 * <<strchr>> requires no supporting OS subroutines.
 *
 * QUICKREF
 *	strchr ansi pure
 */

#include <string.h>
#include <limits.h>

/** Nonzero if X is not aligned on a "long" boundary.  */
#define UNALIGNED(X) ((long)X & (sizeof(long) - 1))

/** How many bytes are loaded each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof(long))

#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X)-0x01010101) & ~(X)&0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/** Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X)-0x0101010101010101) & ~(X)&0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

/**
 * DETECTCHAR returns nonzero if (long)X contains the byte used
 * to fill (long)MASK.
 */
#define DETECTCHAR(X, MASK) (DETECTNULL(X ^ MASK))

char *strchr(const char *s1, int i)
{
	const unsigned char *s = (const unsigned char *)s1;
	unsigned char c = i;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
	unsigned long *aligned_addr;
	unsigned long mask, j;

	/** Special case for finding 0.  */
	if (!c) {
		while (UNALIGNED(s)) {
			if (!*s) {
				return (char *)s;
			}
			s++;
		}
		/** Operate a word at a time.  */
		aligned_addr = (unsigned long *)s;
		while (!DETECTNULL(*aligned_addr)) {
			aligned_addr++;
		}
		/** Found the end of string.  */
		s = (const unsigned char *)aligned_addr;
		while (*s) {
			s++;
		}
		return (char *)s;
	}

	/** All other bytes. Align the pointer, then search a long at a time.  */
	while (UNALIGNED(s)) {
		if (!*s) {
			return NULL;
		}
		if (*s == c) {
			return (char *)s;
		}
		s++;
	}

	mask = c;
	for (j = 8; j < LBLOCKSIZE * 8; j <<= 1) {
		mask = (mask << j) | mask;
	}

	aligned_addr = (unsigned long *)s;
	while (!DETECTNULL(*aligned_addr) && !DETECTCHAR(*aligned_addr, mask)) {
		aligned_addr++;
	}

	/**
	 * The block of bytes currently pointed to by aligned_addr
	 * contains either a null or the target char, or both. We
	 * catch it using the bytewise search.
	 */

	s = (unsigned char *)aligned_addr;

#endif /* not PREFER_SIZE_OVER_SPEED */

	while (*s && *s != c) {
		s++;
	}
	if (*s == c) {
		return (char *)s;
	}

	return NULL;
}

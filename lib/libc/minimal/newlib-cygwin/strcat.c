/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * FUNCTION
 * <<strcat>>---concatenate strings
 *
 * INDEX
 * strcat
 *
 * SYNOPSIS
 * #include <string.h>
 * char *strcat(char *restrict <[dst]>, const char *restrict <[src]>);
 *
 * DESCRIPTION
 * <<strcat>> appends a copy of the string pointed to by <[src]>
 * (including the terminating null character) to the end of the
 * string pointed to by <[dst]>.  The initial character of
 * <[src]> overwrites the null character at the end of <[dst]>.
 *
 * RETURNS
 * This function returns the initial value of <[dst]>
 *
 * PORTABILITY
 * <<strcat>> is ANSI C.
 *
 * <<strcat>> requires no supporting OS subroutines.
 *
 * QUICKREF
 * strcat ansi pure
 */

#include <string.h>
#include <limits.h>

/** Nonzero if X is aligned on a "long" boundary.  */
#define ALIGNED(X) (((long)X & (sizeof(long) - 1)) == 0)

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

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

/**SUPPRESS 560*/
/**SUPPRESS 530*/

char *strcat(char *__restrict s1, const char *__restrict s2)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
	char *s = s1;

	while (*s1) {
		s1++;
	}

	while ((*s1++ = *s2++)) {
		;
	}
	return s;
#else
	char *s = s1;

	/** Skip over the data in s1 as quickly as possible.  */
	if (ALIGNED(s1)) {
		unsigned long *aligned_s1 = (unsigned long *)s1;

		while (!DETECTNULL(*aligned_s1)) {
			aligned_s1++;
		}

		s1 = (char *)aligned_s1;
	}

	while (*s1) {
		s1++;
	}

	/**
	 * s1 now points to the its trailing null character, we can
	 * just use strcpy to do the work for us now.
	 *
	 * ?!? We might want to just include strcpy here.
	 * Also, this will cause many more unaligned string copies because
	 * s1 is much less likely to be aligned.  I don't know if its worth
	 * tweaking strcpy to handle this better.
	 */
	strcpy(s1, s2);

	return s;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

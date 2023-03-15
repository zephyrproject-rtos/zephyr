/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * FUNCTION
 * <<strncpy>>---counted copy string
 *
 * INDEX
 * strncpy
 *
 * SYNOPSIS
 * #include <string.h>
 * char *strncpy(char *restrict <[dst]>, const char *restrict <[src]>,
 * size_t <[length]>);
 *
 * DESCRIPTION
 * <<strncpy>> copies not more than <[length]> characters from
 * the string pointed to by <[src]> (including the terminating
 * null character) to the array pointed to by <[dst]>.  If the
 * string pointed to by <[src]> is shorter than <[length]>
 * characters, null characters are appended to the destination
 * array until a total of <[length]> characters have been
 * written.
 *
 * RETURNS
 * This function returns the initial value of <[dst]>.
 *
 * PORTABILITY
 * <<strncpy>> is ANSI C.
 *
 * <<strncpy>> requires no supporting OS subroutines.
 *
 * QUICKREF
 * strncpy ansi pure
 */

#include <string.h>
#include <limits.h>

/**SUPPRESS 560*/
/**SUPPRESS 530*/

/** Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) (((long)X & (sizeof(long) - 1)) | ((long)Y & (sizeof(long) - 1)))

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

#define TOO_SMALL(LEN) ((LEN) < sizeof(long))

char *strncpy(char *__restrict dst0, const char *__restrict src0, size_t count)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
	char *dscan;
	const char *sscan;

	dscan = dst0;
	sscan = src0;
	while (count > 0) {
		--count;
		if ((*dscan++ = *sscan++) == '\0') {
			break;
		}
	}
	while (count-- > 0) {
		*dscan++ = '\0';
	}

	return dst0;
#else
	char *dst = dst0;
	const char *src = src0;
	long *aligned_dst;
	const long *aligned_src;

	/** If SRC and DEST is aligned and count large enough, then copy words.  */
	if (!UNALIGNED(src, dst) && !TOO_SMALL(count)) {
		aligned_dst = (long *)dst;
		aligned_src = (long *)src;

		/**
		 * SRC and DEST are both "long int" aligned, try to do "long int"
		 * sized copies.
		 */
		while (count >= sizeof(long) && !DETECTNULL(*aligned_src)) {
			count -= sizeof(long);
			*aligned_dst++ = *aligned_src++;
		}

		dst = (char *)aligned_dst;
		src = (char *)aligned_src;
	}

	while (count > 0) {
		--count;
		if ((*dst++ = *src++) == '\0') {
			break;
		}
	}

	while (count-- > 0) {
		*dst++ = '\0';
	}

	return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

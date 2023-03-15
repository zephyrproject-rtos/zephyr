/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * FUNCTION
 * <<strrchr>>---reverse search for character in string
 *
 * INDEX
 * strrchr
 *
 * SYNOPSIS
 * #include <string.h>
 * char * strrchr(const char *<[string]>, int <[c]>);
 *
 * DESCRIPTION
 * This function finds the last occurrence of <[c]> (converted to
 * a char) in the string pointed to by <[string]> (including the
 * terminating null character).
 *
 * RETURNS
 * Returns a pointer to the located character, or a null pointer
 * if <[c]> does not occur in <[string]>.
 *
 * PORTABILITY
 * <<strrchr>> is ANSI C.
 *
 * <<strrchr>> requires no supporting OS subroutines.
 *
 * QUICKREF
 * strrchr ansi pure
 */

#include <string.h>

char *strrchr(const char *s, int i)
{
	const char *last = NULL;
	char c = i;

	if (c) {
		while ((s = strchr(s, c))) {
			last = s;
			s++;
		}
	} else {
		last = strchr(s, c);
	}

	return (char *)last;
}

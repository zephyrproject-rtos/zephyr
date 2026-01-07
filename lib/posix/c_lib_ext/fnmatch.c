/* SPDX-License-Identifier: BSD-3-Clause */

/*    $NetBSD: fnmatch.c,v 1.26 2014/10/12 22:32:33 christos Exp $    */

/*
 * Copyright (c) 1989, 1993, 1994
 *    The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Copyright (c) 2011 The FreeBSD Foundation
 * All rights reserved.
 * Portions of this software were developed by David Chisnall
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/posix/fnmatch.h>
#include <zephyr/toolchain.h>

#define EOS '\0'

#ifndef FNM_NORES
#define FNM_NORES 3
#endif

#ifndef FNM_LEADING_DIR
#define FNM_LEADING_DIR 0x08
#endif

#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0x10
#endif

#define FOLDCASE(ch, flags) foldcase((unsigned char)(ch), (flags))

#define RANGE_ERROR   (-1)
#define RANGE_NOMATCH 1
#define RANGE_MATCH   0

/* POSIX character class matching was missing from the BSD version. This was added in 2025 */
static int rangematch_cc(const char **pattern, int ch)
{
	typedef unsigned long long ull;

	/*
	 * [:alnum:] et al are 9 characters. [:xdigits:] is 10.
	 * If we first check for the leading "[:", then we have at most 8 remaining characters to
	 * compare. Rather than using string comparison, encode the 8 characters into an integer and
	 * compare to a precomputed constants. This likely only works for the "C" locale.
	 */
#define FNM_CC5(a, b, c, d, e)                                                                     \
	(((ull)(a) << 48) | ((ull)(b) << 40) | ((ull)(c) << 32) | ((ull)(d) << 24) |               \
	 ((ull)(e) << 16) | ((ull)':' << 8) | ((ull)']' << 0))
#define FNM_CC6(a, b, c, d, e, f)                                                                  \
	(((ull)(a) << 56) | ((ull)(b) << 48) | ((ull)(c) << 40) | ((ull)(d) << 32) |               \
	 ((ull)(e) << 24) | ((ull)(f) << 16) | ((ull)':' << 8) | ((ull)']' << 0))

	ull key;
	int ret;
	const char *p = *pattern;

	/* check the leading "[:" */
	if ((p[0] != '[') || (p[1] != ':')) {
		return RANGE_ERROR;
	}

	/* encode the remaining characters into a 64-bit integer */
	for (p += 2, key = 0; *p != EOS; ++p) {
		if (*(p - 1) == ']') {
			break;
		}

		key <<= 8;
		key |= (unsigned char)(*p);
	}

	switch (key) {
	case FNM_CC5('a', 'l', 'n', 'u', 'm'):
		ret = !isalnum(ch);
		break;
	case FNM_CC5('a', 'l', 'p', 'h', 'a'):
		ret = !isalpha(ch);
		break;
	case FNM_CC5('b', 'l', 'a', 'n', 'k'):
		ret = !isblank(ch);
		break;
	case FNM_CC5('c', 'n', 't', 'r', 'l'):
		ret = !iscntrl(ch);
		break;
	case FNM_CC5('d', 'i', 'g', 'i', 't'):
		ret = !isdigit(ch);
		break;
	case FNM_CC5('g', 'r', 'a', 'p', 'h'):
		ret = !isgraph(ch);
		break;
	case FNM_CC5('l', 'o', 'w', 'e', 'r'):
		ret = !islower(ch);
		break;
	case FNM_CC5('p', 'r', 'i', 'n', 't'):
		ret = !isprint(ch);
		break;
	case FNM_CC5('p', 'u', 'n', 'c', 't'):
		ret = !ispunct(ch);
		break;
	case FNM_CC5('s', 'p', 'a', 'c', 'e'):
		ret = !isspace(ch);
		break;
	case FNM_CC5('u', 'p', 'p', 'e', 'r'):
		ret = !isupper(ch);
		break;
	case FNM_CC6('x', 'd', 'i', 'g', 'i', 't'):
		ret = !isxdigit(ch);
		break;
	default:
		return RANGE_ERROR;
	}

	*pattern = p;
	return ret;
}

static inline int foldcase(int ch, int flags)
{
	if (((flags & FNM_CASEFOLD) != 0) && isupper(ch)) {
		return tolower(ch);
	}

	return ch;
}

static int rangematch(const char **pattern, char test, int flags)
{
	char c, c2;
	int negate, ok;
	const char *origpat;
	const char *pat = *pattern;

	/*
	 * A bracket expression starting with an unquoted circumflex
	 * character produces unspecified results (IEEE 1003.2-1992,
	 * 3.13.2).  This implementation treats it like '!', for
	 * consistency with the regular expression syntax.
	 * J.T. Conklin (conklin@ngai.kaleida.com)
	 */
	negate = (*pat == '!' || *pat == '^');
	if (negate) {
		++pat;
	}

	test = FOLDCASE(test, flags);

	/*
	 * A right bracket shall lose its special meaning and represent
	 * itself in a bracket expression if it occurs first in the list.
	 * -- POSIX.2 2.8.3.2
	 */
	ok = 0;
	origpat = pat;
	for (;;) {
		if (*pat == ']' && pat > origpat) {
			pat++;
			break;
		} else if (*pat == '\0') {
			return RANGE_ERROR;
		} else if (*pat == '/' && (flags & FNM_PATHNAME)) {
			return RANGE_NOMATCH;
		} else if (*pat == '\\' && !(flags & FNM_NOESCAPE)) {
			pat++;
		} else {
			switch (rangematch_cc(&pat, test)) {
			case RANGE_ERROR:
				/* not a character class, proceed below */
				break;
			case RANGE_MATCH:
				/* a valid character class that was matched */
				ok = 1;
				continue;
			case RANGE_NOMATCH:
				/* a valid character class that was not matched */
				continue;
			}
		}

		c = FOLDCASE(*pat++, flags);

		if (*pat == '-' && *(pat + 1) != EOS && *(pat + 1) != ']') {
			if (*++pat == '\\' && !(flags & FNM_NOESCAPE)) {
				if (*pat != EOS) {
					pat++;
				}
			}
			c2 = FOLDCASE(*pat, flags);
			pat++;
			if (c2 == EOS) {
				return RANGE_ERROR;
			}

			if (flags & FNM_CASEFOLD) {
				c2 = tolower((int)c2);
			}

			if (c <= test && test <= c2) {
				ok = 1;
			}
		} else if (c == test) {
			ok = 1;
		}
	}

	if (ok != negate) {
		*pattern = pat;
		return RANGE_MATCH;
	}

	return RANGE_NOMATCH;
}

static int fnmatchx(const char *pattern, const char *string, const char *stringstart, int flags,
		    size_t recursion)
{
	char c;
	char pc, sc;

	if (recursion-- == 0) {
		return FNM_NORES;
	}

	while (true) {
		pc = FOLDCASE(*pattern++, flags);
		sc = FOLDCASE(*string, flags);
		switch (pc) {
		case EOS:
			if (((flags & FNM_LEADING_DIR) != 0) && (sc == '/')) {
				return 0;
			}
			if (sc == EOS) {
				return 0;
			}
			return FNM_NOMATCH;
		case '?':
			if (sc == EOS) {
				return FNM_NOMATCH;
			}
			if ((sc == '/') && ((flags & FNM_PATHNAME) != 0)) {
				return FNM_NOMATCH;
			}
			if ((sc == '.') && ((flags & FNM_PERIOD) != 0) &&
			    ((string == stringstart) ||
			     (((flags & FNM_PATHNAME) != 0) && (*(string - 1) == '/')))) {
				return FNM_NOMATCH;
			}
			++string;
			break;
		case '*':
			c = FOLDCASE(*pattern, flags);
			/* Collapse multiple stars. */
			while (c == '*') {
				c = FOLDCASE(*++pattern, flags);
			}

			if ((sc == '.') && ((flags & FNM_PERIOD) != 0) &&
			    ((string == stringstart) ||
			     (((flags & FNM_PATHNAME) != 0) && (*(string - 1) == '/')))) {
				return FNM_NOMATCH;
			}

			/* Optimize for pattern with * at end or before /. */
			if (c == EOS) {
				if ((flags & FNM_PATHNAME) == 0) {
					return 0;
				}

				if (((flags & FNM_LEADING_DIR) != 0) ||
				    (strchr(string, '/') == NULL)) {
					return 0;
				}

				return FNM_NOMATCH;
			} else if ((c == '/') && ((flags & FNM_PATHNAME) != 0)) {
				string = strchr(string, '/');
				if (string == NULL) {
					return FNM_NOMATCH;
				}
				break;
			}

			/* General case, use recursion. */
			while (sc != EOS) {
				if (fnmatchx(pattern, string, stringstart, flags, recursion) == 0) {
					return 0;
				}
				sc = FOLDCASE(*string, flags);
				if ((sc == '/') && ((flags & FNM_PATHNAME) != 0)) {
					break;
				}
				++string;
			}
			return FNM_NOMATCH;
		case '[':
			if (sc == EOS) {
				return FNM_NOMATCH;
			}
			if ((sc == '/') && ((flags & FNM_PATHNAME) != 0)) {
				return FNM_NOMATCH;
			}
			if ((sc == '.') && ((flags & FNM_PERIOD) != 0) &&
			    ((string == stringstart) ||
			     (((flags & FNM_PATHNAME) != 0) && (*(string - 1) == '/')))) {
				return FNM_NOMATCH;
			}

			switch (rangematch(&pattern, sc, flags)) {
			case RANGE_ERROR:
				goto norm;
			case RANGE_MATCH:
				break;
			case RANGE_NOMATCH:
				return FNM_NOMATCH;
			}
			++string;
			break;
		case '\\':
			if ((flags & FNM_NOESCAPE) == 0) {
				pc = FOLDCASE(*pattern++, flags);
			}
			__fallthrough;
		default:
norm:
			if (pc != sc) {
				return FNM_NOMATCH;
			}
			++string;
			break;
		}
	}
	CODE_UNREACHABLE;
}

int fnmatch(const char *pattern, const char *string, int flags)
{
	return fnmatchx(pattern, string, string, flags, 64);
}

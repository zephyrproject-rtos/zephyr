/* SPDX-License-Identifier: BSD-3-Clause */

/*    $NetBSD: fnmatch.c,v 1.26 2014/10/12 22:32:33 christos Exp $    */

/*
 * Copyright (c) 1989, 1993, 1994
 *    The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
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

#define MATCH_CLASS6(p,a,b,c,d,e,f,g) \
	((p)[0]==(a) && (p)[1]==(b) && (p)[2]==(c) && (p)[3]==(d) && (p)[4]==(e) && (p)[5]==(f) && (p)[6]==(g))

#define MATCH_CLASS7(p,a,b,c,d,e,f,g,h) \
	((p)[0]==(a) && (p)[1]==(b) && (p)[2]==(c) && (p)[3]==(d) && (p)[4]==(e) && (p)[5]==(f) && (p)[6]==(g) && (p)[7]==(h))

enum fnm_char_class {
	FNM_CC_ALNUM,
	FNM_CC_ALPHA,
	FNM_CC_BLANK,
	FNM_CC_CNTRL,
	FNM_CC_DIGIT,
	FNM_CC_GRAPH,
	FNM_CC_LOWER,
	FNM_CC_PRINT,
	FNM_CC_PUNCT,
	FNM_CC_SPACE,
	FNM_CC_UPPER,
	FNM_CC_XDIGIT,
	FNM_CC_INVALID,
};

static bool fnm_cc_is_valid(const char *pattern, size_t psize, enum fnm_char_class *cc)
{
	if (psize < 4 || *pattern != ':')
		return false;

	pattern++;  /* skip ':' */
	psize--;

	/* Each class name ends with ":]" */
	switch (pattern[0]) {
	case 'a':
		if (MATCH_CLASS6(pattern,'a','l','n','u','m',':',']')) {
			*cc = FNM_CC_ALNUM;
			return true;
		}
		if (MATCH_CLASS6(pattern,'a','l','p','h','a',':',']')) {
			*cc = FNM_CC_ALPHA;
			return true;
		}
		break;
	
	case 'b':
		if (MATCH_CLASS6(pattern,'b','l','a','n','k',':',']')) {
			*cc = FNM_CC_BLANK;
			return true;
		}
		break;

	case 'c':
		if (MATCH_CLASS6(pattern,'c','n','t','r','l',':',']')) {
			*cc = FNM_CC_CNTRL;
			return true;
		}
		break;

	case 'd':
		if (MATCH_CLASS6(pattern,'d','i','g','i','t',':',']')) {
			*cc = FNM_CC_DIGIT;
			return true;
		}
		break;

	case 'g':
		if (MATCH_CLASS6(pattern,'g','r','a','p','h',':',']')) {
			*cc = FNM_CC_GRAPH;
			return true;
		}
		break;

	case 'l':
		if (MATCH_CLASS6(pattern,'l','o','w','e','r',':',']')) {
			*cc = FNM_CC_LOWER;
			return true;
		}
		break;

	case 'p':
		if (MATCH_CLASS6(pattern,'p','r','i','n','t',':',']')) {
			*cc = FNM_CC_PRINT;
			return true;
		}
		if (MATCH_CLASS6(pattern,'p','u','n','c','t',':',']')) {
			*cc = FNM_CC_PUNCT;
			return true;
		}
		break;

	case 's':
		if (MATCH_CLASS6(pattern,'s','p','a','c','e',':',']')) {
			*cc = FNM_CC_SPACE;
			return true;
		}
		break;

	case 'u':
		if (MATCH_CLASS6(pattern,'u','p','p','e','r',':',']')) {
			*cc = FNM_CC_UPPER;
			return true;
		}
		break;

	case 'x':
		if (MATCH_CLASS7(pattern,'x','d','i','g','i','t',':',']')) {
			*cc = FNM_CC_XDIGIT;
			return true;
		}
		break;

	default:
		break;
	}

	return false;
}

static inline int fnm_cc_match(int c, enum fnm_char_class cc)
{
	switch (cc) {
	case FNM_CC_ALNUM:  return isalnum(c);
	case FNM_CC_ALPHA:  return isalpha(c);
	case FNM_CC_BLANK:  return isblank(c);
	case FNM_CC_CNTRL:  return iscntrl(c);
	case FNM_CC_DIGIT:  return isdigit(c);
	case FNM_CC_GRAPH:  return isgraph(c);
	case FNM_CC_LOWER:  return islower(c);
	case FNM_CC_PRINT:  return isprint(c);
	case FNM_CC_PUNCT:  return ispunct(c);
	case FNM_CC_SPACE:  return isspace(c);
	case FNM_CC_UPPER:  return isupper(c);
	case FNM_CC_XDIGIT: return isxdigit(c);
	default:
		break;
	}

	return 0;
}


static inline int foldcase(int ch, int flags)
{

	if ((flags & FNM_CASEFOLD) != 0 && isupper(ch)) {
		return tolower(ch);
	}

	return ch;
}

#define FOLDCASE(ch, flags) foldcase((unsigned char)(ch), (flags))

static bool match_posix_class(const char **pattern, int test)
{
	enum fnm_char_class cc;

	const char *p = *pattern;
	size_t remaining = strlen(p);

	if (!fnm_cc_is_valid(p, remaining, &cc))
		return false;

	/* move pattern pointer past ":]" */
	const char *end = strstr(p, ":]");
	if (end)
		*pattern = end + 2;

	return fnm_cc_match(test, cc);
}

static const char *rangematch(const char *pattern, int test, int flags)
{
	bool negate, ok, need;
	char c, c2;

	if (pattern == NULL) {
		return NULL;
	}

	/*
	 * A bracket expression starting with an unquoted circumflex
	 * character produces unspecified results (IEEE 1003.2-1992,
	 * 3.13.2).  This implementation treats it like '!', for
	 * consistency with the regular expression syntax.
	 * J.T. Conklin (conklin@ngai.kaleida.com)
	 */
	negate = *pattern == '!' || *pattern == '^';
	if (negate) {
		++pattern;
	}

	for (need = true, ok = false, c = FOLDCASE(*pattern++, flags); c != ']' || need;
	     c = FOLDCASE(*pattern++, flags)) {
		need = false;

		if (c == '/' && (flags & FNM_PATHNAME)) {
			return (void *)-1;
		}

		if (c == '\\' && !(flags & FNM_NOESCAPE)) {
			if (*pattern != ']' && *pattern != EOS) {
				c = FOLDCASE(*pattern++, flags);
			}
		}

		if (c == EOS) {
			return NULL;
		}

		if (c == '[' && *pattern == ':') {
			if (match_posix_class(&pattern, test)) {
				ok = true;
				continue;
			} else {
				// skip over class if unrecognized
				while (*pattern && !(*pattern == ':' && *(pattern+1) == ']')) pattern++;
				if (*pattern) pattern += 2;
				continue;
			}
		}

		if (*pattern == '-') {
			c2 = FOLDCASE(*(pattern + 1), flags);
			if (c2 != EOS && c2 != ']') {
				pattern += 2;
				if (c2 == '\\' && !(flags & FNM_NOESCAPE)) {
					c2 = FOLDCASE(*pattern++, flags);
				}

				if (c2 == EOS) {
					return NULL;
				}

				if (c <= test && test <= c2) {
					ok = true;
				}
			}
		} else if (c == test) {
			ok = true;
		}
	}

	return ok == negate ? NULL : pattern;
}

static int fnmatchx(const char *pattern, const char *string, int flags, size_t recursion)
{
	const char *stringstart, *r;
	char c, test;

	if (pattern == NULL || string == NULL) {
		return FNM_NOMATCH;
	}

	if (recursion-- == 0) {
		return FNM_NORES;
	}

	for (stringstart = string;;) {
		c = FOLDCASE(*pattern++, flags);
		switch (c) {
		case EOS:
			if ((flags & FNM_LEADING_DIR) && *string == '/') {
				return 0;
			}

			return *string == EOS ? 0 : FNM_NOMATCH;
		case '?':
			if (*string == EOS) {
				return FNM_NOMATCH;
			}

			if (*string == '/' && (flags & FNM_PATHNAME)) {
				return FNM_NOMATCH;
			}

			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			     ((flags & FNM_PATHNAME) && *(string - 1) == '/'))) {
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

			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			     ((flags & FNM_PATHNAME) && *(string - 1) == '/'))) {
				return FNM_NOMATCH;
			}

			/* Optimize for pattern with * at end or before /. */
			if (c == EOS) {
				if (flags & FNM_PATHNAME) {
					return (flags & FNM_LEADING_DIR) ||
							       strchr(string, '/') == NULL
						       ? 0
						       : FNM_NOMATCH;
				} else {
					return 0;
				}
			} else if (c == '/' && flags & FNM_PATHNAME) {
				string = strchr(string, '/');
				if (string == NULL) {
					return FNM_NOMATCH;
				}

				break;
			}

			/* General case, use recursion. */
			do {
				test = FOLDCASE(*string, flags);
				if (test == EOS) {
					break;
				}

				int e = fnmatchx(pattern, string, flags & ~FNM_PERIOD, recursion);

				if (e != FNM_NOMATCH) {
					return e;
				}

				if (test == '/' && flags & FNM_PATHNAME) {
					break;
				}

				++string;
			} while (true);

			return FNM_NOMATCH;
		case '[':
			if (*string == EOS) {
				return FNM_NOMATCH;
			}

			if (*string == '/' && flags & FNM_PATHNAME) {
				return FNM_NOMATCH;
			}

			if (*string == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			     ((flags & FNM_PATHNAME) && *(string - 1) == '/'))) {
				return FNM_NOMATCH;
			}

			r = rangematch(pattern, FOLDCASE(*string, flags), flags);

			if (r == NULL) {
				if (FOLDCASE('[', flags) != FOLDCASE(*string, flags)) {
					return FNM_NOMATCH;
				}
				++string;
				break;
			}

			if (r == (void *)-1) {
				if (*string != '[') {
					return FNM_NOMATCH;
				}
			} else {
				pattern = r;
			}

			++string;
			break;
		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				c = FOLDCASE(*pattern++, flags);
				if (c == EOS) {
					c = '\0';
					--pattern;
				}
			}
			__fallthrough;
		default:
			if (c != FOLDCASE(*string++, flags)) {
				return FNM_NOMATCH;
			}

			break;
		}
	}
}

int fnmatch(const char *pattern, const char *string, int flags)
{
	return fnmatchx(pattern, string, flags, 64);
}

/* ctype.h */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

static inline int isupper(int a)
{
	return (('A' <= a) && (a <= 'Z'));
}

static inline int isalpha(int c)
{
	/* force to lowercase */
	c |= 32;

	return (('a' <= c) && (c <= 'z'));
}

static inline int isblank(int c)
{
	return ((c == ' ') || (c == '\t'));
}

static inline int isspace(int c)
{
	return ((c == ' ') || (('\t' <= c) && (c <= '\r')));
}

static inline int isgraph(int c)
{
	return ((' ' < c) && (c <= '~'));
}

static inline int isprint(int c)
{
	return ((' ' <= c) && (c <= '~'));
}

static inline int isdigit(int a)
{
	return (('0' <= a) && (a <= '9'));
}

static inline int islower(int c)
{
	return (('a' <= c) && (c <= 'z'));
}

static inline int isxdigit(int a)
{
	if (isdigit(a) != 0) {
		return 1;
	}

	/* force to lowercase */
	a |= 32;

	return (('a' <= a) && (a <= 'f'));
}

static inline int tolower(int chr)
{
	return (chr >= 'A' && chr <= 'Z') ? (chr + 32) : (chr);
}

static inline int toupper(int chr)
{
	return ((chr >= 'a' && chr <= 'z') ? (chr - 32) : (chr));
}

static inline int isalnum(int chr)
{
	return (isalpha(chr) || isdigit(chr));
}

static inline int ispunct(int c)
{
	return (isgraph(c) && !isalnum(c));
}

static inline int iscntrl(int c)
{
	return ((((unsigned int)c) <= 31U) || (((unsigned int)c) == 127U));
}

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_ */

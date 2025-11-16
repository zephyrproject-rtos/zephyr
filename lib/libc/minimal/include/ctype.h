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
	return (int)(((unsigned int)(a)-(unsigned int)'A') < 26U);
}

static inline int isalpha(int c)
{
	return (int)((((unsigned int)c|32u)-(unsigned int)'a') < 26U);
}

static inline int isspace(int c)
{
	return (int)(c == (int)' ' || ((unsigned int)c-(unsigned int)'\t') < 5U);
}

static inline int isgraph(int c)
{
	return (int)((((unsigned int)c) > ' ') &&
			(((unsigned int)c) <= (unsigned int)'~'));
}

static inline int isprint(int c)
{
	return (int)((((unsigned int)c) >= ' ') &&
			(((unsigned int)c) <= (unsigned int)'~'));
}

static inline int isdigit(int a)
{
	return (int)(((unsigned int)(a)-(unsigned int)'0') < 10U);
}

static inline int isxdigit(int a)
{
	unsigned int ua = (unsigned int)a;

	return (int)(((ua - (unsigned int)'0') < 10U) ||
			((ua | 32U) - (unsigned int)'a' < 6U));
}

static inline int tolower(int chr)
{
	return (chr >= (int)'A' && chr <= (int)'Z') ? (chr + 32) : (chr);
}

static inline int toupper(int chr)
{
	return (int)((chr >= (int)'a' && chr <=
				(int)'z') ? (chr - 32) : (chr));
}

static inline int isalnum(int chr)
{
	return (int)(isalpha(chr) || isdigit(chr));
}

static inline int iscntrl(int c)
{
	return (int)((((unsigned int)c) <= 31U) || (((unsigned int)c) == 127U));
}

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_ */

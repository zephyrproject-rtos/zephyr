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
	return (int)(((unsigned)(a)-(unsigned)'A') < 26U);
}

static inline int isalpha(int c)
{
	return (int)((((unsigned)c|32u)-(unsigned)'a') < 26U);
}

static inline int isspace(int c)
{
	return (int)(c == (int)' ' || ((unsigned)c-(unsigned)'\t') < 5U);
}

static inline int isgraph(int c)
{
	return (int)((((unsigned)c) > ' ') &&
			(((unsigned)c) <= (unsigned)'~'));
}

static inline int isprint(int c)
{
	return (int)((((unsigned)c) >= ' ') &&
			(((unsigned)c) <= (unsigned)'~'));
}

static inline int isdigit(int a)
{
	return (int)(((unsigned)(a)-(unsigned)'0') < 10U);
}

static inline int isxdigit(int a)
{
	unsigned int ua = (unsigned int)a;

	return (int)(((ua - (unsigned)'0') < 10U) ||
			((ua | 32U) - (unsigned)'a' < 6U));
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

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_ */

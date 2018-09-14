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
	return ((unsigned)(a)-'A') < 26;
}

static inline int isalpha(int c)
{
	return (((unsigned)c|32)-'a') < 26;
}

static inline int isspace(int c)
{
	return c == ' ' || ((unsigned)c-'\t') < 5;
}

static inline int isgraph(int c)
{
	return ((((unsigned)c) > ' ') && (((unsigned)c) <= '~'));
}

static inline int isprint(int c)
{
	return ((((unsigned)c) >= ' ') && (((unsigned)c) <= '~'));
}

static inline int isdigit(int a)
{
	return (((unsigned)(a)-'0') < 10);
}

static inline int isxdigit(int a)
{
	unsigned int ua = (unsigned int)a;

	return ((ua - '0') < 10) || ((ua - 'a') < 6) || ((ua - 'A') < 6);
}

static inline int tolower(int chr)
{
	return (chr >= 'A' && chr <= 'Z') ? (chr + 32) : (chr);
}

static inline int toupper(int chr)
{
	return (chr >= 'a' && chr <= 'z') ? (chr - 32) : (chr);
}

static inline int isalnum(int chr)
{
	return isalpha(chr) || isdigit(chr);
}

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_ */

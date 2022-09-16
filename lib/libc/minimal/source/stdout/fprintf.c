/* fprintf.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stdio.h>
#include <zephyr/sys/cbprintf.h>

#define DESC(d) ((void *)d)

int fprintf(FILE *ZRESTRICT stream, const char *ZRESTRICT format, ...)
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = cbvprintf(fputc, DESC(stream), format, vargs);
	va_end(vargs);

	return r;
}

int vfprintf(FILE *ZRESTRICT stream, const char *ZRESTRICT format,
	     va_list vargs)
{
	int r;

	r = cbvprintf(fputc, DESC(stream), format, vargs);

	return r;
}

int printf(const char *ZRESTRICT format, ...)
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = cbvprintf(fputc, DESC(stdout), format, vargs);
	va_end(vargs);

	return r;
}

int vprintf(const char *ZRESTRICT format, va_list vargs)
{
	int r;

	r = cbvprintf(fputc, DESC(stdout), format, vargs);

	return r;
}

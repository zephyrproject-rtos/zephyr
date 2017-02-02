/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

static int _stdout_hook_default(int c)
{
	(void)(c);  /* Prevent warning about unused argument */

	return EOF;
}

static int (*_stdout_hook)(int) = _stdout_hook_default;

void __stdout_hook_install(int (*hook)(int))
{
	_stdout_hook = hook;
}

int fputc(int c, FILE *stream)
{
	return (stdout == stream) ? _stdout_hook(c) : EOF;
}

int fputs(const char *_MLIBC_RESTRICT string, FILE *_MLIBC_RESTRICT stream)
{
	if (stream != stdout) {
		return EOF;
	}

	while (*string != '\0') {
		if (_stdout_hook((int)*string) == EOF) {
			return EOF;
		}
		string++;
	}

	return 0;
}

size_t fwrite(const void *_MLIBC_RESTRICT ptr, size_t size, size_t nitems,
			  FILE *_MLIBC_RESTRICT stream)
{
	size_t i;
	size_t j;
	const unsigned char *p;

	if ((stream != stdout) || (nitems == 0) || (size == 0)) {
		return 0;
	}

	p = ptr;
	i = nitems;
	do {
		j = size;
		do {
			if (_stdout_hook((int) *p) == EOF)
				goto done;
			j--;
		} while (j > 0);

		i--;
	} while (i > 0);

done:
	return (nitems - i);
}

int puts(const char *string)
{
	if (fputs(string, stdout) == EOF) {
		return EOF;
	}

	return (_stdout_hook((int) '\n') == EOF) ? EOF : 0;
}

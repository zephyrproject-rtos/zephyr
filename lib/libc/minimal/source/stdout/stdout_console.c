/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

int fputs(const char *restrict string, FILE *restrict stream)
{
	int c;

	if (stream != stdout) {
		return EOF;
	}

	for (c = (int) *string; c != 0; string++) {
		if (_stdout_hook(c) == EOF) {
			return EOF;
		}
	}

	return 0;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems,
			  FILE *restrict stream)
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

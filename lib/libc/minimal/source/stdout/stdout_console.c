/* stdout_console.c */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

int fputs(const char *_Restrict string, FILE *_Restrict stream)
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

size_t fwrite(const void *_Restrict ptr, size_t size, size_t nitems,
			  FILE *_Restrict stream)
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

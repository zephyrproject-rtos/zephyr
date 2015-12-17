/* fprintf.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include <stdarg.h>
#include <stdio.h>

#define DESC(d) ((void *)d)

extern int _prf(int (*func)(), void *dest,
				const char *format, va_list vargs);

int fprintf(FILE *_Restrict F, const char *_Restrict format, ...)
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = _prf(fputc, DESC(F), format, vargs);
	va_end(vargs);

	return r;
}

int vfprintf(FILE *_Restrict F, const char *_Restrict format, va_list vargs)
{
	int r;

	r = _prf(fputc, DESC(F), format, vargs);

	return r;
}

int printf(const char *_Restrict format, ...)
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = _prf(fputc, DESC(stdout), format, vargs);
	va_end(vargs);

	return r;
}

int vprintf(const char *_Restrict format, va_list vargs)
{
	int r;

	r = _prf(fputc, DESC(stdout), format, vargs);

	return r;
}

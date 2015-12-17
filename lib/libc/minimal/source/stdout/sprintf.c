/* sprintf.c */

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

extern int _prf(int (*func)(), void *dest,
				const char *format, va_list vargs);

struct emitter {
	char *ptr;
	int len;
};

static int sprintf_out(int c, struct emitter *p)
{
	if (p->len > 1) { /* need to reserve a byte for EOS */
		*(p->ptr) = c;
		p->ptr += 1;
		p->len -= 1;
	}
	return 0; /* indicate keep going so we get the total count */
}

int snprintf(char *_Restrict s, size_t len, const char *_Restrict format, ...)
{
	va_list vargs;

	struct emitter p;
	int     r;
	char    dummy;

	if ((int) len <= 0) {
		if (len == 0) {
			s = &dummy; /* write final NUL to dummy, since can't change *s */
		} else {
		len = 0x7fffffff;  /* allow up to "maxint" characters */
		}
	}

	p.ptr = s;
	p.len = (int) len;

	va_start(vargs, format);
	r = _prf(sprintf_out, (void *) (&p), format, vargs);
	va_end(vargs);

	*(p.ptr) = 0;
	return r;
}

int sprintf(char *_Restrict s, const char *_Restrict format, ...)
{
	va_list vargs;

	struct emitter p;
	int     r;

	p.ptr = s;
	p.len = (int) 0x7fffffff; /* allow up to "maxint" characters */

	va_start(vargs, format);
	r = _prf(sprintf_out, (void *) (&p), format, vargs);
	va_end(vargs);

	*(p.ptr) = 0;
	return r;
}

int vsnprintf(char *_Restrict s, size_t len, const char *_Restrict format, va_list vargs)
{
	struct emitter p;
	int     r;
	char    dummy;

	if ((int) len <= 0) {
		if (len == 0) {
			s = &dummy; /* write final NUL to dummy, since can't change *s */
		} else {
			len = 0x7fffffff;  /* allow up to "maxint" characters */
		}
	}

	p.ptr = s;
	p.len = (int) len;

	r = _prf(sprintf_out, (void *) (&p), format, vargs);

	*(p.ptr) = 0;
	return r;
}

int vsprintf(char *_Restrict s, const char *_Restrict format, va_list vargs)
{
	struct emitter p;
	int     r;

	p.ptr = s;
	p.len = (int) 0x7fffffff; /* allow up to "maxint" characters */

	r = _prf(sprintf_out, (void *) (&p), format, vargs);

	*(p.ptr) = 0;
	return r;
}

/* sprintf.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

int snprintf(char *restrict s, size_t len, const char *restrict format, ...)
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

int sprintf(char *restrict s, const char *restrict format, ...)
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

int vsnprintf(char *restrict s, size_t len, const char *restrict format, va_list vargs)
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

int vsprintf(char *restrict s, const char *restrict format, va_list vargs)
{
	struct emitter p;
	int     r;

	p.ptr = s;
	p.len = (int) 0x7fffffff; /* allow up to "maxint" characters */

	r = _prf(sprintf_out, (void *) (&p), format, vargs);

	*(p.ptr) = 0;
	return r;
}

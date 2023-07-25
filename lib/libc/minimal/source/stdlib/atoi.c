/* SPDX-License-Identifier: MIT */

/*
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* From: http://fossies.org/dox/musl-1.0.5/atoi_8c_source.html */

#include <stdlib.h>
#include <ctype.h>

int atoi(const char *s)
{
	int n = 0;
	int neg = 0;

	while (isspace((unsigned char)*s) != 0) {
		s++;
	}
	switch (*s) {
	case '-':
		neg = 1;
		s++;
		break;	/* artifact to quiet coverity warning */
	case '+':
		s++;
		break;
	default:
		/* Add an empty default with break, this is a defensive programming.
		 * Static analysis tool won't raise a violation if default is empty,
		 * but has that comment.
		 */
		break;
	}
	/* Compute n as a negative number to avoid overflow on INT_MIN */
	while (isdigit((unsigned char)*s) != 0) {
		n = 10*n - (*s++ - '0');
	}
	return neg ? n : -n;
}

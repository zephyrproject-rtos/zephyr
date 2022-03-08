/* SPDX-License-Identifier: BSD-4-Clause-UC */

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long strtoul(const char *nptr, char **endptr, register int base)
{
	register const char *s = nptr;
	register unsigned long acc;
	register char c;
	register unsigned long cutoff;
	register int any, cutlim;
	register bool neg = false;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace((int)(unsigned char)c));
	if (c == '-') {
		neg = true;
		c = *s++;
	} else if (c == '+') {
		c = *s++;
	}

	if (((base == 0) || (base == 16)) &&
	    (c == '0') && ((*s == 'x') || (*s == 'X'))) {
		c = s[1];
		s += 2;
		base = 16;
	}

	if (base == 0) {
		base = (c == '0') ? 8 : 10;
	}

	cutoff = ULONG_MAX / (unsigned long)base;
	cutlim = (int)(unsigned long)(ULONG_MAX % (unsigned long)base);
	for (acc = 0, any = 0;; c = *s++) {
		int digit;

		if (isdigit((int)(unsigned char)c)) {
			digit = c - '0';
		} else if (isalpha((int)(unsigned char)c)) {
			digit = c - (isupper((int)(unsigned char)c) ? 'A' : 'a') + 10;
		} else {
			break;
		}
		if (digit >= base) {
			break;
		}
		if ((any < 0) || (acc > cutoff) || ((acc == cutoff) && (digit > cutlim))) {
			any = -1;
		} else {
			any = 1;
			acc *= (unsigned long)base;
			acc += (unsigned long)digit;
		}
	}
	if (endptr != NULL) {
		*endptr = (char *)((any != 0) ? (s - 1) : nptr);
	}
	if (any < 0) {
		errno = ERANGE;
		return ULONG_MAX;
	}
	if (neg) {
		return 0UL - acc;
	}
	return acc;
}

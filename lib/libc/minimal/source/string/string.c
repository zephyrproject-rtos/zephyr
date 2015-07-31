/* string.c - common string routines */

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

#include <string.h>

/**
 *
 * @brief Copy a string
 *
 * @return pointer to destination buffer <d>
 */

char *strcpy(char *restrict d, const char *restrict s)
{
	char *dest = d;

	while (*s != '\0') {
		*d = *s;
		d++;
		s++;
	}

	*d = '\0';

	return dest;
}

/**
 *
 * @brief Copy part of a string
 *
 * @return pointer to destination buffer <d>
 */

char *strncpy(char *restrict d, const char *restrict s, size_t n)
{
	char *dest = d;

	while ((n > 0) && *s != '\0') {
		*d = *s;
		s++;
		d++;
		n--;
	}

	while (n > 0) {
		*d = '\0';
		d++;
		n--;
	}

	return dest;
}

/**
 *
 * @brief String scanning operation
 *
 * @return pointer to 1st instance of found byte, or NULL if not found
 */

char *strchr(const char *s, int c)
{
	char tmp = (char) c;

	while ((*s != tmp) && (*s != '\0'))
		s++;

	return (*s == tmp) ? (char *) s : NULL;
}

/**
 *
 * @brief Get string length
 *
 * @return number of bytes in string <s>
 */

size_t strlen(const char *s)
{
	size_t n = 0;

	while (*s != '\0') {
		s++;
		n++;
	}

	return n;
}

/**
 *
 * @brief Compare two strings
 *
 * @return negative # if <s1> < <s2>, 0 if <s1> == <s2>, else positive #
 */

int strcmp(const char *s1, const char *s2)
{
	while ((*s1 == *s2) && (*s1 != '\0')) {
		s1++;
		s2++;
	}

	return *s1 - *s2;
}

/**
 *
 * @brief Compare part of two strings
 *
 * @return negative # if <s1> < <s2>, 0 if <s1> == <s2>, else positive #
 */

int strncmp(const char *s1, const char *s2, size_t n)
{
	while ((n > 0) && (*s1 == *s2) && (*s1 != '\0')) {
		s1++;
		s2++;
		n--;
	}

	return (n == 0) ? 0 : (*s1 - *s2);
}

char *strcat(char *restrict dest, const char *restrict src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}

/**
 *
 * @brief Compare two memory areas
 *
 * @return negative # if <m1> < <m2>, 0 if <m1> == <m2>, else positive #
 */
int memcmp(const void *m1, const void *m2, size_t n)
{
	const char *c1 = m1;
	const char *c2 = m2;

	if (!n)
		return 0;

	while ((--n > 0) && (*c1 == *c2)) {
		c1++;
		c2++;
	}

	return *c1 - *c2;
}

/**
 *
 * @brief Copy bytes in memory with overlapping areas
 *
 * @return pointer to destination buffer <d>
 */

void *memmove(void *d, const void *s, size_t n)
{
	char *dest = d;
	const char *src  = s;

	if ((size_t) (d - s) < n) {
		/*
		 * The <src> buffer overlaps with the start of the <dest> buffer.
		 * Copy backwards to prevent the premature corruption of <src>.
		 */

		while (n > 0) {
			n--;
			dest[n] = src[n];
		}
	} else {
		/* It is safe to perform a forward-copy */
		while (n > 0) {
			*dest = *src;
			dest++;
			src++;
			n--;
		}
	}

	return d;
}

/**
 *
 * @brief Copy bytes in memory
 *
 * @return pointer to start of destination buffer
 */

void *memcpy(void *restrict d, const void *restrict s, size_t n)
{
	/* attempt word-sized copying only if buffers have identical alignment */

	unsigned char *d_byte = (unsigned char *)d;
	const unsigned char *s_byte = (const unsigned char *)s;

	if ((((unsigned int)d ^ (unsigned int)s_byte) & 0x3) == 0) {

		/* do byte-sized copying until word-aligned or finished */

		while (((unsigned int)d_byte) & 0x3) {
			if (n == 0) {
				return d;
			}
			*(d_byte++) = *(s_byte++);
			n--;
		};

		/* do word-sized copying as long as possible */

		unsigned int *d_word = (unsigned int *)d_byte;
		const unsigned int *s_word = (const unsigned int *)s_byte;

		while (n >= sizeof(unsigned int)) {
			*(d_word++) = *(s_word++);
			n -= sizeof(unsigned int);
		}

		d_byte = (unsigned char *)d_word;
		s_byte = (unsigned char *)s_word;
	}

	/* do byte-sized copying until finished */

	while (n > 0) {
		*(d_byte++) = *(s_byte++);
		n--;
	}

	return d;
}

/**
 *
 * @brief Set bytes in memory
 *
 * @return pointer to start of buffer
 */

void *memset(void *buf, int c, size_t n)
{
	/* do byte-sized initialization until word-aligned or finished */

	unsigned char *d_byte = (unsigned char *)buf;
	unsigned char c_byte = (unsigned char)c;

	while (((unsigned int)d_byte) & 0x3) {
		if (n == 0) {
			return buf;
		}
		*(d_byte++) = c_byte;
		n--;
	};

	/* do word-sized initialization as long as possible */

	unsigned int *d_word = (unsigned int *)d_byte;
	unsigned int c_word = (unsigned int)(unsigned char)c;

	c_word |= c_word << 8;
	c_word |= c_word << 16;

	while (n >= sizeof(unsigned int)) {
		*(d_word++) = c_word;
		n -= sizeof(unsigned int);
	}

	/* do byte-sized initialization until finished */

	d_byte = (unsigned char *)d_word;

	while (n > 0) {
		*(d_byte++) = c_byte;
		n--;
	}

	return buf;
}

/**
 *
 * @brief Scan byte in memory
 *
 * @return pointer to start of found byte
 */

void *memchr(const void *s, unsigned char c, size_t n)
{
	if (n != 0) {
		const unsigned char *p = s;

		do {
			if (*p++ == c) {
				return ((void *)(p - 1));
			}

		} while (--n != 0);
	}

	return NULL;
}

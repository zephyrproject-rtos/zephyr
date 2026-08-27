/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/word_granular_access.h>

/* Copy byte. Read byte with byte-sized read, write byte with word-sized-aligned write */
static inline void copy_byte_to_word_granular_access(unsigned char *dst, const unsigned char *src)
{
	/* Mask off the lower bits to get an aligned pointer */
	const sys_mem_word_t mask = sizeof(sys_mem_word_t) - 1;
	sys_mem_word_t d_aligned = (sys_mem_word_t)dst & ~mask;
	unsigned int d_offset = (sys_mem_word_t)dst & mask;

	/* Clear byte at destination and insert byte from source */
	sys_mem_word_t d_word = *(sys_mem_word_t *)d_aligned;
	sys_mem_word_t byte_mask = ~((sys_mem_word_t)0xFF << (d_offset * 8));

	d_word = (d_word & byte_mask) | ((sys_mem_word_t)*src << (d_offset * 8));

	*(sys_mem_word_t *)d_aligned = d_word;
}

/* Copy byte. Read byte with word-sized-aligned read, write byte with byte-sized write */
static inline void copy_byte_from_word_granular_access(unsigned char *dst, const unsigned char *src)
{
	/* Mask off the lower bits to get an aligned pointer */
	const sys_mem_word_t mask = sizeof(sys_mem_word_t) - 1;
	sys_mem_word_t s_aligned = (sys_mem_word_t)src & ~mask;
	unsigned int s_offset = (sys_mem_word_t)src & mask;

	/* Read aligned word from source and extract byte */
	sys_mem_word_t s_word = *(sys_mem_word_t *)s_aligned;
	unsigned char s_byte = (s_word >> (s_offset * 8)) & 0xFF;

	*dst = s_byte;
}

static inline void set_byte_word_granular_access(unsigned char *dst, unsigned char value)
{
	const sys_mem_word_t mask = sizeof(sys_mem_word_t) - 1;
	sys_mem_word_t d_aligned = (sys_mem_word_t)dst & ~mask;
	unsigned int d_offset = (sys_mem_word_t)dst & mask;

	sys_mem_word_t d_word = *(sys_mem_word_t *)d_aligned;

	/* Clear destination byte and insert new value */
	sys_mem_word_t byte_mask = ~((sys_mem_word_t)0xFF << (d_offset * 8));

	d_word = (d_word & byte_mask) | ((sys_mem_word_t)value << (d_offset * 8));

	*(sys_mem_word_t *)d_aligned = d_word;
}

void *sys_memset_word_granular_access(void *buf, int c, size_t n)
{
	/* Do byte-sized initialization until word-aligned or finished */
	unsigned char *d_byte = (unsigned char *)buf;
	unsigned char c_byte = (unsigned char)c;

#if !defined(CONFIG_MINIMAL_LIBC_OPTIMIZE_STRING_FOR_SIZE)
	while (((sys_mem_word_t)d_byte) & (sizeof(sys_mem_word_t) - 1)) {
		if (n == 0) {
			return buf;
		}
		set_byte_word_granular_access(d_byte++, c_byte);
		n--;
	}

	/* Do word-sized initialization as long as possible */
	sys_mem_word_t *d_word = (sys_mem_word_t *)d_byte;
	sys_mem_word_t c_word = (sys_mem_word_t)c_byte;

	c_word |= c_word << 8;
	c_word |= c_word << 16;

	while (n >= sizeof(sys_mem_word_t)) {
		*(d_word++) = c_word;
		n -= sizeof(sys_mem_word_t);
	}

	/* Do byte-sized initialization until finished */
	d_byte = (unsigned char *)d_word;
#endif

	while (n > 0) {
		set_byte_word_granular_access(d_byte++, c_byte);
		n--;
	}

	return buf;
}

void *sys_memcpy_to_word_granular_access(void *d, const void *s, size_t n)
{
	/* Attempt word-sized copying only if buffers have identical alignment */
	unsigned char *d_byte = (unsigned char *)d;
	const unsigned char *s_byte = (const unsigned char *)s;

#if !defined(CONFIG_MINIMAL_LIBC_OPTIMIZE_STRING_FOR_SIZE)
	const sys_mem_word_t mask = sizeof(sys_mem_word_t) - 1;

	if ((((sys_mem_word_t)d ^ (sys_mem_word_t)s_byte) & mask) == 0) {
		/* Deal with unaligned first bytes */
		while (((sys_mem_word_t)d_byte) & mask) {
			if (n == 0) {
				return d;
			}
			copy_byte_to_word_granular_access(d_byte++, s_byte++);
			n--;
		}

		/* Do word-sized copying as long as possible */
		sys_mem_word_t *d_word = (sys_mem_word_t *)d_byte;
		const sys_mem_word_t *s_word = (const sys_mem_word_t *)s_byte;

		while (n >= sizeof(sys_mem_word_t)) {
			*(d_word++) = *(s_word++);
			n -= sizeof(sys_mem_word_t);
		}

		d_byte = (unsigned char *)d_word;
		s_byte = (const unsigned char *)s_word;
	}
#endif

	/* Do byte-sized copying until finished */
	while (n > 0) {
		copy_byte_to_word_granular_access(d_byte++, s_byte++);
		n--;
	}

	return d;
}

void *sys_memcpy_from_word_granular_access(void *d, const void *s, size_t n)
{
	/* Attempt word-sized copying only if buffers have identical alignment */
	unsigned char *d_byte = (unsigned char *)d;
	const unsigned char *s_byte = (const unsigned char *)s;

#if !defined(CONFIG_MINIMAL_LIBC_OPTIMIZE_STRING_FOR_SIZE)
	const sys_mem_word_t mask = sizeof(sys_mem_word_t) - 1;

	if ((((sys_mem_word_t)d ^ (sys_mem_word_t)s_byte) & mask) == 0) {
		/* Deal with unaligned first bytes */
		while (((sys_mem_word_t)d_byte) & mask) {
			if (n == 0) {
				return d;
			}
			copy_byte_from_word_granular_access(d_byte++, s_byte++);
			n--;
		}

		/* Do word-sized copying as long as possible */
		sys_mem_word_t *d_word = (sys_mem_word_t *)d_byte;
		const sys_mem_word_t *s_word = (const sys_mem_word_t *)s_byte;

		while (n >= sizeof(sys_mem_word_t)) {
			*(d_word++) = *(s_word++);
			n -= sizeof(sys_mem_word_t);
		}

		d_byte = (unsigned char *)d_word;
		s_byte = (const unsigned char *)s_word;
	}
#endif

	/* Do byte-sized copying until finished */
	while (n > 0) {
		copy_byte_from_word_granular_access(d_byte++, s_byte++);
		n--;
	}

	return d;
}

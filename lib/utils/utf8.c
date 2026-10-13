/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <errno.h>
#include <zephyr/sys/util_utf8.h>

#define ASCII_CHAR 0x7F
#define SEQUENCE_FIRST_MASK 0xC0
#define SEQUENCE_LEN_2_BYTE 0xC0
#define SEQUENCE_LEN_3_BYTE 0xE0
#define SEQUENCE_LEN_4_BYTE 0xF0
#define MSB_SET 0x80

/* Bits of a lead byte that tell the length of its sequence */
#define LEAD_MASK_2_BYTE       0xE0U
#define LEAD_MASK_3_BYTE       0xF0U
#define LEAD_MASK_4_BYTE       0xF8U
/* Bits of a lead byte that belong to the code point */
#define LEAD_DATA_2_BYTE       0x1FU
#define LEAD_DATA_3_BYTE       0x0FU
#define LEAD_DATA_4_BYTE       0x07U
/* Bits of a continuation byte that belong to the code point */
#define CONTINUATION_DATA_MASK 0x3FU
#define CONTINUATION_DATA_BITS 6U
/* Smallest code point that needs a sequence of that length */
#define CODE_POINT_MIN_2_BYTE  0x80U
#define CODE_POINT_MIN_3_BYTE  0x800U
#define CODE_POINT_MIN_4_BYTE  0x10000U
/* Code points reserved for the UTF-16 surrogates */
#define SURROGATE_FIRST        0xD800U
#define SURROGATE_LAST         0xDFFFU
/* Last code point of Unicode */
#define CODE_POINT_MAX         0x10FFFFU

char *utf8_trunc(char *utf8_str)
{
	const size_t len = strlen(utf8_str);

	if (len == 0U) {
		/* no-op */
		return utf8_str;
	}

	char *last_byte_p = utf8_str + len - 1U;
	uint8_t bytes_truncated;
	char seq_start_byte;

	if ((*last_byte_p & ASCII_CHAR) == *last_byte_p) {
		/* Not part of an UTF8 sequence, return */
		return utf8_str;
	}

	/* Find the starting byte and NULL-terminate other bytes */
	bytes_truncated = 0;
	while ((*last_byte_p & SEQUENCE_FIRST_MASK) != SEQUENCE_FIRST_MASK &&
	       last_byte_p > utf8_str) {
		last_byte_p--;
		bytes_truncated++;
	}
	bytes_truncated++; /* include the starting byte */

	/* Verify if the last character actually need to be truncated
	 * Handles the case where the number of bytes in the last UTF8-char
	 * matches the number of bytes we searched for the starting byte
	 */
	seq_start_byte = *last_byte_p;
	if ((seq_start_byte & SEQUENCE_LEN_4_BYTE) == SEQUENCE_LEN_4_BYTE) {
		if (bytes_truncated == 4) {
			return utf8_str;
		}
	} else if ((seq_start_byte & SEQUENCE_LEN_3_BYTE) == SEQUENCE_LEN_3_BYTE) {
		if (bytes_truncated == 3) {
			return utf8_str;
		}
	} else if ((seq_start_byte & SEQUENCE_LEN_2_BYTE) == SEQUENCE_LEN_2_BYTE) {
		if (bytes_truncated == 2) {
			return utf8_str;
		}
	}

	/* NULL-terminate the unterminated starting byte */
	*last_byte_p = '\0';

	return utf8_str;
}

char *utf8_lcpy(char *dst, const char *src, size_t n)
{
	if (n > 0) {
		strncpy(dst, src, n - 1);
		dst[n - 1] = '\0';

		if (n != 1) {
			utf8_trunc(dst);
		}
	}

	return dst;
}

int utf8_count_chars(const char *s)
{
	int count = 0;
	const char *p = s; /* getting a pointer to increment */

	while (*p != '\0') {
		if ((*p & MSB_SET) == 0) { /* 1-byte character: 0xxxxxxx */
			p += 1;
		} else if ((*p & SEQUENCE_LEN_3_BYTE) == SEQUENCE_FIRST_MASK) {
			/* 2-byte character: 110xxxxx */
			if ((p[1] & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return -EINVAL;
			}
			p += 2;
		} else if ((*p & SEQUENCE_LEN_4_BYTE) == SEQUENCE_LEN_3_BYTE) {
			/* 3-byte character: 1110xxxx */
			if ((p[1] & SEQUENCE_FIRST_MASK) != MSB_SET
				|| (p[2] & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return -EINVAL;
			}
			p += 3;
		} else if ((*p & 0xF8) == SEQUENCE_LEN_4_BYTE) {
			/* 4-byte character: 11110xxx */
			if ((p[1] & SEQUENCE_FIRST_MASK) != MSB_SET
				|| (p[2] & SEQUENCE_FIRST_MASK) != MSB_SET
				|| (p[3] & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return -EINVAL;
			}
			p += 4;
		} else {
			/* Invalid UTF-8 byte (return) */
			return -EINVAL;
		}
		count++;
	}

	return count;
}

bool utf8_is_valid(const char *s, size_t len)
{
	const uint8_t *bytes = (const uint8_t *)s;
	size_t pos = 0U;

	while (pos < len) {
		uint8_t lead = bytes[pos];
		uint32_t code_point = 0U;
		uint32_t code_point_min = 0U;
		size_t seq_len = 0U;

		if ((lead & MSB_SET) == 0U) {
			/* 1-byte character: 0xxxxxxx */
			seq_len = 1U;
			code_point = lead;
			code_point_min = 0U;
		} else if ((lead & LEAD_MASK_2_BYTE) == SEQUENCE_LEN_2_BYTE) {
			/* 2-byte character: 110xxxxx */
			seq_len = 2U;
			code_point = lead & LEAD_DATA_2_BYTE;
			code_point_min = CODE_POINT_MIN_2_BYTE;
		} else if ((lead & LEAD_MASK_3_BYTE) == SEQUENCE_LEN_3_BYTE) {
			/* 3-byte character: 1110xxxx */
			seq_len = 3U;
			code_point = lead & LEAD_DATA_3_BYTE;
			code_point_min = CODE_POINT_MIN_3_BYTE;
		} else if ((lead & LEAD_MASK_4_BYTE) == SEQUENCE_LEN_4_BYTE) {
			/* 4-byte character: 11110xxx */
			seq_len = 4U;
			code_point = lead & LEAD_DATA_4_BYTE;
			code_point_min = CODE_POINT_MIN_4_BYTE;
		} else {
			/* A continuation byte, or 0xF8 to 0xFF: neither starts a sequence */
			return false;
		}

		if (len - pos < seq_len) {
			/* The sequence does not end within the string */
			return false;
		}

		for (size_t i = 1U; i < seq_len; i++) {
			uint8_t byte = bytes[pos + i];

			if ((byte & SEQUENCE_FIRST_MASK) != MSB_SET) {
				/* invalid continuation */
				return false;
			}
			code_point = (code_point << CONTINUATION_DATA_BITS) |
				     (byte & CONTINUATION_DATA_MASK);
		}

		if (code_point < code_point_min) {
			/* Overlong: the code point fits in a shorter sequence */
			return false;
		}
		if (code_point >= SURROGATE_FIRST && code_point <= SURROGATE_LAST) {
			return false;
		}
		if (code_point > CODE_POINT_MAX) {
			return false;
		}

		pos += seq_len;
	}

	return true;
}

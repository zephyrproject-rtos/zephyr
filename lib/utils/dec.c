/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <limits.h>
#include <string.h>
#include <zephyr/sys/util.h>

uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value)
{
	/* Assume base 10 */
	return (uint8_t)itoa_util(value, buf, buflen, 10);
}

int itoa_util(int value, char *buf, size_t buflen, int base)
{
	size_t num_digits = 0;
	char buffer[sizeof(value) * CHAR_BIT + 2];
	static const char digits[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	/* Check for valid buffer */
	if (buf == NULL) {
		return 0;
	}

	/* Check for valid base */
	if (base < 2 || base > 36) {
		*buf = '\0';
		return 0;
	}

	char *p = &buffer[sizeof(buffer) - 1];
	*p = '\0';

	int abs_val = value < 0 ? value : -value;

	do {
		*(--p) = digits[-(abs_val % base)];
		abs_val /= base;
	} while (abs_val);

	if (value < 0) {
		*(--p) = '-';
	}

	num_digits = &buffer[sizeof(buffer)] - p;
	if (num_digits > buflen) {
		/* Buffer too short */
		/* Only return requested number of bytes */
		memcpy(buf, p, buflen);
		return buflen;
	}
	memcpy(buf, p, num_digits);
	/* ignore \0 termination character */
	return num_digits -= 1;
}

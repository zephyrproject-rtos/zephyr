/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/sys/util.h>

int char2hex(char c, uint8_t *x)
{
	if (c >= '0' && c <= '9') {
		*x = c - '0';
	} else if (c >= 'a' && c <= 'f') {
		*x = c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		*x = c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

int hex2char(uint8_t x, char *c)
{
	if (x <= 9) {
		*c = x + (char)'0';
	} else  if (x <= 15) {
		*c = x - 10 + (char)'a';
	} else {
		return -EINVAL;
	}

	return 0;
}

size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen)
{
	if (hexlen < (buflen * 2U + 1U)) {
		return 0;
	}

	for (size_t i = 0; i < buflen; i++) {
		if (hex2char(buf[i] >> 4, &hex[2U * i]) < 0) {
			return 0;
		}
		if (hex2char(buf[i] & 0xf, &hex[2U * i + 1U]) < 0) {
			return 0;
		}
	}

	hex[2U * buflen] = '\0';
	return 2U * buflen;
}

size_t hex2bin(const char *hex, size_t hexlen, uint8_t *buf, size_t buflen)
{
	uint8_t dec;

	if (buflen < hexlen / 2U + hexlen % 2U) {
		return 0;
	}

	/* if hexlen is uneven, insert leading zero nibble */
	if ((hexlen % 2U) != 0) {
		if (char2hex(hex[0], &dec) < 0) {
			return 0;
		}
		buf[0] = dec;
		hex++;
		buf++;
	}

	/* regular hex conversion */
	for (size_t i = 0; i < hexlen / 2U; i++) {
		if (char2hex(hex[2U * i], &dec) < 0) {
			return 0;
		}
		buf[i] = dec << 4;

		if (char2hex(hex[2U * i + 1U], &dec) < 0) {
			return 0;
		}
		buf[i] += dec;
	}

	return hexlen / 2U + hexlen % 2U;
}

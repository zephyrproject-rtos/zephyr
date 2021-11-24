/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>

uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value)
{
	uint8_t divisor = 100;
	uint8_t num_digits = 0;
	uint8_t digit;

	while (buflen > 0U && divisor > 0U) {
		digit = value / divisor;
		if (digit != 0U || divisor == 1U || num_digits != 0U) {
			*buf = digit + '0';
			buf++;
			buflen--;
			num_digits++;
		}

		value -= digit * divisor;
		divisor /= 10U;
	}

	if (buflen != 0U) {
		*buf = '\0';
	}

	return num_digits;
}

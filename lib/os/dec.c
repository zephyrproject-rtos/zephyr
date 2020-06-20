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

	while (buflen > 0 && divisor > 0) {
		digit = value / divisor;
		if (digit != 0 || divisor == 1 || num_digits != 0) {
			*buf = (char)digit + '0';
			buf++;
			buflen--;
			num_digits++;
		}

		value -= digit * divisor;
		divisor /= 10;
	}

	if (buflen) {
		*buf = '\0';
	}

	return num_digits;
}

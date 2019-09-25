/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>

u8_t u8_to_dec(char *buf, u8_t buflen, u8_t value)
{
	u8_t divisor = 100;
	u8_t num_digits = 0;
	u8_t digit;

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


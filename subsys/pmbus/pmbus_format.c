/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/pmbus/pmbus_format.h>
#include <zephyr/sys/util.h>

#define MILLI 1000

static int pow2_to_milli(int64_t mantissa, int exp, int32_t *val)
{
	int64_t v = mantissa * MILLI;

	if (exp >= 0) {
		v *= BIT64(exp);
	} else {
		v = DIV_ROUND_CLOSEST(v, (int64_t)BIT64(-exp));
	}

	if (v < INT32_MIN || v > INT32_MAX) {
		return -ERANGE;
	}

	*val = (int32_t)v;

	return 0;
}

static int64_t milli_to_mantissa(int32_t val, int exp)
{
	if (exp <= 0) {
		return DIV_ROUND_CLOSEST((int64_t)val * (int64_t)BIT64(-exp), MILLI);
	}

	return DIV_ROUND_CLOSEST((int64_t)val, MILLI * (int64_t)BIT64(exp));
}

int pmbus_linear11_to_milli(uint16_t raw, int32_t *val)
{
	int32_t mantissa = sign_extend(raw & GENMASK(10, 0), 10);
	int exp = sign_extend(raw >> 11, 4);

	return pow2_to_milli(mantissa, exp, val);
}

int pmbus_milli_to_linear11(int32_t val, uint16_t *raw)
{
	if (val == 0) {
		*raw = 0U;
		return 0;
	}

	for (int exp = -16; exp <= 15; exp++) {
		int64_t mantissa = milli_to_mantissa(val, exp);

		if (mantissa >= -1024 && mantissa <= 1023) {
			*raw = ((exp & GENMASK(4, 0)) << 11) | (mantissa & GENMASK(10, 0));
			return 0;
		}
	}

	return -ERANGE;
}

int pmbus_ulinear16_to_milli(uint16_t raw, int8_t exp, int32_t *val)
{
	return pow2_to_milli(raw, exp, val);
}

int pmbus_milli_to_ulinear16(int32_t val, int8_t exp, uint16_t *raw)
{
	int64_t mantissa = milli_to_mantissa(val, exp);

	if (mantissa < 0 || mantissa > UINT16_MAX) {
		return -ERANGE;
	}

	*raw = (uint16_t)mantissa;

	return 0;
}

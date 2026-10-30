/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/pmbus/pmbus_format.h>
#include <zephyr/ztest.h>

/* Expected values computed with exact rational arithmetic, rounded half away from zero */

ZTEST(pmbus_format, test_linear11_decode)
{
	static const struct {
		uint16_t raw;
		int32_t val;
	} vectors[] = {
		{0x0000, 0},  {0xF0C8, 50000},   {0xF738, -50000}, {0x8001, 0},
		{0x83FF, 16}, {0x0BFF, 2046000}, {0xF801, 500},
	};
	int32_t val;

	ARRAY_FOR_EACH(vectors, i) {
		zassert_ok(pmbus_linear11_to_milli(vectors[i].raw, &val), "raw 0x%04x",
			   vectors[i].raw);
		zassert_equal(val, vectors[i].val, "raw 0x%04x: %d", vectors[i].raw, val);
	}

	zassert_equal(pmbus_linear11_to_milli(0x7BFF, &val), -ERANGE);
}

ZTEST(pmbus_format, test_linear11_encode)
{
	static const struct {
		int32_t val;
		uint16_t raw;
	} vectors[] = {
		{0, 0x0000},  {50000, 0xE320}, {-50000, 0xE4E0},    {1, 0x8042},
		{-1, 0x87BE}, {1500, 0xBB00},  {INT32_MAX, 0x620C}, {INT32_MIN, 0x65F4},
	};
	uint16_t raw;

	ARRAY_FOR_EACH(vectors, i) {
		zassert_ok(pmbus_milli_to_linear11(vectors[i].val, &raw), "val %d", vectors[i].val);
		zassert_equal(raw, vectors[i].raw, "val %d: 0x%04x", vectors[i].val, raw);
	}
}

ZTEST(pmbus_format, test_linear11_round_trip)
{
	static const int32_t values[] = {1, -1, 999, 1500, 3300, -40000, 12345678};
	uint16_t raw;
	int32_t val;

	ARRAY_FOR_EACH(values, i) {
		zassert_ok(pmbus_milli_to_linear11(values[i], &raw));
		zassert_ok(pmbus_linear11_to_milli(raw, &val));
		/* 11 bit mantissa: relative error below 1/1024 */
		zassert_within(val, values[i], MAX(1, abs(values[i]) / 1024), "%d -> %d", values[i],
			       val);
	}
}

ZTEST(pmbus_format, test_ulinear16)
{
	uint16_t raw;
	int32_t val;

	zassert_ok(pmbus_ulinear16_to_milli(0x0D00, -12, &val));
	zassert_equal(val, 813);
	zassert_ok(pmbus_ulinear16_to_milli(0xFFFF, -12, &val));
	zassert_equal(val, 16000);
	zassert_equal(pmbus_ulinear16_to_milli(0xFFFF, 15, &val), -ERANGE);

	zassert_ok(pmbus_milli_to_ulinear16(3300, -12, &raw));
	zassert_equal(raw, 0x34CD);
	zassert_equal(pmbus_milli_to_ulinear16(-5, -12, &raw), -ERANGE);
	zassert_equal(pmbus_milli_to_ulinear16(70000, -12, &raw), -ERANGE);
}

ZTEST_SUITE(pmbus_format, NULL, NULL, NULL, NULL, NULL);

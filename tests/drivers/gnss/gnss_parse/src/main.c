/*
 * Copyright 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "gnss_parse.h"

struct test_atoi_sample {
	const char *str;
	uint8_t base;
	int32_t value;
};

static const struct test_atoi_sample atoi_samples[] = {
	{.str = "10", .base = 10, .value = 10},
	{.str = "1", .base = 10, .value = 1},
	{.str = "002", .base = 10, .value = 2},
	{.str = "-10", .base = 10, .value = -10},
	{.str = "-1", .base = 10, .value = -1},
	{.str = "-002", .base = 10, .value = -2},
	{.str = "30000000", .base = 10, .value = 30000000},
	{.str = "-30000000", .base = 10, .value = -30000000},
	{.str = "00", .base = 16, .value = 0},
	{.str = "20", .base = 16, .value = 32},
	{.str = "42", .base = 16, .value = 66},
	{.str = "122", .base = 16, .value = 290},
	{.str = "0122", .base = 16, .value = 290},
};

ZTEST(gnss_parse, test_atoi)
{
	int32_t value;

	for (size_t i = 0; i < ARRAY_SIZE(atoi_samples); i++) {
		zassert_ok(gnss_parse_atoi(atoi_samples[i].str, atoi_samples[i].base, &value),
			   "Parse failed");

		zassert_equal(atoi_samples[i].value, value, "Parsed value is incorrect");
	}

	zassert_equal(gnss_parse_atoi("a10", 10, &value), -EINVAL,
		      "Parse should fail due to invalid base 10 chars");

	zassert_equal(gnss_parse_atoi("h#1c", 16, &value), -EINVAL,
		      "Parse should fail due to invalid base 16 chars");
}

struct test_dec_sample {
	const char *str;
	int64_t value;
};

static const struct test_dec_sample dec_to_nano_samples[] = {
	{.str = "10", .value = 10000000000},
	{.str = "1", .value = 1000000000},
	{.str = "002", .value = 2000000000},
	{.str = "-10", .value = -10000000000},
	{.str = "-1", .value = -1000000000},
	{.str = "-002", .value = -2000000000},
	{.str = "30000000", .value = 30000000000000000},
	{.str = "-30000000", .value = -30000000000000000},
	{.str = "0.10", .value = 100000000},
	{.str = "-0.10", .value = -100000000},
	{.str = "1", .value = 1000000000},
	{.str = "002.000", .value = 2000000000},
	{.str = "-002.000", .value = -2000000000},
	{.str = "0.989812343", .value = 989812343},
	{.str = "-0.989812343", .value = -989812343},
	{.str = "0.112211", .value = 112211000},
	{.str = "-0.112211", .value = -112211000},
	{.str = "000000000.112211000000000000", .value = 112211000},
	{.str = "-000000000.11221100000000000", .value = -112211000},
};

ZTEST(gnss_parse, test_dec_to_nano)
{
	int64_t value;

	for (volatile size_t i = 0; i < ARRAY_SIZE(dec_to_nano_samples); i++) {
		zassert_ok(gnss_parse_dec_to_nano(dec_to_nano_samples[i].str, &value),
			   "Parse failed");

		zassert_equal(dec_to_nano_samples[i].value, value, "Parsed value is incorrect");
	}

	zassert_equal(gnss_parse_dec_to_nano("-0s02..000", &value), -EINVAL,
		      "Parse should fail due to double dot");

	zassert_equal(gnss_parse_dec_to_nano("--002.000", &value), -EINVAL,
		      "Parse should fail due to double -");

	zassert_equal(gnss_parse_dec_to_nano("-00s2.000", &value), -EINVAL,
		      "Parse should fail due to invalid char");
}

ZTEST_SUITE(gnss_parse, NULL, NULL, NULL, NULL, NULL);

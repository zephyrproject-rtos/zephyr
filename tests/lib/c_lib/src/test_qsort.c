/*
 * Copyright (c) 2021 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/ztest.h>

static int compare_ints(const void *a, const void *b)
{
	int aa = *(const int *)a;
	int bb = *(const int *)b;

	return (aa > bb) - (aa < bb);
}

/**
 *
 * @brief Test qsort function
 *
 */
ZTEST(test_c_lib, test_qsort)
{
	{
		int actual_int[] = { 1, 3, 2 };
		const int expect_int[] = { 1, 3, 2 };

		qsort(actual_int + 1, 0, sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "out-of-bounds modifications detected");
	}

	{
		int actual_int[] = { 42 };
		const int expect_int[] = { 42 };

		qsort(actual_int, ARRAY_SIZE(actual_int), sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "size 1 not sorted");
	}

	{
		int actual_int[] = { 42, -42 };
		const int expect_int[] = { -42, 42 };

		qsort(actual_int, ARRAY_SIZE(actual_int), sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "size 2 not sorted");
	}

	{
		int actual_int[] = { 42, -42, 0 };
		const int expect_int[] = { -42, 0, 42 };

		qsort(actual_int, ARRAY_SIZE(actual_int), sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "size 3 not sorted");
	}

	{
		int actual_int[] = { 42, -42, 0, -42 };
		const int expect_int[] = { -42, -42, 0, 42 };

		qsort(actual_int, ARRAY_SIZE(actual_int), sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "error handling duplicates");
	}

	{
		/*
		 * NUMS="$(for i in `seq 0 63`; do echo -n "$(((RANDOM - 16384) % 100)), "; done)"
		 * slightly modified to ensure that there were 0, -ve and +ve duplicates
		 */
		static int actual_int[] = {
			1,   18,  -78, 35,  -67, -71, -12, -69, -60, 91,  -15, -99, -33,
			-52, 52,  -4,  -89, -7,	 22,  -52, -87, 32,  -23, 30,  -35, -9,
			15,  -61, 36,  -49, 24,	 -72, -63, 77,	88,  -93, 13,  49,  41,
			35,  -5,  -72, -46, 64,	 -46, -97, -88, 90,  63,  49,  12,  -58,
			-76, 54,  75,  49,  11,	 61,  42,  0,	-42, 42,  -42,
		};

		/* echo $(echo "$NUMS" | sed -e 's/,/\n/g' | sort -n | sed -e 's/\(.*\)/\1,\ /g') */
		static const int expect_int[] = {
			-99, -97, -93, -89, -88, -87, -78, -76, -72, -72, -71, -69, -67,
			-63, -61, -60, -58, -52, -52, -49, -46, -46, -42, -42, -35, -33,
			-23, -15, -12, -9,  -7,	 -5,  -4,  0,	1,   11,  12,  13,  15,
			18,  22,  24,  30,  32,	 35,  35,  36,	41,  42,  42,  49,  49,
			49,  52,  54,  61,  63,	 64,  75,  77,	88,  90,  91,
		};

		qsort(actual_int, ARRAY_SIZE(actual_int), sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "size 64 not sorted");
	}

	{
		/*
		 * NUMS="$(for i in `seq 0 92`; do echo -n "$(((RANDOM - 16384) % 100)), "; done)"
		 * slightly modified to ensure that there were 0, -ve and +ve duplicates
		 */
		static int actual_int[] = {
			1,   18,  -78, 35,  -67, -71, -12, -69, -60, 91,  -15, -99, -33, -52,
			52,  -4,  -89, -7,  22,	 -52, -87, 32,	-23, 30,  -35, -9,  15,	 -61,
			36,  -49, 24,  -72, -63, 77,  88,  -93, 13,  49,  41,  35,  -5,	 -72,
			-46, 64,  -46, -97, 90,	 63,  49,  12,	-58, -76, 54,  75,  49,	 11,
			61,  -45, 92,  7,   74,	 -3,  -9,  96,	83,  33,  15,  -40, -84, -57,
			40,  -93, -27, 38,  24,	 41,  -70, -51, -88, 27,  94,  51,  -11, -2,
			-21, -70, -6,  77,  42,	 0,   -42, 42,	-42,
		};

		/* echo $(echo "$NUMS" | sed -e 's/,/\n/g' | sort -n | sed -e 's/\(.*\)/\1,\ /g') */
		static const int expect_int[] = {
			-99, -97, -93, -93, -89, -88, -87, -84, -78, -76, -72, -72, -71, -70,
			-70, -69, -67, -63, -61, -60, -58, -57, -52, -52, -51, -49, -46, -46,
			-45, -42, -42, -40, -35, -33, -27, -23, -21, -15, -12, -11, -9,	 -9,
			-7,  -6,  -5,  -4,  -3,	 -2,  0,   1,	7,   11,  12,  13,  15,	 15,
			18,  22,  24,  24,  27,	 30,  32,  33,	35,  35,  36,  38,  40,	 41,
			41,  42,  42,  49,  49,	 49,  51,  52,	54,  61,  63,  64,  74,	 75,
			77,  77,  83,  88,  90,	 91,  92,  94,	96,
		};

		qsort(actual_int, ARRAY_SIZE(actual_int), sizeof(int), compare_ints);
		zassert_mem_equal(actual_int, expect_int, sizeof(expect_int),
				  "size 93 not sorted");
	}
}

static int compare_ints_with_boolp_arg(const void *a, const void *b, void *argp)
{
	int aa = *(const int *)a;
	int bb = *(const int *)b;

	*(bool *)argp = true;

	return (aa > bb) - (aa < bb);
}

ZTEST(test_c_lib, test_qsort_r)
{
	bool arg = false;

	const int expect_int[] = { 1, 5, 7 };
	int actual_int[] = { 1, 7, 5 };

	qsort_r(actual_int, ARRAY_SIZE(actual_int), sizeof(actual_int[0]),
		compare_ints_with_boolp_arg, &arg);

	zassert_mem_equal(actual_int, expect_int, sizeof(expect_int), "array not sorted");
	zassert_true(arg, "arg not modified");
}

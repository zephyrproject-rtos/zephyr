/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/bitpool.h>

/**
 * @addtogroup kernel_common_tests
 * @{
 */

#define BITPOOL_TEST_BITCNT (30)
#define BITPOOL_TEST_LONG_BITCNT (510)

#define zassert_bitpool_bit_true(_v, _b) \
	zassert_true(bitpool_get_bit(_v, _b), "Wrong bit (%u) state, expected true", (_b))

#define zassert_bitpool_bit_false(_v, _b) \
	zassert_false(bitpool_get_bit(_v, _b), "Wrong bit (%u) state, expected false", (_b))

#define bitpool_in_range(bit, low, high) ((bit) >= (low) && (bit) <= (high))

#define __bitpool_in_any_range_fn(bit, ...) bitpool_in_range(bit, __VA_ARGS__)
#define _bitpool_in_any_range_fn(range, bit) __bitpool_in_any_range_fn(bit, __DEBRACKET range)
#define bitpool_in_any_range(bit, ...) \
	(FOR_EACH_FIXED_ARG(_bitpool_in_any_range_fn, (|), bit, __VA_ARGS__))

#define bitpool_check_ranges(var, bitcnt, ...) do {         \
	for (size_t n = 0; n < (bitcnt); ++n) {             \
		if (bitpool_in_any_range(n, __VA_ARGS__)) { \
			zassert_bitpool_bit_true(var, n);   \
		} else {                                    \
			zassert_bitpool_bit_false(var, n);  \
		}                                           \
	} } while (0)

ZTEST_USER(bitpool, test_bitpool_find_and_set)
{
	/* Test in a range of first element */
	int ret;
	static const size_t calc_bitcnt = BITPOOL_TEST_BITCNT;
	ATOMIC_VAL_DEFINE(calc_atomic, BITPOOL_TEST_BITCNT) = {0};

	/* Try to find a pool of 1 where every bit is cleared */
	ret = bitpool_find_first_block(calc_atomic, true, 10, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool set found: %d", ret);

	/* Find space of 10 bits - should be present at the begin */
	ret = bitpool_find_first_block(calc_atomic, false, 10, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 10, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 9));

	/* Trying to find 11 ones where there are only 10 available */
	ret = bitpool_find_first_block(calc_atomic, true, 11, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool set found: %d", ret);

	/* Find 3 bits of ones and clear them */
	ret = bitpool_find_first_block(calc_atomic, true, 3, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 3, false);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (3, 9));

	/* Now try to reserve 5 bits */
	ret = bitpool_find_first_block(calc_atomic, false, 5, calc_bitcnt);
	zassert_equal(10, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 5, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (3, 14));

	/* Free 2 in the middle and try to find it */
	bitpool_set_block_to(calc_atomic, 8, 2, false);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (3, 7), (10, 14));

	ret = bitpool_find_first_block(calc_atomic, false, 2, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 2, true);

	ret = bitpool_find_first_block(calc_atomic, false, 2, calc_bitcnt);
	zassert_equal(8, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 2, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 1), (3, 14));
}

ZTEST_USER(bitpool, test_bitpool_find_and_set_long)
{
	/* Test bit finding and setting mainly at the connection point between array elements */
	int ret;
	static const size_t calc_bitcnt = BITPOOL_TEST_LONG_BITCNT;
	ATOMIC_VAL_DEFINE(calc_atomic, BITPOOL_TEST_LONG_BITCNT) = {0};

	/* Try to find a pool of 1 where every bit is cleared */
	ret = bitpool_find_first_block(calc_atomic, true, 1, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool set found: %d", ret);

	/* Find a pool of 60 1 where every bit is cleared */
	ret = bitpool_find_first_block(calc_atomic, false, 60, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 60, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 59));

	/* Find a pool of 10 1 where every bit is cleared */
	ret = bitpool_find_first_block(calc_atomic, false, 10, calc_bitcnt);
	zassert_equal(60, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 10, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 69));

	/* Find another big pool of zeros */
	ret = bitpool_find_first_block(calc_atomic, false, 250, calc_bitcnt);
	zassert_equal(70, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 250, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 319));

	/* Release 10 bits in the middle */
	bitpool_set_block_to(calc_atomic, 60, 10, false);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 59), (70, 319));

	/* Reserve 11 bits that should not fit into released block in the middle */
	ret = bitpool_find_first_block(calc_atomic, false, 11, calc_bitcnt);
	zassert_equal(320, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 11, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 59), (70, 330));

	/* Reserve 9 bits block that should fit the block released ealier */
	ret = bitpool_find_first_block(calc_atomic, false, 9, calc_bitcnt);
	zassert_equal(60, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 9, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 68), (70, 330));

	/* Reserve 1 additional bit to fill the remaining space */
	ret = bitpool_find_first_block(calc_atomic, false, 1, calc_bitcnt);
	zassert_equal(69, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, 1, true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, 330));

	/* Fill to full capacity */
	ret = bitpool_find_first_block(calc_atomic, false, (calc_bitcnt - 331), calc_bitcnt);
	zassert_equal(331, ret, "Bit pool clear found: %d", ret);
	bitpool_set_block_to(calc_atomic, ret, (calc_bitcnt - 331), true);

	bitpool_check_ranges(calc_atomic, calc_bitcnt, (0, calc_bitcnt));

	/* Trying to reserve any more bits */
	ret = bitpool_find_first_block(calc_atomic, false, 1, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool clear found: %d", ret);
}

ZTEST_USER(bitpool, test_bitpool_find_any_size)
{
	int ret;
	size_t size;
	static const size_t calc_bitcnt = BITPOOL_TEST_BITCNT;
	ATOMIC_VAL_DEFINE(calc_atomic, BITPOOL_TEST_BITCNT) = {0};

	/* Try to find a pool of 1 where every bit is cleared */
	ret = bitpool_find_first_block_any_size(calc_atomic, true, &size, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool set found: %d", ret);

	/* Expect to find full size of zeros */
	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	zassert_equal(calc_bitcnt, size, "Bit pool size: %u", size);

	/* Find pools in partially taken area */
	bitpool_set_block_to(calc_atomic, 11, 3, true);
	bitpool_set_block_to(calc_atomic, 16, 2, true);
	bitpool_set_block_to(calc_atomic, 26, calc_bitcnt - 26, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	zassert_equal(11, size, "Bit pool size: %u", size);
	bitpool_set_block_to(calc_atomic, ret, size, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(14, ret, "Bit pool clear found: %d", ret);
	zassert_equal(2, size, "Bit pool size: %u", size);
	bitpool_set_block_to(calc_atomic, ret, size, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(18, ret, "Bit pool clear found: %d", ret);
	zassert_equal(8, size, "Bit pool size: %u", size);
	bitpool_set_block_to(calc_atomic, ret, size, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool clear found: %d", ret);
}

ZTEST_USER(bitpool, test_bitpool_find_any_size_long)
{
	int ret;
	size_t size;
	static const size_t calc_bitcnt = BITPOOL_TEST_LONG_BITCNT;
	ATOMIC_VAL_DEFINE(calc_atomic, BITPOOL_TEST_LONG_BITCNT) = {0};

	/* Try to find a pool of 1 where every bit is cleared */
	ret = bitpool_find_first_block_any_size(calc_atomic, true, &size, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool set found: %d", ret);

	/* Expect to find full size of zeros */
	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(0, ret, "Bit pool clear found: %d", ret);
	zassert_equal(calc_bitcnt, size, "Bit pool size: %u", size);

	/* Find pools in partially taken area */
	bitpool_set_block_to(calc_atomic, 0, 32, true);
	bitpool_set_block_to(calc_atomic, 64, 10, true);
	bitpool_set_block_to(calc_atomic, 100, 100, true);


	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(32, ret, "Bit pool clear found: %d", ret);
	zassert_equal(32, size, "Bit pool size: %u", size);
	bitpool_set_block_to(calc_atomic, ret, size, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(74, ret, "Bit pool clear found: %d", ret);
	zassert_equal(26, size, "Bit pool size: %u", size);
	bitpool_set_block_to(calc_atomic, ret, size, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(200, ret, "Bit pool clear found: %d", ret);
	zassert_equal(calc_bitcnt - 200, size, "Bit pool size: %u", size);
	bitpool_set_block_to(calc_atomic, ret, size, true);

	ret = bitpool_find_first_block_any_size(calc_atomic, false, &size, calc_bitcnt);
	zassert_equal(-ENOSPC, ret, "Bit pool clear found: %d", ret);
}

ZTEST_USER(bitpool, test_bitpool_atomic_cas)
{
	bool res;
	static const size_t calc_bitcnt = BITPOOL_TEST_LONG_BITCNT;
	ATOMIC_DEFINE(tested_atomic, BITPOOL_TEST_LONG_BITCNT) = {0};
	ATOMIC_VAL_DEFINE(old_atomic, BITPOOL_TEST_LONG_BITCNT);
	ATOMIC_VAL_DEFINE(calc_atomic, BITPOOL_TEST_LONG_BITCNT);

	bitpool_copy(tested_atomic, old_atomic, calc_bitcnt);
	memcpy(calc_atomic, old_atomic, sizeof(calc_atomic));

	bitpool_set_block_to(calc_atomic, 10, calc_bitcnt - 20, true);

	/* CAS */
	res = bitpool_atomic_cas(tested_atomic, old_atomic, calc_atomic, calc_bitcnt);
	zassert_true(res, "CAS error");

	bitpool_check_ranges(tested_atomic, calc_bitcnt, (10, calc_bitcnt - 11));

	/* CAS second time with wrong old value - should fail */
	bitpool_set_block_to(calc_atomic, 15, 5, false);

	res = bitpool_atomic_cas(tested_atomic, old_atomic, calc_atomic, calc_bitcnt);
	zassert_false(res, "CAS should fail");

	bitpool_check_ranges(tested_atomic, calc_bitcnt, (10, calc_bitcnt - 11));
}

ZTEST_USER(bitpool, test_bitpool_atomic_op)
{
	ATOMIC_DEFINE(tested_atomic, BITPOOL_TEST_BITCNT) = {0};
	size_t cnt;

	/* Test if the operation works at all */
	BITPOOL_ATOMIC_OP(tested_atomic, old, new, BITPOOL_TEST_BITCNT)
	{
		bitpool_copy(old, new, BITPOOL_TEST_BITCNT);
		bitpool_set_block_to(new, 10, 10, true);
	}
	bitpool_check_ranges(tested_atomic, BITPOOL_TEST_BITCNT, (10, 19));

	/* Test interruption in atomic operation */
	cnt = 0;
	BITPOOL_ATOMIC_OP(tested_atomic, old, new, BITPOOL_TEST_BITCNT)
	{
		bitpool_copy(old, new, BITPOOL_TEST_BITCNT);
		if (cnt < 3) {
			BITPOOL_ATOMIC_OP(tested_atomic, iold, inew, BITPOOL_TEST_BITCNT)
			{
				bitpool_copy(iold, inew, BITPOOL_TEST_BITCNT);
				bitpool_set_bit(inew, cnt);
			}
			bitpool_check_ranges(tested_atomic, BITPOOL_TEST_BITCNT,
					     (0, cnt), (10, 19));
		}
		bitpool_set_bit(new, 15);
		++cnt;
	}
	zassert_equal(4, cnt, "Unexpected number of iterations: %u", cnt);
	bitpool_check_ranges(tested_atomic, BITPOOL_TEST_BITCNT, (0, 2), (10, 19), (15, 15));
}

ZTEST_USER(bitpool, test_bitpool_atomic_op_break)
{
	ATOMIC_DEFINE(tested_atomic, BITPOOL_TEST_BITCNT) = {0};

	BITPOOL_ATOMIC_OP(tested_atomic, old, new, BITPOOL_TEST_BITCNT)
	{
		bitpool_copy(old, new, BITPOOL_TEST_BITCNT);
		bitpool_set_bit(new, 0);
	}
	bitpool_check_ranges(tested_atomic, BITPOOL_TEST_BITCNT, (0, 0));

	BITPOOL_ATOMIC_OP(tested_atomic, old, new, BITPOOL_TEST_BITCNT)
	{
		bitpool_copy(old, new, BITPOOL_TEST_BITCNT);
		bitpool_set_bit(new, 1);
		BITPOOL_ATOMIC_OP_break;
	}
	bitpool_check_ranges(tested_atomic, BITPOOL_TEST_BITCNT, (0, 0));
}

extern void *common_setup(void);
ZTEST_SUITE(bitpool, NULL, common_setup, NULL, NULL, NULL);
/**
 * @}
 */

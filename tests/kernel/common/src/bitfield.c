/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <arch/cpu.h>

#include <tc_util.h>

#define BIT_INDEX(bit)  (bit >> 3)
#define BIT_VAL(bit)    (1 << (bit & 0x7))
#define BITFIELD_SIZE   512

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/**
 * @brief Test bitfield operations
 *
 * @see sys_test_bit(), sys_set_bit(), sys_clear_bit(),
 * sys_bitfield_set_bit(), sys_bitfield_clear_bit(),
 * sys_bitfield_test_bit(), sys_bitfield_test_and_set_bit(),
 * sys_bitfield_test_and_clear_bit()
 */
void test_bitfield(void)
{
	u32_t b1 = 0U;
	unsigned char b2[BITFIELD_SIZE >> 3] = { 0 };
	unsigned int bit;
	int ret;

	TC_PRINT("twiddling bits....\n");

	for (bit = 0U; bit < 32; ++bit) {
		sys_set_bit((mem_addr_t)&b1, bit);

		zassert_equal(b1, (1 << bit),
			      "sys_set_bit failed on bit %d\n", bit);

		zassert_true(sys_test_bit((mem_addr_t)&b1, bit),
			     "sys_test_bit did not detect bit %d\n", bit);

		sys_clear_bit((mem_addr_t)&b1, bit);
		zassert_equal(b1, 0, "sys_clear_bit failed for bit %d\n", bit);

		zassert_false(sys_test_bit((mem_addr_t)&b1, bit),
			      "sys_test_bit erroneously detected bit %d\n",
			      bit);

		zassert_false(sys_test_and_set_bit((mem_addr_t)&b1, bit),
			      "sys_test_and_set_bit erroneously"
			      " detected bit %d\n", bit);
		zassert_equal(b1, (1 << bit),
			      "sys_test_and_set_bit did not set bit %d\n",
			      bit);
		zassert_true(sys_test_and_set_bit((mem_addr_t)&b1, bit),
			     "sys_test_and_set_bit did not detect bit %d\n",
			     bit);
		zassert_equal(b1, (1 << bit),
			      "sys_test_and_set_bit cleared bit %d\n", bit);

		zassert_true(sys_test_and_clear_bit((mem_addr_t)&b1, bit),
			     "sys_test_and_clear_bit did not detect bit %d\n",
			     bit);
		zassert_equal(b1, 0, "sys_test_and_clear_bit did not clear"
			      " bit %d\n", bit);
		zassert_false(sys_test_and_clear_bit((mem_addr_t)&b1, bit),
			      "sys_test_and_clear_bit erroneously detected"
			      " bit %d\n", bit);
		zassert_equal(b1, 0, "sys_test_and_clear_bit set bit %d\n",
			      bit);
	}

	for (bit = 0U; bit < BITFIELD_SIZE; ++bit) {
		sys_bitfield_set_bit((mem_addr_t)b2, bit);
		zassert_equal(b2[BIT_INDEX(bit)], BIT_VAL(bit),
			      "sys_bitfield_set_bit failed for bit %d\n",
			      bit);
		zassert_true(sys_bitfield_test_bit((mem_addr_t)b2, bit),
			     "sys_bitfield_test_bit did not detect bit %d\n",
			     bit);

		sys_bitfield_clear_bit((mem_addr_t)b2, bit);
		zassert_equal(b2[BIT_INDEX(bit)], 0,
			      "sys_bitfield_clear_bit failed for bit %d\n",
			      bit);
		zassert_false(sys_bitfield_test_bit((mem_addr_t)b2, bit),
			      "sys_bitfield_test_bit erroneously detected"
			      " bit %d\n", bit);

		ret = sys_bitfield_test_and_set_bit((mem_addr_t)b2, bit);
		zassert_false(ret, "sys_bitfield_test_and_set_bit erroneously"
			      " detected bit %d\n", bit);

		zassert_equal(b2[BIT_INDEX(bit)], BIT_VAL(bit),
			      "sys_bitfield_test_and_set_bit did not set"
			      " bit %d\n", bit);
		zassert_true(sys_bitfield_test_and_set_bit((mem_addr_t)b2, bit),
			     "sys_bitfield_test_and_set_bit did not detect bit"
			     " %d\n", bit);
		zassert_equal(b2[BIT_INDEX(bit)], BIT_VAL(bit),
			      "sys_bitfield_test_and_set_bit cleared bit %d\n",
			      bit);

		zassert_true(sys_bitfield_test_and_clear_bit((mem_addr_t)b2,
							     bit), "sys_bitfield_test_and_clear_bit did not"
			     " detect bit %d\n", bit);
		zassert_equal(b2[BIT_INDEX(bit)], 0,
			      "sys_bitfield_test_and_clear_bit did not"
			      " clear bit %d\n", bit);
		zassert_false(sys_bitfield_test_and_clear_bit((mem_addr_t)b2,
							      bit), "sys_bitfield_test_and_clear_bit"
			      " erroneously detected bit %d\n", bit);
		zassert_equal(b2[BIT_INDEX(bit)], 0,
			      "sys_bitfield_test_and_clear_bit set bit %d\n",
			      bit);
	}
}

/**
 * @}
 */

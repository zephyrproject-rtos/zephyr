/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/tc_util.h>

#ifdef CONFIG_BIG_ENDIAN
#define BIT_INDEX(bit)  ((3 - ((bit >> 3) & 0x3)) + 4*(bit >> 5))
#else
#define BIT_INDEX(bit)  (bit >> 3)
#endif
#define BIT_VAL(bit)    (1 << (bit & 0x7))
#define BITFIELD_SIZE   512

/**
 * @defgroup kernel_bitfield_tests Bit Fields
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_bitfield_tests
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
ZTEST(bitfield, test_bitfield)
{
#ifdef CONFIG_ARM
	ztest_test_skip();
#else
	uint32_t b1 = 0U;
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
#endif

}

/**
 * @}
 */

extern void *common_setup(void);

ZTEST_SUITE(bitfield, NULL, common_setup, NULL, NULL, NULL);

/*
 * Copyright (c) 2016,2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/cpu.h>

#include <zephyr/tc_util.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_BIG_ENDIAN
#define BIT_INDEX(bit)  ((3 - ((bit >> 3) & 0x3)) + 4*(bit >> 5))
#else
#define BIT_INDEX(bit)  (bit >> 3)
#endif
#define BIT_VAL(bit)    (1 << (bit & 0x7))
#define BITFIELD_SIZE   512

/**
 * @addtogroup kernel_common_tests
 * @{
 */

/* Helper function to compare two bitarrays */
static bool cmp_u32_arrays(uint32_t *a1, uint32_t *a2, size_t sz)
{
	bool are_equal = true;
	size_t i;

	for (i = 0; i < sz; i++) {
		if (a1[i] != a2[i]) {
			are_equal = false;
			printk("%s: [%zu] 0x%x != 0x%x", __func__,
			       i, a1[i], a2[i]);
			break;
		}
	}

	return are_equal;
}

#define FAIL_ALLOC_MSG_FMT "sys_bitarray_alloc with region size %i allocated incorrectly"
#define FAIL_ALLOC_RET_MSG_FMT "sys_bitarray_alloc with region size %i returned incorrect result"
#define FAIL_ALLOC_OFFSET_MSG_FMT "sys_bitarray_alloc with region size %i gave incorrect offset"
#define FAIL_FREE_MSG_FMT "sys_bitarray_free with region size %i and offset %i failed"
#define FREE 0U

void validate_bitarray_define(sys_bitarray_t *ba, size_t num_bits)
{
	size_t num_bundles;
	int i;

	num_bundles = ROUND_UP(ROUND_UP(num_bits, 8) / 8, sizeof(uint32_t))
		      / sizeof(uint32_t);

	zassert_equal(ba->num_bits, num_bits,
		      "SYS_BITARRAY_DEFINE num_bits expected %u, got %u",
		      num_bits, ba->num_bits);

	zassert_equal(ba->num_bundles, num_bundles,
		      "SYS_BITARRAY_DEFINE num_bundles expected %u, got %u",
		      num_bundles, ba->num_bundles);

	for (i = 0; i < num_bundles; i++) {
		zassert_equal(ba->bundles[i], FREE,
			      "SYS_BITARRAY_DEFINE bundles[%u] not free for num_bits %u",
			      i, num_bits);
	}
}

/**
 * @brief Test defining of bitarrays
 *
 * @see SYS_BITARRAY_DEFINE()
 */
ZTEST(bitarray, test_bitarray_declare)
{
	SYS_BITARRAY_DEFINE(ba_1_bit, 1);
	SYS_BITARRAY_DEFINE(ba_32_bit, 32);
	SYS_BITARRAY_DEFINE(ba_33_bit, 33);
	SYS_BITARRAY_DEFINE(ba_64_bit, 64);
	SYS_BITARRAY_DEFINE(ba_65_bit, 65);
	SYS_BITARRAY_DEFINE(ba_128_bit, 128);
	SYS_BITARRAY_DEFINE(ba_129_bit, 129);

	/* Test SYS_BITFIELD_DECLARE by asserting that a sufficient number of uint32_t
	 * in the declared array are set as free to represent the number of bits
	 */

	validate_bitarray_define(&ba_1_bit, 1);
	validate_bitarray_define(&ba_32_bit, 32);
	validate_bitarray_define(&ba_33_bit, 33);
	validate_bitarray_define(&ba_64_bit, 64);
	validate_bitarray_define(&ba_65_bit, 65);
	validate_bitarray_define(&ba_128_bit, 128);
	validate_bitarray_define(&ba_129_bit, 129);
}

bool bitarray_bundles_is_zero(sys_bitarray_t *ba)
{
	bool ret = true;
	unsigned int i;

	for (i = 0; i < ba->num_bundles; i++) {
		if (ba->bundles[i] != 0) {
			ret = false;
			break;
		}
	}

	return ret;
}

/**
 * @brief Test bitarrays set and clear
 *
 * @see sys_bitarray_set_bit()
 * @see sys_bitarray_clear_bit()
 * @see sys_bitarray_test_bit()
 * @see sys_bitarray_test_and_set_bit()
 * @see sys_bitarray_test_and_clear_bit()
 */
ZTEST(bitarray, test_bitarray_set_clear)
{
	int ret;
	int bit_val;
	size_t bit, bundle_idx, bit_idx_in_bundle;

	/* Bitarrays have embedded spinlocks and can't on the stack. */
	if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		ztest_test_skip();
	}

	SYS_BITARRAY_DEFINE(ba, 234);

	for (bit = 0U; bit < ba.num_bits; ++bit) {
		bundle_idx = bit / (sizeof(ba.bundles[0]) * 8);
		bit_idx_in_bundle = bit % (sizeof(ba.bundles[0]) * 8);

		ret = sys_bitarray_set_bit(&ba, bit);
		zassert_equal(ret, 0,
			      "sys_bitarray_set_bit failed on bit %d", bit);
		zassert_equal(ba.bundles[bundle_idx], BIT(bit_idx_in_bundle),
			      "sys_bitarray_set_bit did not set bit %d\n", bit);
		zassert_not_equal(sys_bitfield_test_bit((mem_addr_t)ba.bundles, bit),
				  0, "sys_bitarray_set_bit did not set bit %d\n", bit);

		ret = sys_bitarray_test_bit(&ba, bit, &bit_val);
		zassert_equal(ret, 0,
			      "sys_bitarray_test_bit failed at bit %d", bit);
		zassert_equal(bit_val, 1,
			      "sys_bitarray_test_bit did not detect bit %d\n", bit);

		ret = sys_bitarray_clear_bit(&ba, bit);
		zassert_equal(ret, 0,
			      "sys_bitarray_clear_bit failed at bit %d", bit);
		zassert_equal(ba.bundles[bundle_idx], 0,
			      "sys_bitarray_clear_bit did not clear bit %d\n", bit);
		zassert_equal(sys_bitfield_test_bit((mem_addr_t)ba.bundles, bit),
			      0, "sys_bitarray_set_bit did not set bit %d\n", bit);

		ret = sys_bitarray_test_bit(&ba, bit, &bit_val);
		zassert_equal(ret, 0,
			      "sys_bitarray_test_bit failed at bit %d", bit);
		zassert_equal(bit_val, 0,
			      "sys_bitarray_test_bit erroneously detected bit %d\n",
			      bit);

		ret = sys_bitarray_test_and_set_bit(&ba, bit, &bit_val);
		zassert_equal(ret, 0,
			      "sys_bitarray_test_and_set_bit failed at bit %d", bit);
		zassert_equal(bit_val, 0,
			      "sys_bitarray_test_and_set_bit erroneously detected bit %d\n",
			      bit);
		zassert_equal(ba.bundles[bundle_idx], BIT(bit_idx_in_bundle),
			      "sys_bitarray_test_and_set_bit did not set bit %d\n", bit);
		zassert_not_equal(sys_bitfield_test_bit((mem_addr_t)ba.bundles, bit),
				  0, "sys_bitarray_set_bit did not set bit %d\n", bit);

		ret = sys_bitarray_test_and_set_bit(&ba, bit, &bit_val);
		zassert_equal(ret, 0,
			      "sys_bitarray_test_and_set_bit failed at bit %d", bit);
		zassert_equal(bit_val, 1,
			      "sys_bitarray_test_and_set_bit did not detect bit %d\n",
			      bit);
		zassert_equal(ba.bundles[bundle_idx], BIT(bit_idx_in_bundle),
			      "sys_bitarray_test_and_set_bit cleared bit %d\n", bit);
		zassert_not_equal(sys_bitfield_test_bit((mem_addr_t)ba.bundles, bit),
				  0, "sys_bitarray_set_bit did not set bit %d\n", bit);

		ret = sys_bitarray_test_and_clear_bit(&ba, bit, &bit_val);
		zassert_equal(ret, 0,
			      "sys_bitarray_test_and_clear_bit failed at bit %d", bit);
		zassert_equal(bit_val, 1,
			      "sys_bitarray_test_and_clear_bit did not detect bit %d\n",
			      bit);
		zassert_equal(ba.bundles[bundle_idx], 0,
			      "sys_bitarray_test_and_clear_bit did not clear bit %d\n",
			      bit);
		zassert_equal(sys_bitfield_test_bit((mem_addr_t)ba.bundles, bit),
			      0, "sys_bitarray_set_bit did not set bit %d\n", bit);

		ret = sys_bitarray_test_and_clear_bit(&ba, bit, &bit_val);
		zassert_equal(ret, 0,
			      "sys_bitarray_test_and_clear_bit failed at bit %d", bit);
		zassert_equal(bit_val, 0,
			      "sys_bitarray_test_and_clear_bit erroneously detected bit %d\n",
			      bit);
		zassert_equal(ba.bundles[bundle_idx], 0,
			      "sys_bitarray_test_and_clear_bit set bit %d\n",
			      bit);
		zassert_equal(sys_bitfield_test_bit((mem_addr_t)ba.bundles, bit),
			      0, "sys_bitarray_set_bit did not set bit %d\n", bit);
	}

	/* All this should fail because we go outside of
	 * total bits in bit array. Also needs to make sure bits
	 * are not changed.
	 */
	ret = sys_bitarray_set_bit(&ba, ba.num_bits);
	zassert_not_equal(ret, 0, "sys_bitarray_set_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_set_bit() erroneously changed bitarray");

	ret = sys_bitarray_clear_bit(&ba, ba.num_bits);
	zassert_not_equal(ret, 0, "sys_bitarray_clear_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_clear_bit() erroneously changed bitarray");

	ret = sys_bitarray_test_bit(&ba, ba.num_bits, &bit_val);
	zassert_not_equal(ret, 0, "sys_bitarray_test_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_test_bit() erroneously changed bitarray");

	ret = sys_bitarray_test_and_set_bit(&ba, ba.num_bits, &bit_val);
	zassert_not_equal(ret, 0,
			  "sys_bitarray_test_and_set_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_test_and_set_bit() erroneously changed bitarray");

	ret = sys_bitarray_test_and_clear_bit(&ba, ba.num_bits, &bit_val);
	zassert_not_equal(ret, 0,
			  "sys_bitarray_test_and_clear_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_test_and_clear_bit() erroneously changed bitarray");

	ret = sys_bitarray_set_bit(&ba, -1);
	zassert_not_equal(ret, 0, "sys_bitarray_set_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_set_bit() erroneously changed bitarray");

	ret = sys_bitarray_clear_bit(&ba, -1);
	zassert_not_equal(ret, 0, "sys_bitarray_clear_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_clear_bit() erroneously changed bitarray");

	ret = sys_bitarray_test_bit(&ba, -1, &bit_val);
	zassert_not_equal(ret, 0, "sys_bitarray_test_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_test_bit() erroneously changed bitarray");

	ret = sys_bitarray_test_and_set_bit(&ba, -1, &bit_val);
	zassert_not_equal(ret, 0,
			  "sys_bitarray_test_and_set_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_test_and_set_bit() erroneously changed bitarray");

	ret = sys_bitarray_test_and_clear_bit(&ba, -1, &bit_val);
	zassert_not_equal(ret, 0,
			  "sys_bitarray_test_and_clear_bit() should fail but not");
	zassert_true(bitarray_bundles_is_zero(&ba),
		     "sys_bitarray_test_and_clear_bit() erroneously changed bitarray");
}

void alloc_and_free_predefined(void)
{
	int ret;
	size_t offset;

	uint32_t ba_128_expected[4];

	SYS_BITARRAY_DEFINE(ba_128, 128);

	printk("Testing bit array alloc and free with predefined patterns\n");

	/* Pre-populate the bits */
	ba_128.bundles[0] = 0x0F0F070F;
	ba_128.bundles[1] = 0x0F0F0F0F;
	ba_128.bundles[2] = 0x0F0F0F0F;
	ba_128.bundles[3] = 0x0F0F0000;

	/* Expected values */
	ba_128_expected[0] = 0x0F0FFF0F;
	ba_128_expected[1] = 0x0F0F0F0F;
	ba_128_expected[2] = 0x0F0F0F0F;
	ba_128_expected[3] = 0x0F0F0000;

	ret = sys_bitarray_alloc(&ba_128, 5, &offset);
	zassert_equal(ret, 0, "sys_bitarray_alloc() failed: %d", ret);
	zassert_equal(offset, 11, "sys_bitarray_alloc() offset expected %d, got %d", 11, offset);
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_alloc() failed bits comparison");

	ret = sys_bitarray_alloc(&ba_128, 16, &offset);
	ba_128_expected[2] = 0xFF0F0F0F;
	ba_128_expected[3] = 0x0F0F0FFF;
	zassert_equal(ret, 0, "sys_bitarray_alloc() failed: %d", ret);
	zassert_equal(offset, 92, "sys_bitarray_alloc() offset expected %d, got %d", 92, offset);
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_alloc() failed bits comparison");

	ret = sys_bitarray_free(&ba_128, 5, 11);
	ba_128_expected[0] = 0x0F0F070F;
	zassert_equal(ret, 0, "sys_bitarray_free() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_free() failed bits comparison");

	ret = sys_bitarray_free(&ba_128, 5, 0);
	zassert_not_equal(ret, 0, "sys_bitarray_free() should fail but not");
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_free() failed bits comparison");

	ret = sys_bitarray_free(&ba_128, 24, 92);
	zassert_not_equal(ret, 0, "sys_bitarray_free() should fail but not");
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_free() failed bits comparison");

	ret = sys_bitarray_free(&ba_128, 16, 92);
	ba_128_expected[2] = 0x0F0F0F0F;
	ba_128_expected[3] = 0x0F0F0000;
	zassert_equal(ret, 0, "sys_bitarray_free() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_free() failed bits comparison");

	/* test in-between bundles */
	ba_128.bundles[0] = 0x7FFFFFFF;
	ba_128.bundles[1] = 0xFFFFFFFF;
	ba_128.bundles[2] = 0x00000000;
	ba_128.bundles[3] = 0x00000000;

	ba_128_expected[0] = 0x7FFFFFFF;
	ba_128_expected[1] = 0xFFFFFFFF;
	ba_128_expected[2] = 0xFFFFFFFF;
	ba_128_expected[3] = 0x00000003;

	ret = sys_bitarray_alloc(&ba_128, 34, &offset);
	zassert_equal(ret, 0, "sys_bitarray_alloc() failed: %d", ret);
	zassert_equal(offset, 64, "sys_bitarray_alloc() offset expected %d, got %d", 64, offset);
	zassert_true(cmp_u32_arrays(ba_128.bundles, ba_128_expected, ba_128.num_bundles),
		     "sys_bitarray_alloc() failed bits comparison");
}

static inline size_t count_bits(uint32_t val)
{
	/* Implements Brian Kernighan’s Algorithm
	 * to count bits.
	 */

	size_t cnt = 0;

	while (val != 0) {
		val = val & (val - 1);
		cnt++;
	}

	return cnt;
}

size_t get_bitarray_popcnt(sys_bitarray_t *ba)
{
	size_t popcnt = 0;
	unsigned int idx;

	for (idx = 0; idx < ba->num_bundles; idx++) {
		popcnt += count_bits(ba->bundles[idx]);
	}

	return popcnt;
}

void alloc_and_free_loop(int divisor)
{
	int ret;
	size_t offset;
	size_t bit;
	size_t num_bits;
	size_t cur_popcnt;
	size_t expected_popcnt = 0;

	SYS_BITARRAY_DEFINE(ba, 234);

	printk("Testing bit array alloc and free with divisor %d\n", divisor);

	for (bit = 0U; bit < ba.num_bits; ++bit) {
		cur_popcnt = get_bitarray_popcnt(&ba);
		zassert_equal(cur_popcnt, expected_popcnt,
			      "bit count expected %u, got %u (at bit %u)",
			      expected_popcnt, cur_popcnt, bit);

		/* Allocate half of remaining bits */
		num_bits = (ba.num_bits - bit) / divisor;

		ret = sys_bitarray_alloc(&ba, num_bits, &offset);
		if (num_bits == 0) {
			zassert_not_equal(ret, 0,
					  "sys_bitarray_free() should fail but not (bit %u)",
					  bit);
		} else {
			zassert_equal(ret, 0,
				      "sys_bitarray_alloc() failed (%d) at bit %u",
				      ret, bit);
			zassert_equal(offset, bit,
				      "sys_bitarray_alloc() offset expected %d, got %d",
				      bit, offset);

			expected_popcnt += num_bits;
		}

		cur_popcnt = get_bitarray_popcnt(&ba);
		zassert_equal(cur_popcnt, expected_popcnt,
			      "bit count expected %u, got %u (at bit %u)",
			      expected_popcnt, cur_popcnt, bit);

		/* Free all but the first bit of allocated region */
		ret = sys_bitarray_free(&ba, (num_bits - 1), (bit + 1));
		if ((num_bits == 0) || ((num_bits - 1) == 0)) {
			zassert_not_equal(ret, 0,
					  "sys_bitarray_free() should fail but not (bit %u)",
					  bit);
		} else {
			zassert_equal(ret, 0,
				      "sys_bitarray_free() failed (%d) at bit %u",
				      ret, (bit + 1));

			expected_popcnt -= num_bits - 1;
		}
	}
}

void alloc_and_free_interval(void)
{
	int ret;
	size_t cnt;
	size_t offset;
	size_t expected_offset;
	size_t expected_popcnt, cur_popcnt;

	/* Make sure number of bits is multiple of 8 */
	SYS_BITARRAY_DEFINE(ba, 152);

	printk("Testing bit array interval alloc and free\n");

	/* Pre-populate the bits so that 4-bit already allocated,
	 * then 4 free bits, and repeat.
	 */
	for (cnt = 0; cnt < ba.num_bundles; cnt++) {
		ba.bundles[cnt] = 0x0F0F0F0F;
	}

	expected_offset = 4;
	expected_popcnt = get_bitarray_popcnt(&ba);
	for (cnt = 0; cnt <= (ba.num_bits / 8); cnt++) {

		ret = sys_bitarray_alloc(&ba, 4, &offset);
		if (cnt == (ba.num_bits / 8)) {
			zassert_not_equal(ret, 0,
					  "sys_bitarray_free() should fail but not (cnt %u)",
					  cnt);
		} else {
			zassert_equal(ret, 0,
				      "sys_bitarray_alloc() failed (%d) (cnt %u)",
				      ret, cnt);

			zassert_equal(offset, expected_offset,
				      "offset expected %u, got %u (cnt %u)",
				      expected_offset, offset, cnt);

			expected_popcnt += 4;

			cur_popcnt = get_bitarray_popcnt(&ba);
			zassert_equal(cur_popcnt, expected_popcnt,
				      "bit count expected %u, got %u (cnt %u)",
				      expected_popcnt, cur_popcnt, cnt);


			expected_offset += 8;
		}
	}
}

/**
 * @brief Test bitarrays allocation and free
 *
 * @see sys_bitarray_alloc()
 * @see sys_bitarray_free()
 */
ZTEST(bitarray, test_bitarray_alloc_free)
{
	int i;

	/* Bitarrays have embedded spinlocks and can't on the stack. */
	if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		ztest_test_skip();
	}

	alloc_and_free_predefined();

	i = 1;
	while (i < 65) {
		alloc_and_free_loop(i);

		i *= 2;
	}

	alloc_and_free_interval();
}

ZTEST(bitarray, test_bitarray_region_set_clear)
{
	int ret;

	/* Bitarrays have embedded spinlocks and can't on the stack. */
	if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		ztest_test_skip();
	}

	uint32_t ba_expected[4];

	SYS_BITARRAY_DEFINE(ba, 64);

	printk("Testing bit array region bit tests\n");

	/* Pre-populate the bits */
	ba.bundles[0] = 0xFF0F0F0F;
	ba.bundles[1] = 0x0F0F0FFF;

	zassert_true(sys_bitarray_is_region_set(&ba,  4,  0));
	zassert_true(sys_bitarray_is_region_set(&ba, 12, 32));
	zassert_true(sys_bitarray_is_region_set(&ba,  8, 32));
	zassert_true(sys_bitarray_is_region_set(&ba, 14, 30));
	zassert_true(sys_bitarray_is_region_set(&ba, 20, 24));

	zassert_false(sys_bitarray_is_region_cleared(&ba,  4,  0));
	zassert_false(sys_bitarray_is_region_cleared(&ba, 12, 32));
	zassert_false(sys_bitarray_is_region_cleared(&ba,  8, 32));
	zassert_false(sys_bitarray_is_region_cleared(&ba, 14, 30));
	zassert_false(sys_bitarray_is_region_cleared(&ba, 20, 24));

	ba.bundles[0] = ~ba.bundles[0];
	ba.bundles[1] = ~ba.bundles[1];

	zassert_true(sys_bitarray_is_region_cleared(&ba,  4,  0));
	zassert_true(sys_bitarray_is_region_cleared(&ba, 12, 32));
	zassert_true(sys_bitarray_is_region_cleared(&ba,  8, 32));
	zassert_true(sys_bitarray_is_region_cleared(&ba, 14, 30));
	zassert_true(sys_bitarray_is_region_cleared(&ba, 20, 24));

	zassert_false(sys_bitarray_is_region_set(&ba,  4,  0));
	zassert_false(sys_bitarray_is_region_set(&ba, 12, 32));
	zassert_false(sys_bitarray_is_region_set(&ba,  8, 32));
	zassert_false(sys_bitarray_is_region_set(&ba, 14, 30));
	zassert_false(sys_bitarray_is_region_set(&ba, 20, 24));

	zassert_false(sys_bitarray_is_region_set(&ba, 10, 60));
	zassert_false(sys_bitarray_is_region_cleared(&ba, 10, 60));
	zassert_false(sys_bitarray_is_region_set(&ba, 8, 120));
	zassert_false(sys_bitarray_is_region_cleared(&ba, 8, 120));

	printk("Testing bit array region bit manipulations\n");

	/* Pre-populate the bits */
	ba.bundles[0] = 0xFF0F0F0F;
	ba.bundles[1] = 0x0F0F0FFF;

	/* Expected values */
	ba_expected[0] = 0xFF0F0F0F;
	ba_expected[1] = 0x0F0F0FFF;

	ret = sys_bitarray_set_region(&ba, 4, 0);
	zassert_equal(ret, 0, "sys_bitarray_set_region() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_set_region() failed bits comparison");

	ret = sys_bitarray_set_region(&ba, 4, 4);
	ba_expected[0] = 0xFF0F0FFF;
	zassert_equal(ret, 0, "sys_bitarray_set_region() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_set_region() failed bits comparison");

	ret = sys_bitarray_clear_region(&ba, 4, 4);
	ba_expected[0] = 0xFF0F0F0F;
	zassert_equal(ret, 0, "sys_bitarray_clear_region() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_clear_region() failed bits comparison");

	ret = sys_bitarray_clear_region(&ba, 14, 30);
	ba_expected[0] = 0x3F0F0F0F;
	ba_expected[1] = 0x0F0F0000;
	zassert_equal(ret, 0, "sys_bitarray_clear_region() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_clear_region() failed bits comparison");

	ret = sys_bitarray_set_region(&ba, 14, 30);
	ba_expected[0] = 0xFF0F0F0F;
	ba_expected[1] = 0x0F0F0FFF;
	zassert_equal(ret, 0, "sys_bitarray_set_region() failed: %d", ret);
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_set_region() failed bits comparison");

	ret = sys_bitarray_set_region(&ba, 10, 60);
	zassert_equal(ret, -EINVAL, "sys_bitarray_set_region() should fail but not");
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_set_region() failed bits comparison");

	ret = sys_bitarray_set_region(&ba, 8, 120);
	zassert_equal(ret, -EINVAL, "sys_bitarray_set_region() should fail but not");
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_set_region() failed bits comparison");

	ret = sys_bitarray_clear_region(&ba, 10, 60);
	zassert_equal(ret, -EINVAL, "sys_bitarray_clear_region() should fail but not");
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_clear_region() failed bits comparison");

	ret = sys_bitarray_clear_region(&ba, 8, 120);
	zassert_equal(ret, -EINVAL, "sys_bitarray_clear_region() should fail but not");
	zassert_true(cmp_u32_arrays(ba.bundles, ba_expected, ba.num_bundles),
		     "sys_bitarray_clear_region() failed bits comparison");

	SYS_BITARRAY_DEFINE(bw, 128);

	/* Pre-populate the bits */
	bw.bundles[0] = 0xFF0F0F0F;
	bw.bundles[1] = 0xF0000000;
	bw.bundles[2] = 0xFFFFFFFF;
	bw.bundles[3] = 0x0000000F;

	zassert_true(sys_bitarray_is_region_set(&bw, 40, 60));
	zassert_false(sys_bitarray_is_region_cleared(&bw, 40, 60));

	bw.bundles[2] = 0xFFFEEFFF;

	zassert_false(sys_bitarray_is_region_set(&bw, 40, 60));
	zassert_false(sys_bitarray_is_region_cleared(&bw, 40, 60));

	bw.bundles[1] = 0x0FFFFFFF;
	bw.bundles[2] = 0x00000000;
	bw.bundles[3] = 0xFFFFFFF0;

	zassert_true(sys_bitarray_is_region_cleared(&bw, 40, 60));
	zassert_false(sys_bitarray_is_region_set(&bw, 40, 60));

	bw.bundles[2] = 0x00011000;

	zassert_false(sys_bitarray_is_region_cleared(&bw, 40, 60));
	zassert_false(sys_bitarray_is_region_set(&bw, 40, 60));
}

/**
 * @brief Test find MSB and LSB operations
 *
 * @details Verify the functions that find out the most significant
 * bit and least significant bit work as expected.
 *
 * @see find_msb_set(), find_lsb_set()
 */
ZTEST(bitarray, test_ffs)
{
	uint32_t value;
	unsigned int bit;

	/* boundary test, input is min */
	value = 0x0;
	zassert_equal(find_msb_set(value), 0, "MSB is not matched");
	zassert_equal(find_lsb_set(value), 0, "LSB is not matched");

	/* boundary test, input is min + 1 */
	value = 0x00000001;
	zassert_equal(find_msb_set(value), 1, "MSB is not matched");
	zassert_equal(find_lsb_set(value), 1, "LSB is not matched");

	/* average value test */
	value = 0x80000000;
	zassert_equal(find_msb_set(value), 32, "MSB is not matched");
	zassert_equal(find_lsb_set(value), 32, "LSB is not matched");

	/* mediate value test */
	value = 0x000FF000;
	zassert_equal(find_msb_set(value), 20, "MSB is not matched");
	zassert_equal(find_lsb_set(value), 13, "LSB is not matched");

	/* boundary test, input is max */
	value = 0xffffffff;
	zassert_equal(find_msb_set(value), 32, "MSB is not matched");
	zassert_equal(find_lsb_set(value), 1, "LSB is not matched");

	/* boundary test, input is max - 1 */
	value = 0xfffffffe;
	zassert_equal(find_msb_set(value), 32, "MSB is not matched");
	zassert_equal(find_lsb_set(value), 2, "LSB is not matched");

	/* equivalent class testing, each bit means a class */
	for (bit = 0; bit < 32 ; bit++) {
		value = 1UL << bit;
		zassert_equal(find_msb_set(value), bit + 1, "MSB is not matched");
		zassert_equal(find_lsb_set(value), bit + 1, "LSB is not matched");
	}
}
extern void *common_setup(void);
ZTEST_SUITE(bitarray, NULL, common_setup, NULL, NULL, NULL);

/**
 * @}
 */

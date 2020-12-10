/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <arch/cpu.h>

#include <tc_util.h>
#include <sys/bitfield.h>

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
}

#define FAIL_DEC_MSG_FMT "Z_BITFIELD_DECLARE with size %i failed"
#define FAIL_ALLOC_MSG_FMT "z_bitfield_alloc with region size %i allocated incorrectly"
#define FAIL_ALLOC_RET_MSG_FMT "z_bitfield_alloc with region size %i returned incorrect result"
#define FAIL_ALLOC_OFFSET_MSG_FMT "z_bitfield_alloc with region size %i gave incorrect offset"
#define FAIL_FREE_MSG_FMT "z_bitfield_free with region size %i and offset %i failed"
#define FREE ~0 // If this is changed, all test cases must be changed

Z_BITFIELD_DECLARE(bf_1_bit, 1)
Z_BITFIELD_DECLARE(bf_32_bit, 32)
Z_BITFIELD_DECLARE(bf_33_bit, 33)
Z_BITFIELD_DECLARE(bf_64_bit, 64)
Z_BITFIELD_DECLARE(bf_65_bit, 65)
Z_BITFIELD_DECLARE(bf_128_bit, 128)
Z_BITFIELD_DECLARE(bf_129_bit, 129)

// Helper function to compare actual and expected bitfield
bool is_actual_bf_expected(unsigned long *bf_actual, unsigned long *bf_expected, size_t len)
{
	bool are_equal = true;

	for (size_t i = 0; i < len; i++) {
		if (bf_actual[i] != bf_expected[i]) {
			are_equal = false;
			break;
		}
	}

	return are_equal;
}

/**
 * @brief Test bitfield declare, allocation and free
 * 
 * @see Z_BITFIELD_DECLARE(), z_bitfield_alloc(), z_bitfield_free()
 */
void test_bitfield_declare_alloc_and_free(void)
{
	/* Test Z_BITFIELD_DECLARE by asserting that a sufficent number of longs
	 * in the declared array are set as free to represent the number of bits
	 */
	
	// 32 and 64 bit test cases
	zassert_equal(bf_1_bit[0], FREE, FAIL_DEC_MSG_FMT, 1); // 1 bit -> 1 long
	zassert_equal(bf_32_bit[0], FREE, FAIL_DEC_MSG_FMT, 32);
	zassert_equal(bf_33_bit[0], FREE, FAIL_DEC_MSG_FMT, 33);
	zassert_equal(bf_64_bit[0], FREE, FAIL_DEC_MSG_FMT, 64); 
	zassert_equal(bf_65_bit[0], FREE, FAIL_DEC_MSG_FMT, 65);
	zassert_equal(bf_128_bit[0], FREE, FAIL_DEC_MSG_FMT, 128);
	zassert_equal(bf_129_bit[0], FREE, FAIL_DEC_MSG_FMT, 129);

	zassert_equal(bf_65_bit[1], FREE, FAIL_DEC_MSG_FMT, 65);
	zassert_equal(bf_128_bit[1], FREE, FAIL_DEC_MSG_FMT, 128);
	zassert_equal(bf_129_bit[1], FREE, FAIL_DEC_MSG_FMT, 129);

	zassert_equal(bf_129_bit[2], FREE, FAIL_DEC_MSG_FMT, 129);

	// 32 bit test cases only
	#ifndef CONFIG_64BIT
	zassert_equal(bf_33_bit[1], FREE, FAIL_DEC_MSG_FMT, 33); // 33 bits -> 2 longs
	zassert_equal(bf_64_bit[1], FREE, FAIL_DEC_MSG_FMT, 64); // 64 -> 2

	zassert_equal(bf_65_bit[2], FREE, FAIL_DEC_MSG_FMT, 65); // etc.

	zassert_equal(bf_128_bit[2], FREE, FAIL_DEC_MSG_FMT, 128);
	zassert_equal(bf_128_bit[3], FREE, FAIL_DEC_MSG_FMT, 128);

	zassert_equal(bf_129_bit[3], FREE, FAIL_DEC_MSG_FMT, 129);
	zassert_equal(bf_129_bit[4], FREE, FAIL_DEC_MSG_FMT, 129);
	#endif /* !CONFIG_64BIT */

	/* Test z_bitfield_alloc */
	int ret = 0;
	size_t offset = 0;

	#ifdef CONFIG_64BIT
	#else
	size_t bf_alloc_0F_bf_size = 114;
	size_t bf_alloc_0F_region_size = 5;
	unsigned long bf_alloc_0F_actual[] = {0x0F0F0F8F0, 0x0F0F0F0F0, 0x0F0F0F0F0, 0x0F0F0FFFF}; // thought: off edge of arr
	unsigned long bf_alloc_0F_expected[] = {0x0F000F0F0, 0x0F0F0F0F0, 0x0F0F0F0F0, 0x0F0F0FFFF};
	size_t bf_alloc_0F_offset_expected = 76;
	int bf_alloc_0F_ret_expected = 0;
	#endif /* CONFIG_64BIT */

	// 0F0F...
	ret = z_bitfield_alloc(bf_alloc_0F_actual, bf_alloc_0F_bf_size, bf_alloc_0F_region_size, &offset);
	printf("Actual: %x , Expected: %x \n", bf_alloc_0F_actual[0], bf_alloc_0F_expected[0]);
	zassert_true(is_actual_bf_expected(bf_alloc_0F_actual, bf_alloc_0F_expected, 4), 
			FAIL_ALLOC_MSG_FMT, bf_alloc_0F_region_size);
	zassert_equal(ret, bf_alloc_0F_ret_expected, FAIL_ALLOC_RET_MSG_FMT);
	zassert_equal(offset, bf_alloc_0F_offset_expected, FAIL_ALLOC_OFFSET_MSG_FMT);

	/* Test z_bitfield_free */

	// Define test cases
	
	// 32 and 64 bit cases
	size_t bf_free_0_region_size = 0;
	size_t bf_free_0_offset = 0;

	size_t bf_free_1_region_size = 1;
	size_t bf_free_1_offset = 21;

	size_t bf_free_2_region_size = 2;
	size_t bf_free_2_offset = 21;

	size_t bf_free_begin_offset = 0;

	size_t bf_free_mid_offset = 12;
	size_t bf_free_mid_region_size = 14;

	size_t bf_free_end_region_size = 14;

	#ifdef CONFIG_64BIT
	unsigned long bf_free_0_actual[] = {0xDEADBEEFDEADBEEF}; // before
	unsigned long bf_free_0_expected[] = {0xDEADBEEFDEADBEEF}; // after

	unsigned long bf_free_1_actual[] = {0xDEADC0DEDEADC0DE};
	unsigned long bf_free_1_expected[] = {0xDEADC4DEDEADC0DE};

	unsigned long bf_free_2_actual[] = {0xDEADC0DEDEADC0DE};
	unsigned long bf_free_2_expected[] = {0xDEADC6DEDEADC0DE};

	size_t bf_free_begin_region_size = 37;
	unsigned long bf_free_begin_actual[] = {0x00000000006ADBEEF};
	unsigned long bf_free_begin_expected[] = {0xFFFFFFFFFEADBEEF};

	unsigned long bf_free_mid_actual[] = {0xffe0000fDEADBEEF};
	unsigned long bf_free_mid_expected[] = {0xffefffcfDEADBEEF};

	size_t bf_free_end_offset = 50;
	unsigned long bf_free_end_actual[] = {0xDEADC0DEDEADABAB};
	unsigned long bf_free_end_expected[] = {0xDEADC0DEDEADBFFF};

	size_t bf_free_across_region_size = 65;
	size_t bf_free_across_offset = 64;
	unsigned long bf_free_across_actual[] = {0x0, 0x0, 0x0, 0x0};
	unsigned long bf_free_across_expected[] = {0x0, 0xFFFFFFFFFFFFFFFF, 0x8000000000000000, 0x0};

	size_t bf_free_across_multiple_region_size = 190;
	size_t bf_free_across_multiple_offset = 65;
	unsigned long bf_free_across_multiple_actual[] = {0x0, 0x0, 0x0, 0x0};
	unsigned long bf_free_across_multiple_expected[] = 
		{0x0, 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFE};
	#else
	unsigned long bf_free_0_actual[] = {0xDEADBEEF}; // before
	unsigned long bf_free_0_expected[] = {0xDEADBEEF}; // after

	unsigned long bf_free_1_actual[] = {0xDEADC0DE};
	unsigned long bf_free_1_expected[] = {0xDEADC4DE};

	unsigned long bf_free_2_actual[] = {0xDEADC0DE};
	unsigned long bf_free_2_expected[] = {0xDEADC6DE};

	size_t bf_free_begin_region_size = 5;
	unsigned long bf_free_begin_actual[] = {0x06ADBEEF};
	unsigned long bf_free_begin_expected[] = {0xFEADBEEF};

	unsigned long bf_free_mid_actual[] = {0xffe0000f};
	unsigned long bf_free_mid_expected[] = {0xffefffcf};

	size_t bf_free_end_offset = 18;
	unsigned long bf_free_end_actual[] = {0xDEADABAB};
	unsigned long bf_free_end_expected[] = {0xDEADBFFF};

	size_t bf_free_across_region_size = 33;
	size_t bf_free_across_offset = 32;
	unsigned long bf_free_across_actual[] = {0x0, 0x0, 0x0, 0x0};
	unsigned long bf_free_across_expected[] = {0x0, 0xFFFFFFFF, 0x80000000, 0x0};

	size_t bf_free_across_multiple_region_size = 94;
	size_t bf_free_across_multiple_offset = 33;
	unsigned long bf_free_across_multiple_actual[] = {0x0, 0x0, 0x0, 0x0};
	unsigned long bf_free_across_multiple_expected[] = {0x0, 0x7FFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE};
	#endif /* CONFIG_64BIT */
	
	// Region size 0
	z_bitfield_free(bf_free_0_actual, bf_free_0_region_size, bf_free_0_offset);
	zassert_true(is_actual_bf_expected(bf_free_0_actual, bf_free_0_expected, 1), 
			FAIL_FREE_MSG_FMT, bf_free_0_region_size, bf_free_0_offset);

	// Region size 1
	z_bitfield_free(bf_free_1_actual, bf_free_1_region_size, bf_free_1_offset);
	zassert_true(is_actual_bf_expected(bf_free_1_actual, bf_free_1_expected, 1), 
			FAIL_FREE_MSG_FMT, bf_free_1_region_size, bf_free_1_offset);

	// Region size 2
	z_bitfield_free(bf_free_2_actual, bf_free_2_region_size, bf_free_2_offset);
	zassert_true(is_actual_bf_expected(bf_free_2_actual, bf_free_2_expected, 1), 
			FAIL_FREE_MSG_FMT, bf_free_2_region_size, bf_free_2_offset);

	// Beginning of one long
	z_bitfield_free(bf_free_begin_actual, bf_free_begin_region_size, bf_free_begin_offset);
	zassert_true(is_actual_bf_expected(bf_free_begin_actual, bf_free_begin_expected, 1), 
			FAIL_FREE_MSG_FMT, bf_free_begin_region_size, bf_free_begin_offset);

	// Middle of one long
	z_bitfield_free(bf_free_mid_actual, bf_free_mid_region_size, bf_free_mid_offset);
	zassert_true(is_actual_bf_expected(bf_free_mid_actual, bf_free_mid_expected, 1), 
			FAIL_FREE_MSG_FMT, bf_free_mid_region_size, bf_free_mid_offset);
	
	// End of one long
	z_bitfield_free(bf_free_end_actual, bf_free_end_region_size, bf_free_end_offset);
	zassert_true(is_actual_bf_expected(bf_free_end_actual, bf_free_end_expected, 1), 
			FAIL_FREE_MSG_FMT, bf_free_end_region_size, bf_free_end_offset);

	// Across two longs in array
	z_bitfield_free(bf_free_across_actual, bf_free_across_region_size, bf_free_across_offset);
	zassert_true(is_actual_bf_expected(bf_free_across_actual, bf_free_across_expected, 4), 
			FAIL_FREE_MSG_FMT, bf_free_across_region_size, bf_free_across_offset);

	// Across multiple longs in array
	z_bitfield_free(bf_free_across_multiple_actual, bf_free_across_multiple_region_size, bf_free_across_multiple_offset);
	zassert_true(is_actual_bf_expected(bf_free_across_multiple_actual, bf_free_across_multiple_expected, 4), 
			FAIL_FREE_MSG_FMT, bf_free_across_multiple_region_size, bf_free_across_multiple_offset);
}

/**
 * @}
 */

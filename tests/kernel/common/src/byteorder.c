/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>

#include <misc/byteorder.h>

/**
 * @addtogroup kernel_common_tests
 * @{
 */
/**
 * @brief Test swapping for memory contents
 *
 * @details Verify the functionality provided by
 * sys_memcpy_swap()
 *
 * @see sys_memcpy_swap()
 */
void test_byteorder_memcpy_swap(void)
{
	u8_t buf_orig[8] = { 0x00, 0x01, 0x02, 0x03,
			     0x04, 0x05, 0x06, 0x07 };
	u8_t buf_chk[8] = { 0x07, 0x06, 0x05, 0x04,
			    0x03, 0x02, 0x01, 0x00 };
	u8_t buf_dst[8] = { 0 };

	sys_memcpy_swap(buf_dst, buf_orig, 8);
	zassert_true((memcmp(buf_dst, buf_chk, 8) == 0),
		     "Swap memcpy failed");

	sys_memcpy_swap(buf_dst, buf_chk, 8);
	zassert_true((memcmp(buf_dst, buf_orig, 8) == 0),
		     "Swap memcpy failed");
}

/**
 * @brief Test sys_mem_swap() functionality
 *
 * @details Test if sys_mem_swap() reverses the contents
 *
 * @see sys_mem_swap()
 */
void test_byteorder_mem_swap(void)
{
	u8_t buf_orig_1[8] = { 0x00, 0x01, 0x02, 0x03,
			       0x04, 0x05, 0x06, 0x07 };
	u8_t buf_orig_2[11] = { 0x00, 0x01, 0x02, 0x03,
				0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0xa0 };
	u8_t buf_chk_1[8] = { 0x07, 0x06, 0x05, 0x04,
			      0x03, 0x02, 0x01, 0x00 };
	u8_t buf_chk_2[11] = { 0xa0, 0x09, 0x08, 0x07,
			       0x06, 0x05, 0x04, 0x03,
			       0x02, 0x01, 0x00 };

	sys_mem_swap(buf_orig_1, 8);
	zassert_true((memcmp(buf_orig_1, buf_chk_1, 8) == 0),
		     "Swapping buffer failed");

	sys_mem_swap(buf_orig_2, 11);
	zassert_true((memcmp(buf_orig_2, buf_chk_2, 11) == 0),
		     "Swapping buffer failed");
}
/**
 * @}
 */

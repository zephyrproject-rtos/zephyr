/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include <lib/crc/crc16_sw.c>
#include <lib/crc/crc8_sw.c>

void test_crc16(void)
{
	u8_t test0[] = { };
	u8_t test1[] = { 'A' };
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert(crc16_ccitt(test0, sizeof(test0)) == 0x1d0f, "pass", "fail");
	zassert(crc16_ccitt(test1, sizeof(test1)) == 0x9479, "pass", "fail");
	zassert(crc16_ccitt(test2, sizeof(test2)) == 0xe5cc, "pass", "fail");
}

void test_crc8_ccitt(void)
{
	u8_t test0[] = { 0 };
	u8_t test1[] = { 'A' };
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, test0,
			   sizeof(test0)) == 0xF3, "pass", "fail");
	zassert(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, test1,
			   sizeof(test1)) == 0x33, "pass", "fail");
	zassert(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, test2,
			   sizeof(test2)) == 0xFB, "pass", "fail");
}

void test_main(void)
{
	ztest_test_suite(test_crc,
			 ztest_unit_test(test_crc16),
			 ztest_unit_test(test_crc8_ccitt));
	ztest_run_test_suite(test_crc);
}

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include <lib/crc/crc32_sw.c>
#include <lib/crc/crc16_sw.c>
#include <lib/crc/crc8_sw.c>

void test_crc32_ieee(void)
{
	u8_t test1[] = { 'A' };
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	u8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r' };

	zassert_equal(crc32_ieee(test1, sizeof(test1)), 0xD3D99E8B, NULL);
	zassert_equal(crc32_ieee(test2, sizeof(test2)), 0xCBF43926, NULL);
	zassert_equal(crc32_ieee(test3, sizeof(test3)), 0x20089AA4, NULL);
}

void test_crc16(void)
{
	u8_t test0[] = { };
	u8_t test1[] = { 'A' };
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert_equal(crc16(test0, sizeof(test0), 0x1021, 0xffff, true),
		      0x1d0f, NULL);
	zassert_equal(crc16(test1, sizeof(test1), 0x1021, 0xffff, true),
		      0x9479, NULL);
	zassert_equal(crc16(test2, sizeof(test2), 0x1021, 0xffff, true),
		      0xe5cc, NULL);
}

void test_crc16_ansi(void)
{
	u8_t test0[] = { };
	u8_t test1[] = { 'A' };
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert(crc16_ansi(test0, sizeof(test0)) == 0x800d, "pass", "fail");
	zassert(crc16_ansi(test1, sizeof(test1)) == 0x8f85, "pass", "fail");
	zassert(crc16_ansi(test2, sizeof(test2)) == 0x9ecf, "pass", "fail");
}

void test_crc16_ccitt(void)
{
	u8_t test0[] = { };
	u8_t test1[] = { 'A' };
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	u8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r', 0, 0 };
	u16_t crc;

	zassert_equal(crc16_ccitt(0, test0, sizeof(test0)), 0x0, NULL);
	zassert_equal(crc16_ccitt(0, test1, sizeof(test1)), 0x538d, NULL);
	zassert_equal(crc16_ccitt(0, test2, sizeof(test2)), 0x2189, NULL);

	/* Appending the CRC to a buffer and computing the CRC over
	 * the extended buffer leaves a residual of zero.
	 */
	crc = crc16_ccitt(0, test3, sizeof(test3) - sizeof(u16_t));
	test3[sizeof(test3)-2] = (u8_t)(crc >> 0);
	test3[sizeof(test3)-1] = (u8_t)(crc >> 8);

	zassert_equal(crc16_ccitt(0, test3, sizeof(test3)), 0, NULL);
}

void test_crc16_ccitt_for_ppp(void)
{
	/* Example capture including FCS from
	 * https://www.horo.ch/techno/ppp-fcs/examples_en.html
	 */
	u8_t test0[] = {
		0xff, 0x03, 0xc0, 0x21, 0x01, 0x01, 0x00, 0x17,
		0x02, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x05, 0x06,
		0x00, 0x2a, 0x2b, 0x78, 0x07, 0x02, 0x08, 0x02,
		0x0d, 0x03, 0x06, 0xa5, 0xf8
	};
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert_equal(crc16_ccitt(0xffff, test0, sizeof(test0)),
		      0xf0b8, NULL);
	zassert_equal(crc16_ccitt(0xffff, test2, sizeof(test2)) ^ 0xFFFF,
		      0x906e, NULL);
}

void test_crc16_itu_t(void)
{
	u8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert_equal(crc16_itu_t(0, test2, sizeof(test2)),
		      0x31c3, NULL);
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
			 ztest_unit_test(test_crc32_ieee),
			 ztest_unit_test(test_crc16),
			 ztest_unit_test(test_crc16_ansi),
			 ztest_unit_test(test_crc16_ccitt),
			 ztest_unit_test(test_crc16_ccitt_for_ppp),
			 ztest_unit_test(test_crc16_itu_t),
			 ztest_unit_test(test_crc8_ccitt));
	ztest_run_test_suite(test_crc);
}

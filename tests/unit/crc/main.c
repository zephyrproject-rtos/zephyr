/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/sys/crc.h>
#include "../../../lib/os/crc8_sw.c"
#include "../../../lib/os/crc16_sw.c"
#include "../../../lib/os/crc32_sw.c"
#include "../../../lib/os/crc32c_sw.c"
#include "../../../lib/os/crc7_sw.c"

void test_crc32c(void)
{
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r' };

	/* Single streams */
	zassert_equal(crc32_c(0, test1, sizeof(test1), true, true),
			0xE16DCDEE, NULL);
	zassert_equal(crc32_c(0, test2, sizeof(test2), true, true),
			0xE3069283, NULL);
	zassert_equal(crc32_c(0, test3, sizeof(test3), true, true),
			0xFCDEB58D, NULL);

	/* Continuous streams - test1, test2 and test3 are considered part
	 * of one big stream whose CRC needs to be calculated. Note that the
	 * CRC of the first string is passed over to the second crc calculation,
	 * second to third and so on.
	 */
	zassert_equal(crc32_c(0, test1, sizeof(test1), true, false),
			0x1E923211, NULL);
	zassert_equal(crc32_c(0x1E923211, test2, sizeof(test2), false, false),
			0xB2983B83, NULL);
	zassert_equal(crc32_c(0xB2983B83, test3, sizeof(test3), false, true),
			0x7D4F9D21, NULL);
}

void test_crc32_ieee(void)
{
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r' };

	zassert_equal(crc32_ieee(test1, sizeof(test1)), 0xD3D99E8B, NULL);
	zassert_equal(crc32_ieee(test2, sizeof(test2)), 0xCBF43926, NULL);
	zassert_equal(crc32_ieee(test3, sizeof(test3)), 0x20089AA4, NULL);
}

void test_crc16(void)
{
	uint8_t test[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	/* CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-16/KERMIT
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-kermit
	 * check=0x2189
	 * poly is 0x1021, reflected 0x8408
	 */
	zassert_equal(crc16_reflect(0x8408, 0x0, test, sizeof(test)), 0x2189, NULL);

	/* CRC-16/DECT-X
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-dect-x
	 * check=0x007f
	 */
	zassert_equal(crc16(0x0589, 0x0, test, sizeof(test)), 0x007f, NULL);
}

void test_crc16_ansi(void)
{
	uint8_t test[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	uint16_t crc16_c = crc16_ansi(test, sizeof(test));

	/* CRC-16/ANSI, CRC-16/MODBUS, CRC-16/USB, CRC-16/IBM
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-modbus
	 * check=0x4b37
	 * poly is 0x1021, reflected 0xA001
	 */
	zassert_equal(crc16_c, 0x4b37, NULL);
	zassert_equal(crc16_reflect(0xA001, 0xffff, test, sizeof(test)), crc16_c, NULL);
}

void test_crc16_ccitt(void)
{
	uint8_t test0[] = { };
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r', 0, 0 };
	uint16_t crc;

	zassert_equal(crc16_ccitt(0, test0, sizeof(test0)), 0x0, NULL);
	zassert_equal(crc16_ccitt(0, test1, sizeof(test1)), 0x538d, NULL);
	/* CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-16/KERMIT
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-kermit
	 * check=0x2189
	 */
	zassert_equal(crc16_ccitt(0, test2, sizeof(test2)), 0x2189, NULL);
	/* CRC-16/X-25, CRC-16/IBM-SDLC, CRC-16/ISO-HDLC
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-sdlc
	 * check=0x906e
	 */
	zassert_equal(crc16_ccitt(0xffff, test2, sizeof(test2)) ^ 0xffff,
		      0x906e, NULL);

	/* Appending the CRC to a buffer and computing the CRC over
	 * the extended buffer leaves a residual of zero.
	 */
	crc = crc16_ccitt(0, test3, sizeof(test3) - sizeof(uint16_t));
	test3[sizeof(test3)-2] = (uint8_t)(crc >> 0);
	test3[sizeof(test3)-1] = (uint8_t)(crc >> 8);

	zassert_equal(crc16_ccitt(0, test3, sizeof(test3)), 0, NULL);
}

void test_crc16_ccitt_for_ppp(void)
{
	/* Example capture including FCS from
	 * https://www.horo.ch/techno/ppp-fcs/examples_en.html
	 */
	uint8_t test0[] = {
		0xff, 0x03, 0xc0, 0x21, 0x01, 0x01, 0x00, 0x17,
		0x02, 0x06, 0x00, 0x0a, 0x00, 0x00, 0x05, 0x06,
		0x00, 0x2a, 0x2b, 0x78, 0x07, 0x02, 0x08, 0x02,
		0x0d, 0x03, 0x06, 0xa5, 0xf8
	};
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert_equal(crc16_ccitt(0xffff, test0, sizeof(test0)),
		      0xf0b8, NULL);
	zassert_equal(crc16_ccitt(0xffff, test2, sizeof(test2)) ^ 0xFFFF,
		      0x906e, NULL);
}

void test_crc16_itu_t(void)
{
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	/* CRC-16/XMODEM, CRC-16/ACORN, CRC-16/LTE
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-xmodem
	 * check=0x31c3
	 */
	zassert_equal(crc16_itu_t(0, test2, sizeof(test2)), 0x31c3, NULL);
	/* CRC16/CCITT-FALSE, CRC-16/IBM-3740, CRC-16/AUTOSAR
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-3740
	 * check=0x29b1
	 */
	zassert_equal(crc16_itu_t(0xffff, test2, sizeof(test2)), 0x29b1, NULL);
	/* CRC-16/GSM
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-gsm
	 * check=0xce3c
	 */
	zassert_equal(crc16_itu_t(0, test2, sizeof(test2)) ^ 0xffff, 0xce3c, NULL);

}

void test_crc8_ccitt(void)
{
	uint8_t test0[] = { 0 };
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, test0,
			   sizeof(test0)) == 0xF3, "pass", "fail");
	zassert(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, test1,
			   sizeof(test1)) == 0x33, "pass", "fail");
	zassert(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, test2,
			   sizeof(test2)) == 0xFB, "pass", "fail");
}

void test_crc7_be(void)
{
	uint8_t test0[] = { 0 };
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	zassert_equal(crc7_be(0, test0, sizeof(test0)), 0, NULL);
	zassert_equal(crc7_be(0, test1, sizeof(test1)), 0xDA, NULL);
	zassert_equal(crc7_be(0, test2, sizeof(test2)), 0xEA, NULL);
}

void test_crc8(void)
{
	uint8_t fcs, expected;

	uint8_t test0[] = { 0x00 };
	uint8_t test1[] = { 0xBE, 0xEF };
	uint8_t test2[] = { 0x07, 0x3F };        /* GSM 07.10 example */
	uint8_t test3[] = { 0x07, 0x3F, 0x89 };  /* GSM 07.10 example */
	uint8_t test4[] = { 0x03, 0x02, 0x0A, 0x38, 0x17, 0x00 };
	uint8_t test5[] = { 0x03, 0x3f, 0x01, 0x1c };  /* Our GSM 07.10 calc */

	fcs = crc8(test0, sizeof(test0), 0x00, 0x00, false);
	expected = 0x00;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test0, sizeof(test0), 0x31, 0x00, false);
	expected = 0x00;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test1, sizeof(test1), 0x07, 0x00, false);
	expected = 0x1a;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test1, sizeof(test1), 0x31, 0xff, false);
	expected = 0x92;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test1, sizeof(test1), 0x07, 0x00, false);
	expected = 0x1a;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test2, sizeof(test2), 0x31, 0x00, false);
	expected = 0x45;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test2, sizeof(test2), 0x31, 0xff, false);
	expected = 0xc4;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test2, sizeof(test2), 0x07, 0x00, false);
	expected = 0xd6;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test2, sizeof(test2), 0x07, 0xff, false);
	expected = 0x01;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test2, sizeof(test2), 0xe0, 0xff, true);
	expected = 0x76;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test3, sizeof(test3), 0xe0, 0xff, true);
	expected = 0xcf;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test3, sizeof(test3), 0x07, 0xff, false);
	expected = 0xb1;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test4, sizeof(test4), 0x31, 0x00, false);
	expected = 0x3a;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test4, sizeof(test4), 0x07, 0x00, false);
	expected = 0xaf;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test4, sizeof(test4), 0x9b, 0xff, false);
	expected = 0xf0;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test4, sizeof(test4), 0x1d, 0xfd, false);
	expected = 0x49;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);

	fcs = crc8(test5, sizeof(test5), 0xe0, 0xff, true);
	expected = 0xcf;
	zassert_equal(fcs, expected, "0x%02x vs 0x%02x", fcs, expected);
}

void test_main(void)
{
	ztest_test_suite(test_crc,
			 ztest_unit_test(test_crc32c),
			 ztest_unit_test(test_crc32_ieee),
			 ztest_unit_test(test_crc16),
			 ztest_unit_test(test_crc16_ansi),
			 ztest_unit_test(test_crc16_ccitt),
			 ztest_unit_test(test_crc16_ccitt_for_ppp),
			 ztest_unit_test(test_crc16_itu_t),
			 ztest_unit_test(test_crc8_ccitt),
			 ztest_unit_test(test_crc7_be),
			 ztest_unit_test(test_crc8));
	ztest_run_test_suite(test_crc);
}

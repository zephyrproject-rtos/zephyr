/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/crc.h>
#include "../../../subsys/crc/crc8_sw.c"
#include "../../../subsys/crc/crc16_sw.c"
#include "../../../subsys/crc/crc32_sw.c"
#include "../../../subsys/crc/crc32c_sw.c"
#include "../../../subsys/crc/crc7_sw.c"
#include "../../../subsys/crc/crc24_sw.c"
#include "../../../subsys/crc/crc4_sw.c"
#include "../../../subsys/crc/crc32k_4_2_sw.c"

ZTEST(crc, test_crc32_k_4_2)
{
	uint8_t test1[] = "A";
	uint8_t test2[] = "123456789";
	uint8_t test3[] = "Zephyr";

	const uint32_t TEST2_CRC = 0x3ee83603;

	zassert_equal(crc32_k_4_2_update(0xFFFFFFFF, test1, sizeof(test1) - 1), 0x2d098604);
	zassert_equal(crc32_k_4_2_update(0xFFFFFFFF, test2, sizeof(test2) - 1), TEST2_CRC);
	zassert_equal(crc32_k_4_2_update(0xFFFFFFFF, test3, sizeof(test3) - 1), 0xacf334b2);

	/* test iteration */
	uint32_t crc = 0xFFFFFFFF;

	for (size_t i = 0; i < sizeof(test2) - 1; i++) {
		crc = crc32_k_4_2_update(crc, &test2[i], 1);
	}
	zassert_equal(crc, TEST2_CRC);
}

ZTEST(crc, test_crc32c)
{
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r' };

	/* Single streams */
	zassert_equal(crc32_c(0, test1, sizeof(test1), true, true),
			0xE16DCDEE);
	zassert_equal(crc32_c(0, test2, sizeof(test2), true, true),
			0xE3069283);
	zassert_equal(crc32_c(0, test3, sizeof(test3), true, true),
			0xFCDEB58D);

	/* Continuous streams - test1, test2 and test3 are considered part
	 * of one big stream whose CRC needs to be calculated. Note that the
	 * CRC of the first string is passed over to the second crc calculation,
	 * second to third and so on.
	 */
	zassert_equal(crc32_c(0, test1, sizeof(test1), true, false),
			0x1E923211);
	zassert_equal(crc32_c(0x1E923211, test2, sizeof(test2), false, false),
			0xB2983B83);
	zassert_equal(crc32_c(0xB2983B83, test3, sizeof(test3), false, true),
			0x7D4F9D21);
}

struct crc32_ieee_case {
	uint8_t data[12];
	uint8_t len;
	uint32_t expected;
};

static const struct crc32_ieee_case crc32_ieee_cases[] = {
	{{'A'},                               1, 0xD3D99E8BU},
	{{'1', '2', '3', '4', '5', '6', '7', '8', '9'}, 9, 0xCBF43926U},
	{{'Z', 'e', 'p', 'h', 'y', 'r'},           6, 0x20089AA4U},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(crc32_ieee_params, crc32_ieee_cases);

ZTEST_P(crc, test_crc32_ieee)
{
	const struct crc32_ieee_case *tc = ZTEST_GET_PARAM_PTR(struct crc32_ieee_case);

	zassert_equal(crc32_ieee(tc->data, tc->len), tc->expected);
}

ZTEST(crc, test_crc24_pgp)
{
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r' };

	zassert_equal(crc24_pgp(test1, sizeof(test1)), 0x00FE86FA);
	zassert_equal(crc24_pgp(test2, sizeof(test2)), 0x0021CF02);
	zassert_equal(crc24_pgp(test3, sizeof(test3)), 0x004662E9);

	/* Compute a CRC in several steps */
	zassert_equal(crc24_pgp_update(CRC24_PGP_INITIAL_VALUE, test2, 3), 0x0009DF67);
	zassert_equal(crc24_pgp_update(0x0009DF67, test2 + 3, 2), 0x00BA353A);
	zassert_equal(crc24_pgp_update(0x00BA353A, test2 + 5, 4), 0x0021CF02);
}

ZTEST(crc, test_crc24q_rtcm3)
{
	uint8_t test1[] = {0xD3, 0x00, 0x04, 0x4C, 0xE0, 0x00, 0x80};
	uint8_t test2[] = {0xD3, 0x00, 0x04, 0x4C, 0xE0, 0x00, 0x80, 0xED, 0xED, 0xD6};

	zassert_equal(crc24q_rtcm3(test1, sizeof(test1)), 0xEDEDD6);
	zassert_equal(crc24q_rtcm3(test2, sizeof(test2)), 0x000000);
}

ZTEST(crc, test_crc16)
{
	uint8_t test[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	/* CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-16/KERMIT
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-kermit
	 * check=0x2189
	 * poly is 0x1021, reflected 0x8408
	 */
	zassert_equal(crc16_reflect(0x8408, 0x0, test, sizeof(test)), 0x2189);

	/* CRC-16/DECT-X
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-dect-x
	 * check=0x007f
	 */
	zassert_equal(crc16(0x0589, 0x0, test, sizeof(test)), 0x007f);
}

ZTEST(crc, test_crc16_ansi)
{
	uint8_t test[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	uint16_t crc16_c = crc16_ansi(test, sizeof(test));

	/* CRC-16/ANSI, CRC-16/MODBUS, CRC-16/USB, CRC-16/IBM
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-modbus
	 * check=0x4b37
	 * poly is 0x1021, reflected 0xA001
	 */
	zassert_equal(crc16_c, 0x4b37);
	zassert_equal(crc16_reflect(0xA001, 0xffff, test, sizeof(test)), crc16_c);
}

ZTEST(crc, test_crc16_ccitt)
{
	uint8_t test0[] = { };
	uint8_t test1[] = { 'A' };
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	uint8_t test3[] = { 'Z', 'e', 'p', 'h', 'y', 'r', 0, 0 };
	uint16_t crc;

	zassert_equal(crc16_ccitt(0, test0, sizeof(test0)), 0x0);
	zassert_equal(crc16_ccitt(0, test1, sizeof(test1)), 0x538d);
	/* CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-16/KERMIT
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-kermit
	 * check=0x2189
	 */
	zassert_equal(crc16_ccitt(0, test2, sizeof(test2)), 0x2189);
	/* CRC-16/X-25, CRC-16/IBM-SDLC, CRC-16/ISO-HDLC
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-sdlc
	 * check=0x906e
	 */
	zassert_equal(crc16_ccitt(0xffff, test2, sizeof(test2)) ^ 0xffff,
		      0x906e);

	/* Appending the CRC to a buffer and computing the CRC over
	 * the extended buffer leaves a residual of zero.
	 */
	crc = crc16_ccitt(0, test3, sizeof(test3) - sizeof(uint16_t));
	test3[sizeof(test3)-2] = (uint8_t)(crc >> 0);
	test3[sizeof(test3)-1] = (uint8_t)(crc >> 8);

	zassert_equal(crc16_ccitt(0, test3, sizeof(test3)), 0);
}

ZTEST(crc, test_crc16_ccitt_for_ppp)
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
		      0xf0b8);
	zassert_equal(crc16_ccitt(0xffff, test2, sizeof(test2)) ^ 0xFFFF,
		      0x906e);
}

ZTEST(crc, test_crc16_itu_t)
{
	uint8_t test2[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	/* CRC-16/XMODEM, CRC-16/ACORN, CRC-16/LTE
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-xmodem
	 * check=0x31c3
	 */
	zassert_equal(crc16_itu_t(0, test2, sizeof(test2)), 0x31c3);
	/* CRC16/CCITT-FALSE, CRC-16/IBM-3740, CRC-16/AUTOSAR
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-ibm-3740
	 * check=0x29b1
	 */
	zassert_equal(crc16_itu_t(0xffff, test2, sizeof(test2)), 0x29b1);
	/* CRC-16/GSM
	 * https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-gsm
	 * check=0xce3c
	 */
	zassert_equal(crc16_itu_t(0, test2, sizeof(test2)) ^ 0xffff, 0xce3c);

}

struct crc4_case {
	uint8_t data[8];
	uint8_t len;
	bool reflect;
	uint8_t expected;
};

static const struct crc4_case crc4_cases[] = {
	{{'A'},                     1, true,  0x2},
	{{'Z', 'e', 'p', 'h', 'y', 'r'}, 6, true,  0x0},
	{{'A'},                     1, false, 0x4},
	{{'Z', 'e', 'p', 'h', 'y', 'r'}, 6, false, 0xE},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(crc4_params, crc4_cases);

ZTEST_P(crc, test_crc4)
{
	const struct crc4_case *tc = ZTEST_GET_PARAM_PTR(struct crc4_case);

	zassert_equal(crc4(tc->data, tc->len, 0x3U, 0x0U, tc->reflect), tc->expected);
}

ZTEST(crc, test_crc4_ti)
{
	uint8_t test1[] = {'Z', 'e', 'p'};

	zassert_equal(crc4_ti(0x0, test1, sizeof(test1)), 0xF);
	zassert_equal(crc4_ti(0x5, test1, sizeof(test1)), 0xB);
}

struct crc8_ccitt_case {
	uint8_t data[12];
	uint8_t len;
	uint8_t expected;
};

static const struct crc8_ccitt_case crc8_ccitt_cases[] = {
	{{0},                               1, 0xF3},
	{{'A'},                             1, 0x33},
	{{'1', '2', '3', '4', '5', '6', '7', '8', '9'}, 9, 0xFB},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(crc8_ccitt_params, crc8_ccitt_cases);

ZTEST_P(crc, test_crc8_ccitt)
{
	const struct crc8_ccitt_case *tc = ZTEST_GET_PARAM_PTR(struct crc8_ccitt_case);

	zassert_equal(crc8_ccitt(CRC8_CCITT_INITIAL_VALUE, tc->data, tc->len), tc->expected);
}

struct crc8_rohc_case {
	uint8_t data[12];
	uint8_t len;
	uint8_t expected;
};

static const struct crc8_rohc_case crc8_rohc_cases[] = {
	{{0},                               1, 0xcf},
	{{'A'},                             1, 0x2e},
	{{'1', '2', '3', '4', '5', '6', '7', '8', '9'}, 9, 0xd0},
	{{0x07, 0x3F},                      2, 0x76}, /* GSM 07.10 example */
	{{0x07, 0x3F, 0x89},                3, 0xcf}, /* GSM 07.10 example */
	{{0x03, 0x3f, 0x01, 0x1c},          4, 0xcf}, /* Our GSM 07.10 calc */
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(crc8_rohc_params, crc8_rohc_cases);

ZTEST_P(crc, test_crc8_rohc)
{
	const struct crc8_rohc_case *tc = ZTEST_GET_PARAM_PTR(struct crc8_rohc_case);

	zassert_equal(crc8_rohc(CRC8_ROHC_INITIAL_VALUE, tc->data, tc->len), tc->expected);
}

struct crc7_be_case {
	uint8_t data[12];
	uint8_t len;
	uint8_t expected;
};

static const struct crc7_be_case crc7_be_cases[] = {
	{{0},                               1, 0x00},
	{{'A'},                             1, 0xDA},
	{{'1', '2', '3', '4', '5', '6', '7', '8', '9'}, 9, 0xEA},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(crc7_be_params, crc7_be_cases);

ZTEST_P(crc, test_crc7_be)
{
	const struct crc7_be_case *tc = ZTEST_GET_PARAM_PTR(struct crc7_be_case);

	zassert_equal(crc7_be(0, tc->data, tc->len), tc->expected);
}

struct crc8_case {
	uint8_t data[8];
	uint8_t len;
	uint8_t poly;
	uint8_t init;
	bool reflect;
	uint8_t expected;
};

static const struct crc8_case crc8_cases[] = {
	/* test0 = {0x00} */
	{{0x00},                              1, 0x00, 0x00, false, 0x00},
	{{0x00},                              1, 0x31, 0x00, false, 0x00},
	/* test1 = {0xBE, 0xEF} */
	{{0xBE, 0xEF},                        2, 0x07, 0x00, false, 0x1a},
	{{0xBE, 0xEF},                        2, 0x31, 0xff, false, 0x92},
	{{0xBE, 0xEF},                        2, 0x07, 0x00, false, 0x1a},
	/* test2 = {0x07, 0x3F} (GSM 07.10 example) */
	{{0x07, 0x3F},                        2, 0x31, 0x00, false, 0x45},
	{{0x07, 0x3F},                        2, 0x31, 0xff, false, 0xc4},
	{{0x07, 0x3F},                        2, 0x07, 0x00, false, 0xd6},
	{{0x07, 0x3F},                        2, 0x07, 0xff, false, 0x01},
	{{0x07, 0x3F},                        2, 0xe0, 0xff, true,  0x76},
	/* test3 = {0x07, 0x3F, 0x89} (GSM 07.10 example) */
	{{0x07, 0x3F, 0x89},                  3, 0xe0, 0xff, true,  0xcf},
	{{0x07, 0x3F, 0x89},                  3, 0x07, 0xff, false, 0xb1},
	/* test4 = {0x03, 0x02, 0x0A, 0x38, 0x17, 0x00} */
	{{0x03, 0x02, 0x0A, 0x38, 0x17, 0x00}, 6, 0x31, 0x00, false, 0x3a},
	{{0x03, 0x02, 0x0A, 0x38, 0x17, 0x00}, 6, 0x07, 0x00, false, 0xaf},
	{{0x03, 0x02, 0x0A, 0x38, 0x17, 0x00}, 6, 0x9b, 0xff, false, 0xf0},
	{{0x03, 0x02, 0x0A, 0x38, 0x17, 0x00}, 6, 0x1d, 0xfd, false, 0x49},
	/* test5 = {0x03, 0x3f, 0x01, 0x1c} (Our GSM 07.10 calc) */
	{{0x03, 0x3f, 0x01, 0x1c},            4, 0xe0, 0xff, true,  0xcf},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(crc8_params, crc8_cases);

ZTEST_P(crc, test_crc8)
{
	const struct crc8_case *tc = ZTEST_GET_PARAM_PTR(struct crc8_case);
	uint8_t fcs = crc8(tc->data, tc->len, tc->poly, tc->init, tc->reflect);

	zassert_equal(fcs, tc->expected, "0x%02x vs 0x%02x", fcs, tc->expected);
}

ZTEST_SUITE(crc, NULL, NULL, NULL, NULL, NULL);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, crc, test_crc32_ieee, crc32_ieee_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, crc, test_crc4,       crc4_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, crc, test_crc8_ccitt, crc8_ccitt_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, crc, test_crc8_rohc,  crc8_rohc_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, crc, test_crc7_be,    crc7_be_params);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, crc, test_crc8,       crc8_params);

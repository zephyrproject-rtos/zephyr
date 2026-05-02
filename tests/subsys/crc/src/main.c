/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/crc.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

/* Define result of CRC computation */
#define RESULT_CRC8 0xB2

/**
 * @brief Test crc8 works
 */
ZTEST(crc_subsys, test_crc_8)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc8(data, sizeof(data), CRC8_REFLECT_POLY, 0x00, true), RESULT_CRC8);
}

/* Define result of CRC computation */
#define RESULT_CRC8_CCITT 0x4D

/**
 * @brief Test crc8_ccitt works
 */
ZTEST(crc_subsys, test_crc_8_ccitt)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc8_ccitt(0x00, data, sizeof(data)), RESULT_CRC8_CCITT);
}

/* Define result of CRC computation */
#define RESULT_CRC8_ROHC 0xB2

/**
 * @brief Test that crc_8_rohc works
 */
ZTEST(crc_subsys, test_crc_8_rohc)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc8_rohc(0x00, data, sizeof(data)), RESULT_CRC8_ROHC);
}

/* Define result of CRC computation */
#define RESULT_CRC16 0xE58F

/**
 * @brief Test that crc_16 works
 */
ZTEST(crc_subsys, test_crc_16)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc16(CRC16_POLY, CRC16_INIT_VAL, data, sizeof(data)), RESULT_CRC16);
}
/* Define result of CRC computation */
#define RESULT_CRC16_REFLECT 0xD543

/**
 * @brief Test that crc_16_reflect works
 */
ZTEST(crc_subsys, test_crc_16_reflect)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc16_reflect(CRC16_REFLECT_POLY, CRC16_INIT_VAL, data, sizeof(data)),
		      RESULT_CRC16_REFLECT);
}

/* Define result of CRC computation */
#define RESULT_CRC16_ANSI 0xDE03

/**
 * @brief Test that crc_16_ansi works
 */
ZTEST(crc_subsys, test_crc_16_ansi)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc16_ansi(data, sizeof(data)), RESULT_CRC16_ANSI);
}

/* Define result of CRC computation */
#define RESULT_CRC_CCITT 0x445C

/**
 * @brief Test that crc_16_ccitt works
 */
ZTEST(crc_subsys, test_crc_16_ccitt)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc16_ccitt(0x0000, data, sizeof(data)), RESULT_CRC_CCITT);
}

/* Define result of CRC computation */
#define RESULT_CRC16_ITU_T 0x8866

/**
 * @brief Test that crc_16_itu_t works
 */
ZTEST(crc_subsys, test_crc_16_itu_t)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc16_itu_t(0x0000, data, sizeof(data)), RESULT_CRC16_ITU_T);
}

/* Define result of CRC computation */
#define RESULT_CRC32_C 0xBB19ECB2

/**
 * @brief Test that crc32_c works
 */
ZTEST(crc_subsys, test_crc_32_c)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc32_c(0x000, data, sizeof(data), true, false), RESULT_CRC32_C);
}
/* Define result of CRC computation */
#define RESULT_CRC32_IEEE 0xCEA4A6C2

/**
 * @brief Test that crc_32_ieee works
 */
ZTEST(crc_subsys, test_crc_32_ieee)
{
	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	zassert_equal(crc32_ieee(data, sizeof(data)), RESULT_CRC32_IEEE);
}

/* Define result of CRC computation */
#define RESULT_CRC8_CCITT_REMAIN_1 0x57

/**
 * @brief Test crc8_ccitt_remain_1 work
 */
ZTEST(crc_subsys, test_crc_8_ccitt_remain_1)
{
	uint8_t data[9] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D};

	zassert_equal(crc8_ccitt(0x00, data, sizeof(data)), RESULT_CRC8_CCITT_REMAIN_1);
}

/* Define result of CRC computation */
#define RESULT_CRC8_ROHC_REMAIN_2 0x4F

/**
 * @brief Test that crc_8_rohc_remain_2 works
 */
ZTEST(crc_subsys, test_crc_8_rohc_remain_2)
{
	uint8_t data[10] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D, 0xFF};

	zassert_equal(crc8_rohc(0x00, data, sizeof(data)), RESULT_CRC8_ROHC_REMAIN_2);
}

/* Define result of CRC computation */
#define RESULT_CRC_CCITT_REMAIN_3 0x454B

/**
 * @brief Test that crc_16_ccitt_remain_3 works
 */
ZTEST(crc_subsys, test_crc_16_ccitt_remain_3)
{
	uint8_t data[11] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D, 0xFF, 0xE2};

	zassert_equal(crc16_ccitt(0x0000, data, sizeof(data)), RESULT_CRC_CCITT_REMAIN_3);
}

/* Define result of CRC computation */
#define RESULT_CRC16_ITU_T_REMAIN_1 0x917E

/**
 * @brief Test that crc_16_itu_t_remain_1 works
 */
ZTEST(crc_subsys, test_crc_16_itu_t_remain_1)
{
	uint8_t data[9] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D};

	zassert_equal(crc16_itu_t(0x0000, data, sizeof(data)), RESULT_CRC16_ITU_T_REMAIN_1);
}

ZTEST_SUITE(crc_subsys, NULL, NULL, NULL, NULL, NULL);

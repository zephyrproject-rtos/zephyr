/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/crc.h>

#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

#define WAIT_THREAD_STACK_SIZE 1024
#define WAIT_THREAD_PRIO       -10

static void wait_thread_entry(void *a, void *b, void *c);

K_THREAD_STACK_DEFINE(wait_thread_stack_area, WAIT_THREAD_STACK_SIZE);
struct k_thread wait_thread_data;

/* Define result of CRC computation */
#define RESULT_CRC_16_THREADSAFE 0xD543

/**
 * 1) Take the semaphore
 * 2) Sleep for 50 ms (to allow ztest main thread to attempt to acquire semaphore)
 * 3) Give the semaphore
 */
static void wait_thread_entry(void *a, void *b, void *c)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = CRC16_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	crc_begin(dev, &ctx);

	k_sleep(K_MSEC(50));

	crc_update(dev, &ctx, data, sizeof(data));
	crc_finish(dev, &ctx);
	zassert_equal(crc_verify(&ctx, RESULT_CRC_16_THREADSAFE), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_8 0xB2

/**
 * @brief Test that crc8 works
 */
ZTEST(crc, test_crc_8)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = CRC8_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	crc_begin(dev, &ctx);

	k_sleep(K_MSEC(50));

	crc_update(dev, &ctx, data, sizeof(data));
	crc_finish(dev, &ctx);
	zassert_equal(crc_verify(&ctx, RESULT_CRC_8), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_16 0xD543

/**
 * @brief Test that crc16 works
 */
ZTEST(crc, test_crc_16)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = CRC16_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);

	zassert_equal(crc_verify(&ctx, RESULT_CRC_16), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_CCITT 0x445C

/**
 * @brief Test that crc_16_ccitt works
 */
ZTEST(crc, test_crc_16_ccitt)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = CRC16_CCITT_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);

	zassert_equal(crc_verify(&ctx, RESULT_CRC_CCITT), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_32_C 0xBB19ECB2

/**
 * @brief Test that crc_32_c works
 */
ZTEST(crc, test_crc_32_c)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {
		.type = CRC32_C,
		.polynomial = CRC32C_POLY,
		.seed = CRC32_C_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);

	zassert_equal(crc_verify(&ctx, RESULT_CRC_32_C), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_32_IEEE 0xCEA4A6C2

/**
 * @brief Test that crc_32_ieee works
 */
ZTEST(crc, test_crc_32_ieee)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};
	struct crc_ctx ctx = {
		.type = CRC32_IEEE,
		.polynomial = CRC32_IEEE_POLY,
		.seed = CRC32_IEEE_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, RESULT_CRC_32_IEEE), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_8_REMAIN_3 0xBB

/**
 * @brief Test that crc_8_remain_3 works
 */
ZTEST(crc, test_crc_8_remain_3)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[11] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D, 0x4D, 0x51};

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = CRC8_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, RESULT_CRC_8_REMAIN_3), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_16_REMAIN_1 0x2055

/**
 * @brief Test that crc_16_remain_1 works
 */
ZTEST(crc, test_crc_16_remain_1)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[9] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D};

	struct crc_ctx ctx = {
		.type = CRC16,
		.polynomial = CRC16_POLY,
		.seed = CRC16_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);

	zassert_equal(crc_verify(&ctx, RESULT_CRC_16_REMAIN_1), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_CCITT_REMAIN_2 0x24BD

/**
 * @brief Test that crc_16_ccitt works
 */
ZTEST(crc, test_crc_16_ccitt_remain_2)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[10] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0xFF, 0xA0};

	struct crc_ctx ctx = {
		.type = CRC16_CCITT,
		.polynomial = CRC16_CCITT_POLY,
		.seed = CRC16_CCITT_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);

	zassert_equal(crc_verify(&ctx, RESULT_CRC_CCITT_REMAIN_2), 0);
}

/* Define result of CRC computation */
#define RESULT_DISCONTINUOUS_BUFFER 0x75

/**
 * @brief Test CRC calculation with discontinuous buffers.
 */
ZTEST(crc, test_discontinuous_buf)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data1[5] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E};
	uint8_t data2[5] = {0x49, 0x00, 0xC4, 0x3B, 0x78};

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = CRC8_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT,
	};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data1, sizeof(data1)), 0);
	zassert_equal(crc_update(dev, &ctx, data2, sizeof(data2)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, RESULT_DISCONTINUOUS_BUFFER), 0);
}

/* Define result of CRC computation */
#define RESULT_CRC_8_REMAIN_3_THREADSAFE 0xBB

/**
 * @brief Test CRC function semaphore wait for thread safety
 *
 * Verifies that CRC operations are blocked until a semaphore is released. A new thread
 * acquires the semaphore, and the main thread's CRC operations wait until it is released.
 */
ZTEST(crc, test_crc_threadsafe)
{
	static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_crc));

	uint8_t data[11] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3D, 0x4D, 0x51};

	struct crc_ctx ctx = {
		.type = CRC8,
		.polynomial = CRC8_POLY,
		.seed = CRC8_INIT_VAL,
		.reversed = CRC_FLAG_REVERSE_OUTPUT | CRC_FLAG_REVERSE_INPUT,
	};

	/**
	 * Create new thread that will immediately take the semaphore
	 */
	k_thread_create(&wait_thread_data, wait_thread_stack_area,
			K_THREAD_STACK_SIZEOF(wait_thread_stack_area), wait_thread_entry, NULL,
			NULL, NULL, WAIT_THREAD_PRIO, 0, K_NO_WAIT);

	/**
	 * Sleep for 10 ms to ensure that new thread has taken lock
	 */
	k_sleep(K_MSEC(10));

	/**
	 * Attempt to take semaphore, this should wait for the new thread to give the semaphore
	 * before executing
	 */
	crc_begin(dev, &ctx);
	crc_update(dev, &ctx, data, sizeof(data));
	crc_finish(dev, &ctx);
	zassert_equal(crc_verify(&ctx, RESULT_CRC_8_REMAIN_3_THREADSAFE), 0);
}

ZTEST_SUITE(crc, NULL, NULL, NULL, NULL, NULL);

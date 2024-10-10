/*
 * Copyright (c) 2024 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/crc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define WAIT_THREAD_STACK_SIZE 1024
#define WAIT_THREAD_PRIO       -10

static void wait_thread_entry(void *a, void *b, void *c);

K_THREAD_STACK_DEFINE(wait_thread_stack_area, WAIT_THREAD_STACK_SIZE);
struct k_thread wait_thread_data;

/**
 * 1) Take the semaphore
 * 2) Sleep for 50 ms (to allow ztest main thread to attempt to acquire semaphore)
 * 3) Give the semaphore
 */
static void wait_thread_entry(void *a, void *b, void *c)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {.type = CRC8_CCITT_HW,
			      .flags = 0,
			      .polynomial = CRC8_CCITT_POLY,
			      .initial_value = CRC8_CCITT_INIT_VAL};

	crc_begin(dev, &ctx);

	k_sleep(K_MSEC(50));

	crc_update(dev, &ctx, data, sizeof(data));
	crc_finish(dev, &ctx);
	zassert_equal(crc_verify(&ctx, 0x4D), 0);
}

/**
 * @brief Test that crc_8_ccitt works
 */
ZTEST(crc, test_crc_8_ccitt)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {.type = CRC8_CCITT_HW,
			      .flags = 0,
			      .polynomial = CRC8_CCITT_POLY,
			      .initial_value = CRC8_CCITT_INIT_VAL};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, 0x4D), 0);
}

/**
 * @brief Test that crc_32_ieee works
 */
ZTEST(crc, test_crc_32_ieee_remain_0)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));
	uint32_t flags = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT;

	uint8_t data[8] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4};

	struct crc_ctx ctx = {.type = CRC32_IEEE_HW,
			      .flags = flags,
			      .polynomial = CRC32_IEEE_POLY,
			      .initial_value = CRC32_IEEE_INIT_VAL};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, 0xCEA4A6C2), 0);
}

/**
 * @brief Test that crc_32_ieee works with a single byte
 *		remaining after the main process loop
 */
ZTEST(crc, test_crc_32_ieee_remain_1)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));
	uint32_t flags = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT;

	uint8_t data[9] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3B};

	struct crc_ctx ctx = {.type = CRC32_IEEE_HW,
			      .flags = flags,
			      .polynomial = CRC32_IEEE_POLY,
			      .initial_value = CRC32_IEEE_INIT_VAL};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, 0x16AD0193), 0);
}

/**
 * @brief Test that crc_32_ieee works with two bytes
 *		remaining after the main process loop
 */
ZTEST(crc, test_crc_32_ieee_remain_2)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));
	uint32_t flags = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT;

	uint8_t data[10] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3B, 0x78};

	struct crc_ctx ctx = {.type = CRC32_IEEE_HW,
			      .flags = flags,
			      .polynomial = CRC32_IEEE_POLY,
			      .initial_value = CRC32_IEEE_INIT_VAL};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, 0xE5CC797C), 0);
}

/**
 * @brief Test that crc_32_ieee works with three bytes
 *		remaining after the main process loop
 */
ZTEST(crc, test_crc_32_ieee_remain_3)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));
	uint32_t flags = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT;

	uint8_t data[11] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3B, 0x78, 0xB6};

	struct crc_ctx ctx = {.type = CRC32_IEEE_HW,
			      .flags = flags,
			      .polynomial = CRC32_IEEE_POLY,
			      .initial_value = CRC32_IEEE_INIT_VAL};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data, sizeof(data)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, 0xA956085A), 0);
}

/**
 * @brief Test CRC calculation with discontinuous buffers.
 */
ZTEST(crc, test_discontinuous_buf)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));
	uint32_t flags = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT;
	uint8_t data1[5] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E};
	uint8_t data2[5] = {0x49, 0x00, 0xC4, 0x3B, 0x78};

	struct crc_ctx ctx = {.type = CRC32_IEEE_HW,
			      .flags = flags,
			      .polynomial = CRC32_IEEE_POLY,
			      .initial_value = CRC32_IEEE_INIT_VAL};

	zassert_equal(crc_begin(dev, &ctx), 0);
	zassert_equal(crc_update(dev, &ctx, data1, sizeof(data1)), 0);
	zassert_equal(crc_update(dev, &ctx, data2, sizeof(data2)), 0);
	zassert_equal(crc_finish(dev, &ctx), 0);
	zassert_equal(crc_verify(&ctx, 0xE5CC797C), 0);
}

/**
 * @brief Test CRC function semaphore wait for thread safety
 *
 * Verifies that CRC operations are blocked until a semaphore is released. A new thread
 * acquires the semaphore, and the main thread's CRC operations wait until it is released.
 */
ZTEST(crc, test_crc_threadsafe)
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(crc));

	uint8_t data[11] = {0x0A, 0x2B, 0x4C, 0x6D, 0x8E, 0x49, 0x00, 0xC4, 0x3B, 0x78, 0xB6};
	uint32_t flags = CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT;
	struct crc_ctx ctx = {.type = CRC32_IEEE_HW,
			      .flags = flags,
			      .polynomial = CRC32_IEEE_POLY,
			      .initial_value = CRC32_IEEE_INIT_VAL};

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
	zassert_equal(crc_verify(&ctx, 0xA956085A), 0);
}

ZTEST_SUITE(crc, NULL, NULL, NULL, NULL, NULL);

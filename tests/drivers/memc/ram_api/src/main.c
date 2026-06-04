/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/devicetree.h>

#define MEMC_DEV DEVICE_DT_GET(DT_ALIAS(memc0))
#define BUF_SIZE 4096U

/* Offset for non-zero test */
#define OFFSET_B 4096U

static uint8_t wr_buf[BUF_SIZE];
static uint8_t rd_buf[BUF_SIZE];

static void fill_pattern(uint8_t *buf, size_t len, uint8_t seed)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(i * 3u + seed);
	}
}

static void before_each(void *unused)
{
	const struct device *dev = MEMC_DEV;

	zassert_true(device_is_ready(dev), "MEMC device not ready");

	zassert_true(DEVICE_API_IS(memc, dev), "Device does not implement the MEMC read/write API");
}

ZTEST_SUITE(memc_ram_api, NULL, NULL, before_each, NULL, NULL);

ZTEST(memc_ram_api, test_rw_offset_zero)
{
	const struct device *dev = MEMC_DEV;

	fill_pattern(wr_buf, sizeof(wr_buf), 0x00u);

	zassert_ok(memc_write(dev, 0, wr_buf, sizeof(wr_buf)), "memc_write at offset 0 failed");

	memset(rd_buf, 0, sizeof(rd_buf));
	zassert_ok(memc_read(dev, 0, rd_buf, sizeof(rd_buf)), "memc_read at offset 0 failed");

	for (size_t i = 0; i < sizeof(wr_buf); i++) {
		zassert_equal(rd_buf[i], wr_buf[i], "@0+%zu: wrote 0x%02x read 0x%02x", i,
			      wr_buf[i], rd_buf[i]);
	}
}

ZTEST(memc_ram_api, test_rw_nonzero_offset)
{
	const struct device *dev = MEMC_DEV;

	fill_pattern(wr_buf, sizeof(wr_buf), 0xA5u);

	zassert_ok(memc_write(dev, OFFSET_B, wr_buf, sizeof(wr_buf)),
		   "memc_write at offset %u failed", (unsigned int)OFFSET_B);

	memset(rd_buf, 0, sizeof(rd_buf));
	zassert_ok(memc_read(dev, OFFSET_B, rd_buf, sizeof(rd_buf)),
		   "memc_read at offset %u failed", (unsigned int)OFFSET_B);

	for (size_t i = 0; i < sizeof(wr_buf); i++) {
		zassert_equal(rd_buf[i], wr_buf[i], "@%u+%zu: wrote 0x%02x read 0x%02x",
			      (unsigned int)OFFSET_B, i, wr_buf[i], rd_buf[i]);
	}
}

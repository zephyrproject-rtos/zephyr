/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/espi/mchp_xec_espi.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define SRAM_BAR0_SIZE 1024U
#define SRAM_BAR1_SIZE 4096U

static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));

static void check_sram_bar(uint8_t id, size_t exp_size)
{
	uint8_t *buf = NULL;
	size_t size = 0;
	int ret;

	ret = mchp_xec_espi_sram_bar_get(espi_dev, id, &buf, &size);
	zassert_equal(ret, 0, "SRAM BAR %u get failed (%d)", id, ret);
	zassert_not_null(buf, "SRAM BAR %u buffer is NULL", id);
	zassert_equal(size, exp_size, "SRAM BAR %u size %zu", id, size);
	zassert_equal((uintptr_t)buf % size, 0U, "SRAM BAR %u buffer %p not aligned", id,
		      (void *)buf);
}

ZTEST(espi_sram_bar, test_sram_bar_get)
{
	check_sram_bar(0U, SRAM_BAR0_SIZE);
	check_sram_bar(1U, SRAM_BAR1_SIZE);
}

ZTEST(espi_sram_bar, test_sram_bar_get_invalid_args)
{
	uint8_t *buf;
	size_t size;

	zassert_equal(mchp_xec_espi_sram_bar_get(NULL, 0U, &buf, &size), -EINVAL);
	zassert_equal(mchp_xec_espi_sram_bar_get(espi_dev, 0U, NULL, &size), -EINVAL);
	zassert_equal(mchp_xec_espi_sram_bar_get(espi_dev, 0U, &buf, NULL), -EINVAL);
}

ZTEST(espi_sram_bar, test_sram_bar_get_unknown_id)
{
	uint8_t *buf;
	size_t size;

	zassert_equal(mchp_xec_espi_sram_bar_get(espi_dev, 2U, &buf, &size), -ENODEV);
}

ZTEST_SUITE(espi_sram_bar, NULL, NULL, NULL, NULL, NULL);

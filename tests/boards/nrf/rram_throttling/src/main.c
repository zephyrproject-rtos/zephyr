/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define TEST_AREA storage_partition

/* TEST_AREA is only defined for configurations that rely on
 * fixed-partition nodes.
 */
#if defined(TEST_AREA)
#define TEST_AREA_OFFSET FIXED_PARTITION_OFFSET(TEST_AREA)
#define TEST_AREA_SIZE   FIXED_PARTITION_SIZE(TEST_AREA)
#define TEST_AREA_DEVICE FIXED_PARTITION_DEVICE(TEST_AREA)
#else
#error "Unsupported configuraiton"
#endif

#define BUF_SIZE        512
#define TEST_ITERATIONS 100
/* Expected delay in ms: In this expression CONFIG_NRF_RRAM_THROTTLING_DELAY
 * is expressed in microseconds and CONFIG_NRF_RRAM_THROTTLING_DATA_BLOCK is in
 * number of write lines of 16 bytes each.
 */
#define EXPECTED_DELAY                                                                             \
	((TEST_ITERATIONS * BUF_SIZE * CONFIG_NRF_RRAM_THROTTLING_DELAY) /                         \
	 (CONFIG_NRF_RRAM_THROTTLING_DATA_BLOCK * 16 * 1000))
#if !defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && !defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#error There is no flash device enabled or it is missing Kconfig options
#endif

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static uint8_t __aligned(4) buf[BUF_SIZE];

static void *rram_throttling_setup(void)
{
	zassert_true(device_is_ready(flash_dev));

	TC_PRINT("Test will run on device %s\n", flash_dev->name);
	TC_PRINT("TEST_AREA_OFFSET = 0x%lx\n", (unsigned long)TEST_AREA_OFFSET);
	TC_PRINT("TEST_AREA_SIZE   = 0x%lx\n", (unsigned long)TEST_AREA_SIZE);

	return NULL;
}

ZTEST(rram_throttling, test_flash_throttling)
{
	int rc;
	uint32_t start = TEST_AREA_OFFSET;

#if !defined(CONFIG_SOC_FLASH_NRF_THROTTLING)
	ztest_test_skip();
#endif

	int64_t ts = k_uptime_get();

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		rc = flash_write(flash_dev, start, buf, BUF_SIZE);
		zassert_equal(rc, 0, "Cannot write to flash");
	}
	/* Delay measured in milliseconds */
	int64_t delta = k_uptime_delta(&ts);

	zassert_true(delta > EXPECTED_DELAY, "Invalid delay, expected > %d, measured: %lld",
		     EXPECTED_DELAY, delta);
}

ZTEST_SUITE(rram_throttling, NULL, rram_throttling_setup, NULL, NULL, NULL);

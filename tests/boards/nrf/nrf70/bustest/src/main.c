/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing ztests for nrf70 buslib library
 */

#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/rpu_hw_if.h>
#include <zephyr/drivers/wifi/nrf_wifi/bus/qspi_if.h>

LOG_MODULE_REGISTER(nrf70_bustest, CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL);

#define DATARAM_ADDR 0x0C0000
static struct qspi_dev *dev;

static int wifi_on(void *state)
{
	int ret;

	ARG_UNUSED(state);

	dev = qspi_dev();

	ret = rpu_init();
	if (ret) {
		LOG_ERR("%s: RPU init failed with error %d", __func__, ret);
		return -1;
	}

	ret = dev->init(qspi_defconfig());
	if (ret) {
		LOG_ERR("%s: QSPI device init failed", __func__);
		return -1;
	}

	ret = rpu_enable();
	if (ret) {
		LOG_ERR("%s: RPU enable failed with error %d", __func__, ret);
		return -1;
	}
	k_sleep(K_MSEC(10));
	LOG_INF("Wi-Fi ON done");
	return 0;
}

static void wifi_off(void *state)
{
	ARG_UNUSED(state);

	int ret;

	ret = rpu_disable();
	if (ret) {
		LOG_ERR("%s: RPU disable failed with error %d", __func__, ret);
	}

	ret = dev->deinit();
	if (ret) {
		LOG_ERR("%s: QSPI device de-init failed", __func__);
	}
	k_sleep(K_MSEC(10));
	LOG_INF("Wi-Fi OFF done");
}


static int memtest(uint32_t addr, char *memblock_name)
{
	const uint32_t pattern = 0x12345678;
	uint32_t offset = 1;
	uint32_t *buff, *rxbuff;
	int i;

	int err_count;
	int32_t rem_words = CONFIG_NRF70BUS_MEMTEST_LENGTH;
	uint32_t test_chunk, chunk_no = 0;
	int ret = -1;

	buff = k_malloc(CONFIG_NRF70BUS_MEMTEST_LENGTH * 4);
	if (buff == NULL) {
		LOG_ERR("Failed to allocate memory for buff");
		return -1;
	}

	rxbuff = k_malloc(CONFIG_NRF70BUS_MEMTEST_LENGTH * 4);
	if (rxbuff == NULL) {
		LOG_ERR("Failed to allocate memory for rxbuff");
		k_free(buff);
		return -1;
	}

	while (rem_words > 0) {
		test_chunk = (rem_words < CONFIG_NRF70BUS_MEMTEST_LENGTH) ?
					  rem_words : CONFIG_NRF70BUS_MEMTEST_LENGTH;

		for (i = 0; i < test_chunk; i++) {
			buff[i] = pattern +
					  (i + chunk_no * CONFIG_NRF70BUS_MEMTEST_LENGTH) * offset;
		}

		addr = addr + chunk_no * CONFIG_NRF70BUS_MEMTEST_LENGTH;

		if (rpu_write(addr, buff, test_chunk * 4) ||
		    rpu_read(addr, rxbuff, test_chunk * 4)) {
			goto err;
		}

		err_count = 0;
		for (i = 0; i < test_chunk; i++) {
			if (buff[i] != rxbuff[i]) {
				err_count++;
				LOG_ERR("%s: failed (%d), Expected 0x%x, Read 0x%x",
						__func__, i, buff[i], rxbuff[i]);
				if (err_count > 4)
					goto err;
			}
		}
		if (err_count) {
			goto err;
		}
		rem_words -= CONFIG_NRF70BUS_MEMTEST_LENGTH;
		chunk_no++;
	}
	ret = 0;
err:
	k_free(rxbuff);
	k_free(buff);
	return ret;
}

static int test_sysbus(void)
{
	int val, i;
	/* List of some SYS bus addresses and default values to test bus read
	 *  integrity
	 */
	const uint32_t addr[] = {0x714, 0x71c, 0x720,
							0x728, 0x734, 0x738};
	const uint32_t val_arr[] = {
		0x000003f3, 0x0110f13f, 0x000003f3,
		0x0003073f, 0x0003073f, 0x03013f8f};

	for (i = 0; i < ARRAY_SIZE(addr); i++) {
		rpu_read(addr[i], &val, 4);
		if (val != val_arr[i]) {
			LOG_ERR("%s: SYSBUS R/W failed (%d) : read = 0x%x, expected = 0x%x",
						 __func__, i, val, val_arr[i]);
			return -1;
		}
	}
	return 0;
}

static int test_peripbus(void)
{
	uint32_t val;
	int i;
	/* Some Perip bus addresses that we can write/read to validate bus access*/
	const uint32_t addr[] = {0x62820, 0x62830, 0x62840, 0x62850, 0x62860, 0x62870};

	for (i = 0; i < ARRAY_SIZE(addr); i++) {
		val = 0xA5A5A5A5; /* Test pattern */
		rpu_write(addr[i], &val, 4);
		val = 0;
		rpu_read(addr[i], &val, 4);
		/* Perip bus is 24-bit and hence LS 8 bits read are invalid, so discard them
		 * in the check
		 */
		if (val >> 8 != 0xA5A5A5) {
			LOG_ERR("%s: PERIP BUS R/W failed (%d): read = 0x%x",
						__func__, i, val >> 8);
			return -1;
		}
	}
	return 0;
}

/* Register address macros for testing */
#define RDSR0_ADDR    0x05
#define RDSR1_ADDR    0x1F
#define RDSR2_ADDR    0x2F
#define WRSR2_ADDR    0x3F

static int test_rdsr0(void)
{
	int ret;
	uint8_t val;

	LOG_INF("Testing RDSR0");

	ret = rpu_read_reg(RDSR0_ADDR, &val);
	if (ret) {
		LOG_ERR("Failed to read RDSR0: %d", ret);
		return -1;
	}

	LOG_INF("RDSR0 value: 0x%x", val);

	if (val == 0x42) {
		return 0;
	}
	LOG_ERR("RDSR0 test failed: expected 0x42, got 0x%x", val);
	return -1;
}

/* Helper function to write and verify a test pattern */
static int test_rdsr2_pattern(uint8_t test_pattern, const char *pattern_name)
{
	int ret;
	uint8_t read_val;
	uint8_t masked_pattern = test_pattern & 0xFE;  /* Ensure bit 0 is 0 */

	LOG_INF("Testing RDSR2 pattern %s (0x%02x)", pattern_name, masked_pattern);

	ret = rpu_write_reg(WRSR2_ADDR, masked_pattern);
	if (ret) {
		LOG_ERR("Failed to write RDSR2 pattern %s", pattern_name);
		return -1;
	}

	ret = rpu_read_reg(RDSR2_ADDR, &read_val);
	if (ret) {
		LOG_ERR("Failed to read RDSR2 after writing pattern %s", pattern_name);
		return -1;
	}

	/* Compare only bits 7:1 */
	if ((read_val & 0xFE) != masked_pattern) {
		LOG_ERR("RDSR2 pattern %s test failed: wrote 0x%02x, read 0x%02x (bits 7:1)",
				pattern_name, masked_pattern, read_val & 0xFE);
		return -1;
	}

	LOG_INF("RDSR2 pattern %s test passed: wrote 0x%02x, read 0x%02x",
			pattern_name, masked_pattern, read_val);
	return 0;
}

/* Individual test for pattern 0xAA (10101010) */
static int test_rdsr2_pattern_0xAA(void)
{
	return test_rdsr2_pattern(0xAA, "0xAA (10101010)");
}

/* Individual test for pattern 0x54 (01010100) */
static int test_rdsr2_pattern_0x54(void)
{
	return test_rdsr2_pattern(0x54, "0x54 (01010100)");
}

/* Helper function to test walking bit patterns */
static int test_rdsr2_walking_pattern(bool walking_ones, const char *test_name)
{
	int ret, i;
	uint8_t read_val, test_val;

	LOG_INF("Starting %s test for RDSR2 bits 7:1", test_name);

	for (i = 1; i <= 7; i++) {
		if (walking_ones) {
			test_val = (1 << i);  /* Set walking bit */
		} else {
			test_val = (0xFE & ~(1 << i));  /* Clear walking bit */
		}

		ret = rpu_write_reg(WRSR2_ADDR, test_val & 0xFE);
		/* Ensure bit 0 is 0 when writing */
		if (ret) {
			LOG_ERR("Failed to write RDSR2 %s bit %d", test_name, i);
			return -1;
		}

		ret = rpu_read_reg(RDSR2_ADDR, &read_val);
		if (ret) {
			LOG_ERR("Failed to read RDSR2 after writing %s bit %d", test_name, i);
			return -1;
		}

		/* Compare only bits 7:1 */
		if ((read_val & 0xFE) != (test_val & 0xFE)) {
			LOG_ERR("RDSR2 %s bit %d test failed: wrote 0x%02x, read 0x%02x (bits 7:1)",
					test_name, i, test_val & 0xFE, read_val & 0xFE);
			return -1;
		}
		LOG_DBG("RDSR2 %s bit %d passed: wrote 0x%02x, read 0x%02x",
				test_name, i, test_val & 0xFE, read_val);
	}

	LOG_INF("%s test completed successfully", test_name);
	return 0;
}

/* Individual test for walking '1' pattern */
static int test_rdsr2_walking_ones(void)
{
	return test_rdsr2_walking_pattern(true, "walking '1'");
}

/* Individual test for walking '0' pattern */
static int test_rdsr2_walking_zeros(void)
{
	return test_rdsr2_walking_pattern(false, "walking '0'");
}

/* Test for RDSR1 register */
static int test_rdsr1(void)
{
	int ret;
	uint8_t val;

	LOG_INF("Testing RDSR1");

	ret = rpu_read_reg(RDSR1_ADDR, &val);
	if (ret) {
		LOG_ERR("Failed to read RDSR1: %d", ret);
		return -1;
	}

	LOG_INF("RDSR1 value: 0x%x", val);

	/* RDSR1 should have RPU_AWAKE_BIT set when RPU is awake */
	if (val & 0x02) {  /* RPU_AWAKE_BIT = BIT(1) */
		LOG_INF("RDSR1 test passed: RPU is awake");
		return 0;
	}
	LOG_ERR("RDSR1 test failed: RPU is not awake (0x%x)", val);
	return -1;
}

ZTEST_SUITE(bustest_suite, NULL, (void *)wifi_on, NULL, NULL, wifi_off);

ZTEST(bustest_suite, test_sysbus)
{
	zassert_equal(0, test_sysbus(), "SYSBUS read validation failed!!!");
}

ZTEST(bustest_suite, test_peripbus)
{
	zassert_equal(0, test_peripbus(), "PERIP BUS read/write validation failed!!!");
}

ZTEST(bustest_suite, test_dataram)
{
	zassert_equal(0, memtest(DATARAM_ADDR, "DATA RAM"), "DATA RAM memtest failed!!!");
}

ZTEST(bustest_suite, test_rdsr0)
{
	zassert_equal(0, test_rdsr0(), "RDSR0 test failed!!!");
}

ZTEST(bustest_suite, test_rdsr2_pattern_0xAA)
{
	zassert_equal(0, test_rdsr2_pattern_0xAA(), "RDSR2 pattern 0xAA test failed!!!");
}

ZTEST(bustest_suite, test_rdsr2_pattern_0x54)
{
	zassert_equal(0, test_rdsr2_pattern_0x54(), "RDSR2 pattern 0x54 test failed!!!");
}

ZTEST(bustest_suite, test_rdsr2_walking_ones)
{
	zassert_equal(0, test_rdsr2_walking_ones(), "RDSR2 walking '1' test failed!!!");
}

ZTEST(bustest_suite, test_rdsr2_walking_zeros)
{
	zassert_equal(0, test_rdsr2_walking_zeros(), "RDSR2 walking '0' test failed!!!");
}

ZTEST(bustest_suite, test_rdsr1)
{
	zassert_equal(0, test_rdsr1(), "RDSR1 test failed!!!");
}

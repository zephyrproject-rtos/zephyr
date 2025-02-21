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

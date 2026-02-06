/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/espi.h>

#include "espi_flash_caf_sample.h"

LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

static const struct device *const espi_dev = DEVICE_DT_GET(DT_NODELABEL(espi0));

uint8_t flash_write_buf[MAX_TEST_BUF_SIZE];
uint8_t flash_read_buf[MAX_TEST_BUF_SIZE];

int read_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t block_len)
{
	uint8_t i = 0;
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len / MAX_FLASH_REQUEST;
	int ret = 0;
	struct espi_flash_packet pckt;

	for (i = 0; i < transactions; i++) {
		pckt.buf = buf;
		pckt.flash_addr = flash_addr;
		pckt.len = MAX_FLASH_REQUEST;

		ret = espi_read_flash(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_read_flash failed: %d", ret);
			return ret;
		}

		buf += MAX_FLASH_REQUEST;
		flash_addr += MAX_FLASH_REQUEST;
	}

	LOG_INF("%d read flash transactions completed", transactions);
	return 0;
}

int write_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t block_len)
{
	uint8_t i = 0;
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len / MAX_FLASH_REQUEST;
	int ret = 0;
	struct espi_flash_packet pckt;

	/* Split operation in multiple MAX_FLASH_REQ transactions */
	for (i = 0; i < transactions; i++) {
		pckt.buf = buf;
		pckt.flash_addr = flash_addr;
		pckt.len = MAX_FLASH_REQUEST;

		ret = espi_write_flash(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_write_flash failed: %d", ret);
			return ret;
		}

		buf += MAX_FLASH_REQUEST;
		flash_addr += MAX_FLASH_REQUEST;
	}

	LOG_INF("%d write flash transactions completed", transactions);
	return 0;
}

int espi_flash_test(uint32_t start_flash_addr, uint8_t blocks)
{
	uint8_t i;
	uint8_t pattern;
	uint32_t flash_addr;
	int ret = 0;

	LOG_INF("Test eSPI write flash");
	flash_addr = start_flash_addr;
	pattern = 0x99;
	for (i = 0; i <= blocks; i++) {
		memset(flash_write_buf, pattern++, sizeof(flash_write_buf));
		ret = write_test_block(flash_write_buf, flash_addr, sizeof(flash_write_buf));
		if (ret) {
			LOG_ERR("Failed to write to eSPI");
			return ret;
		}

		flash_addr += sizeof(flash_write_buf);
	}

	LOG_INF("Test eSPI read flash");
	flash_addr = start_flash_addr;
	pattern = 0x99;
	for (i = 0; i <= blocks; i++) {
		/* Set expected content */
		memset(flash_write_buf, pattern, sizeof(flash_write_buf));
		/* Clear last read content */
		memset(flash_read_buf, 0, sizeof(flash_read_buf));
		ret = read_test_block(flash_read_buf, flash_addr, sizeof(flash_read_buf));
		if (ret) {
			LOG_ERR("Failed to read from eSPI");
			return ret;
		}

		/* Compare buffers  */
		int cmp = memcmp(flash_write_buf, flash_read_buf, sizeof(flash_write_buf));

		if (cmp != 0) {
			LOG_ERR("eSPI read mismmatch at %d expected %x", cmp, pattern);
		}

		flash_addr += sizeof(flash_read_buf);
		pattern++;
	}

	return 0;
}

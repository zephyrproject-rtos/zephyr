/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <flash.h>
#include <string.h>

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

static inline bool is_addr_valid(off_t addr, size_t len)
{
	if (addr + len > NRF_FICR->CODEPAGESIZE * NRF_FICR->CODESIZE ||
			addr < 0) {
		return false;
	}

	return true;
}

static void nvmc_wait_ready(void)
{
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		;
	}
}

static int flash_nrf5_read(struct device *dev, off_t addr,
			    void *data, size_t len)
{
	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (void *)addr, len);

	return 0;
}

static int flash_nrf5_write(struct device *dev, off_t addr,
			     const void *data, size_t len)
{
	uint32_t addr_word;
	uint32_t tmp_word;
	void *data_word;
	uint32_t remaining = len;
	uint32_t count = 0;

	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	/* Start with a word-aligned address and handle the offset */
	addr_word = addr & ~0x3;

	/* If not aligned, read first word, update and write it back */
	if (!is_aligned_32(addr)) {
		tmp_word = *(uint32_t *)(addr_word);
		count = sizeof(uint32_t) - (addr & 0x3);
		if (count > len) {
			count = len;
		}
		memcpy((uint8_t *)&tmp_word + (addr & 0x3), data, count);
		nvmc_wait_ready();
		*(uint32_t *)addr_word = tmp_word;
		addr_word = addr + count;
		remaining -= count;
	}

	/* Write all the 4-byte aligned data */
	data_word = (void *) data + count;
	while (remaining >= sizeof(uint32_t)) {
		nvmc_wait_ready();
		*(uint32_t *)addr_word = *(uint32_t *)data_word;
		addr_word += sizeof(uint32_t);
		data_word += sizeof(uint32_t);
		remaining -= sizeof(uint32_t);
	}

	/* Write remaining data */
	if (remaining) {
		tmp_word = *(uint32_t *)(addr_word);
		memcpy((uint8_t *)&tmp_word, data_word, remaining);
		nvmc_wait_ready();
		*(uint32_t *)addr_word = tmp_word;
	}

	nvmc_wait_ready();

	return 0;
}

static int flash_nrf5_erase(struct device *dev, off_t addr, size_t size)
{
	uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
	uint32_t n_pages = size / pg_size;

	/* Erase can only be done per page */
	if (((addr % pg_size) != 0) || ((size % pg_size) != 0)) {
		return -EINVAL;
	}

	if (!is_addr_valid(addr, size)) {
		return -EINVAL;
	}

	if (!n_pages) {
		return 0;
	}

	/* Erase uses a specific configuration register */
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
	nvmc_wait_ready();

	for (uint32_t i = 0; i < n_pages; i++) {
		NRF_NVMC->ERASEPAGE = (uint32_t)addr + (i * pg_size);
		nvmc_wait_ready();
	}

	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	nvmc_wait_ready();

	return 0;
}

static int flash_nrf5_write_protection(struct device *dev, bool enable)
{
	if (enable) {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	} else {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
	}
	nvmc_wait_ready();

	return 0;
}

static const struct flash_driver_api flash_nrf5_api = {
	.read = flash_nrf5_read,
	.write = flash_nrf5_write,
	.erase = flash_nrf5_erase,
	.write_protection = flash_nrf5_write_protection,
};

static int nrf5_flash_init(struct device *dev)
{
	dev->driver_api = &flash_nrf5_api;

	return 0;
}

DEVICE_INIT(nrf5_flash, CONFIG_SOC_FLASH_NRF5_DEV_NAME, nrf5_flash_init,
	     NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

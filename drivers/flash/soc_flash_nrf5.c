/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <nanokernel.h>
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

	memcpy(data, (void *)addr, len);

	return 0;
}

static int flash_nrf5_write(struct device *dev, off_t addr,
			     const void *data, size_t len)
{
	uint32_t addr_word;
	void *data_word;

	/* Write needs a full 32-bit word into a word-aligned address */
	if ((!is_aligned_32(len)) || (!is_aligned_32(addr))) {
		return -EINVAL;
	}

	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	/* Write all the 4-byte aligned data */
	for (uint32_t i = 0; i < len; i += sizeof(uint32_t)) {
		addr_word = addr + i;
		data_word = (void *)data + i;
		nvmc_wait_ready();
		*(uint32_t *)addr_word = *(uint32_t *)data_word;
	}
	nvmc_wait_ready();

	return 0;
}

static int flash_nrf5_erase(struct device *dev, off_t addr, size_t size)
{
	uint32_t pg_size = NRF_FICR->CODEPAGESIZE;
	uint32_t n_pages = size / pg_size;

	/* Erase can only be done per page */
	if (((addr % pg_size) != 0) || ((size % pg_size) != 0) ||
			(n_pages == 0)) {
		return -EINVAL;
	}

	if (!is_addr_valid(addr, size)) {
		return -EINVAL;
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

static struct flash_driver_api flash_nrf5_api = {
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
	     NULL, NULL, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

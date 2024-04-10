/*
 * Copyright (c) 2024 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_rram_controller
#define SOC_NV_FLASH_NODE DT_INST(1, soc_nv_flash)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_atm_rram, CONFIG_FLASH_LOG_LEVEL);

#include "at_wrpr.h"

static void rram_write_enable(off_t addr, size_t len)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(len);

	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION0 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION1 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION2 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION3 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION4 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION5 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION6 = 0;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION7 = 0;
}

static void rram_write_disable(off_t addr, size_t len)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(len);

	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION0 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION1 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION2 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION3 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION4 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION5 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION6 = 0xffffffff;
	CMSDK_WRPR0_NONSECURE->RRAM_WRITE_PROTECTION7 = 0xffffffff;
}

static int flash_atm_rram_read(struct device const *dev, off_t addr, void *data, size_t len)
{
	LOG_DBG("flash_atm_rram_read(0x%08lx, %zu)", (unsigned long)addr, len);

	if (!len) {
		return 0;
	}

	memcpy(data, (void const *)(DT_REG_ADDR(SOC_NV_FLASH_NODE) + addr), len);

	return 0;
}

static int flash_atm_rram_write(struct device const *dev, off_t addr, void const *data, size_t len)
{
	LOG_DBG("flash_atm_rram_write(0x%08lx, %zu)", (unsigned long)addr, len);

	if (!len) {
		return 0;
	}

	rram_write_enable(addr, len);

	memcpy((void *)(DT_REG_ADDR(SOC_NV_FLASH_NODE) + addr), data, len);

	rram_write_disable(addr, len);

	return 0;
}

static int flash_atm_rram_erase(struct device const *dev, off_t addr, size_t size)
{
	LOG_DBG("flash_atm_rram_erase(0x%08lx, %zu)", (unsigned long)addr, size);

	rram_write_enable(addr, size);

	memset((void *)(DT_REG_ADDR(SOC_NV_FLASH_NODE) + addr), 0xff, size);

	rram_write_disable(addr, size);

	return 0;
}

static struct flash_parameters const *flash_atm_rram_get_parameters(struct device const *dev)
{
	ARG_UNUSED(dev);

	static struct flash_parameters const flash_atm_rram_parameters = {
		.write_block_size = FLASH_WRITE_BLK_SZ,
		.erase_value = 0xff,
	};

	return &flash_atm_rram_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_atm_rram_pages_layout(struct device const *dev,
					struct flash_pages_layout const **layout,
					size_t *layout_size)
{
	static struct flash_pages_layout const flash_atm_rram_pages_layout = {
		.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / FLASH_ERASE_BLK_SZ,
		.pages_size = FLASH_ERASE_BLK_SZ,
	};

	*layout = &flash_atm_rram_pages_layout;
	*layout_size = 1;
}
#endif // CONFIG_FLASH_PAGE_LAYOUT

static struct flash_driver_api const flash_atm_rram_api = {
	.read = flash_atm_rram_read,
	.write = flash_atm_rram_write,
	.erase = flash_atm_rram_erase,
	.get_parameters = flash_atm_rram_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_atm_rram_pages_layout,
#endif
};

static int flash_atm_rram_init(struct device const *dev)
{
	LOG_DBG("flash_atm_rram base:0x%08lx", (unsigned long)DT_REG_ADDR(SOC_NV_FLASH_NODE));

	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_atm_rram_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_atm_rram_api);

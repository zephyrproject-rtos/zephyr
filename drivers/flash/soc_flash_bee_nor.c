/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <flash_nor_device.h>

#ifdef CONFIG_SOC_SERIES_RTL8752H
#include <platform_cfg.h>
#endif

#define DT_DRV_COMPAT realtek_bee_nor_flash_controller

#include "flash_priv.h"

#define SOC_NV_FLASH_NODE SOC_NV_FLASH_CHILD_NODE(0)

#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)

/* Erase block must be 4KB (4096) - 64KB support TBD */
BUILD_ASSERT(FLASH_ERASE_BLK_SZ == 4096, "FLASH_ERASE_BLK_SZ must be 4KB (64KB TBD)");
/* Write has no alignment restriction, set to 1 byte */
BUILD_ASSERT(FLASH_WRITE_BLK_SZ == 1, "FLASH_WRITE_BLK_SZ must be 1");

#define FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define FLASH_ADDR DT_REG_ADDR(SOC_NV_FLASH_NODE)

#ifdef CONFIG_SOC_SERIES_RTL8752H
#define GET_FLASH_BIT_MODE_STR(mode)                                                               \
	((mode) == FLASH_NOR_1_BIT_MODE   ? "FLASH_NOR_1_BIT_MODE"                                 \
	 : (mode) == FLASH_NOR_2_BIT_MODE ? "FLASH_NOR_2_BIT_MODE"                                 \
	 : (mode) == FLASH_NOR_4_BIT_MODE ? "FLASH_NOR_4_BIT_MODE"                                 \
					  : "Invalid mode")
#endif

LOG_MODULE_REGISTER(flash_bee_nor, CONFIG_FLASH_LOG_LEVEL);

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_pages_layout_bee_nor[] = {
	{.pages_size = FLASH_ERASE_BLK_SZ, .pages_count = FLASH_SIZE / FLASH_ERASE_BLK_SZ}};
static void flash_bee_nor_page_layout(const struct device *dev,
				      const struct flash_pages_layout **layout, size_t *layout_size)
{
	*layout = flash_pages_layout_bee_nor;

	/*
	 * For flash memories which have uniform page sizes, this routine
	 * returns an array of length 1, which specifies the page size and
	 * number of pages in the memory.
	 */
	*layout_size = 1;
}
#endif

static const struct flash_parameters flash_bee_nor_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

/**
 * @brief Check if offset and len are valid (protect against negative values, overflows, and
 * out-of-bounds)
 */
static int flash_bee_nor_check_bounds(off_t offset, size_t len)
{
	if (offset < 0 || offset >= FLASH_SIZE || (FLASH_SIZE - offset) < len) {
		LOG_DBG("offset(%ld) or len(%zu) out of bounds", offset, len);
		return -EINVAL;
	}

	return 0;
}

static int flash_bee_nor_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	if (len == 0U) {
		return 0;
	}

	if (data == NULL) {
		LOG_ERR("Read data buffer is NULL");
		return -EINVAL;
	}

	int rc = flash_bee_nor_check_bounds(offset, len);

	if (rc != 0) {
		return rc;
	}

	FLASH_NOR_RET_TYPE ret = flash_nor_read_locked(FLASH_ADDR + offset, (uint8_t *)data, len);

	if (ret != FLASH_NOR_RET_SUCCESS) {
		LOG_ERR("flash_nor_read_locked failed: %d", ret);
		return -EIO;
	}

	return 0;
}

static int flash_bee_nor_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	FLASH_NOR_RET_TYPE ret;

	if (len == 0U) {
		return 0;
	}

	if (data == NULL) {
		LOG_ERR("Write data buffer is NULL");
		return -EINVAL;
	}

	int rc = flash_bee_nor_check_bounds(offset, len);

	if (rc != 0) {
		return rc;
	}

	ret = flash_nor_write_locked(FLASH_ADDR + offset, (uint8_t *)data, len);
	if (ret != FLASH_NOR_RET_SUCCESS) {
		LOG_ERR("flash_nor_write_locked failed: %d", ret);
		return -EIO;
	}

	return 0;
}

static int flash_bee_nor_erase(const struct device *dev, off_t offset, size_t len)
{
	FLASH_NOR_RET_TYPE ret;

	if (len == 0U) {
		return 0;
	}

	if ((offset % FLASH_ERASE_BLK_SZ) != 0) {
		LOG_ERR("offset %ld: not aligned to erase block size %d", offset,
			FLASH_ERASE_BLK_SZ);
		return -EINVAL;
	}

	if ((len % FLASH_ERASE_BLK_SZ) != 0) {
		LOG_ERR("len %zu: not aligned to erase block size %d", len, FLASH_ERASE_BLK_SZ);
		return -EINVAL;
	}

	int rc = flash_bee_nor_check_bounds(offset, len);

	if (rc != 0) {
		return rc;
	}

	uint32_t addr = FLASH_ADDR + offset;

	for (size_t remaining = len; remaining > 0; remaining -= FLASH_ERASE_BLK_SZ) {
		ret = flash_nor_erase_locked(addr, FLASH_NOR_ERASE_SECTOR);
		if (ret != FLASH_NOR_RET_SUCCESS) {
			LOG_ERR("flash_nor_erase_locked failed: %d", ret);
			return -EIO;
		}
		addr += FLASH_ERASE_BLK_SZ;
	}

	return 0;
}

static const struct flash_parameters *flash_bee_nor_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_bee_nor_parameters;
}

static DEVICE_API(flash, flash_bee_nor_driver_api) = {
	.read = flash_bee_nor_read,
	.write = flash_bee_nor_write,
	.erase = flash_bee_nor_erase,
	.get_parameters = flash_bee_nor_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_bee_nor_page_layout,
#endif
};

static int flash_bee_nor_init(const struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_RTL8752H

	if (flash_nor_get_exist(FLASH_NOR_IDX_SPIC0) != FLASH_NOR_EXIST_NONE) {
		if (flash_nor_load_query_info(FLASH_NOR_IDX_SPIC0) == FLASH_NOR_RET_SUCCESS) {
			/* apply SW Block Protect */
			if (boot_cfg.flash_setting.bp_enable) {
				/**
				 * set flash default block protect level depend on
				 * different flash id and different flash layout
				 */
				boot_cfg.flash_setting.bp_lv = flash_nor_get_default_bp_lv();
			} else {
				boot_cfg.flash_setting.bp_lv = 0;
			}

			if (flash_nor_set_tb_bit(FLASH_NOR_IDX_SPIC0, 1) == FLASH_NOR_RET_SUCCESS &&
			    flash_nor_set_bp_lv(FLASH_NOR_IDX_SPIC0,
						boot_cfg.flash_setting.bp_lv) ==
				    FLASH_NOR_RET_SUCCESS) {
				LOG_INF("Flash BP Lv = %d", boot_cfg.flash_setting.bp_lv);
			} else {
				LOG_INF("Flash BP fail!");
			}
		}
		flash_nor_dump_flash_info();
	}

	/*
	 * Switch flash to 4-bit mode before kernel starts to avoid xip isr
	 * affecting the calibration flow.
	 */
	if (flash_nor_try_high_speed_mode(FLASH_NOR_IDX_SPIC0, CONFIG_SOC_FLASH_BEE_NOR_BIT_MODE) ==
	    FLASH_NOR_RET_SUCCESS) {
		LOG_INF("Flash change to %s",
			GET_FLASH_BIT_MODE_STR(CONFIG_SOC_FLASH_BEE_NOR_BIT_MODE));
	} else {
		LOG_WRN("Flash change to %s failed, using default mode",
			GET_FLASH_BIT_MODE_STR(CONFIG_SOC_FLASH_BEE_NOR_BIT_MODE));
	}
#elif defined(CONFIG_SOC_SERIES_RTL87X2G)
	if (flash_nor_get_exist_nsc(FLASH_NOR_IDX_SPIC0)) {
		flash_nor_dump_main_info();
		flash_nor_cmd_list_init_nsc();
		flash_nor_init_bp_lv_nsc();
	}
#endif
	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_bee_nor_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_bee_nor_driver_api);

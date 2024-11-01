/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_SOC_FLASH_RENESAS_RA_LP_H_
#define ZEPHYR_DRIVERS_FLASH_SOC_FLASH_RENESAS_RA_LP_H_

#include <zephyr/drivers/flash.h>
#include <zephyr/sys/atomic.h>
#include <instances/r_flash_lp.h>
#include <api/r_flash_api.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>

#define DT_DRV_COMPAT renesas_ra_flash_lp_controller

#define FLASH_LP_CF_START DT_REG_ADDR(DT_NODELABEL(flash0))
#define FLASH_LP_DF_START DT_REG_ADDR(DT_NODELABEL(flash1))
#define FLASH_LP_CF_SIZE  DT_REG_SIZE(DT_NODELABEL(flash0))
#define FLASH_LP_DF_SIZE  DT_REG_SIZE(DT_NODELABEL(flash1))

#define FLASH_LP_VERSION DT_INST_PROP(0, flash_hardware_version)

#if (FLASH_LP_VERSION == 3)

#define FLASH_LP_CF_BLOCK_SIZE   DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 0, pages_size)
#define FLASH_LP_CF_BLOCKS_COUNT DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 0, pages_count)

BUILD_ASSERT(FLASH_LP_CF_BLOCK_SIZE == BSP_FEATURE_FLASH_LP_CF_BLOCK_SIZE,
	     "flash0 pages_size expected to be equal with block size");
#else
#error This Flash-LP version is not supported
#endif

#define FLASH_LP_DF_BLOCK_SIZE   DT_PROP(DT_NODELABEL(flash1), erase_block_size)
#define FLASH_LP_DF_BLOCKS_COUNT (FLASH_LP_DF_SIZE / FLASH_LP_DF_BLOCK_SIZE)

BUILD_ASSERT(FLASH_LP_DF_BLOCK_SIZE == BSP_FEATURE_FLASH_LP_DF_BLOCK_SIZE,
	     "flash1 erase-block-size expected to be equal with block size");

enum flash_region {
	CODE_FLASH,
	DATA_FLASH,
};

#if defined(CONFIG_FLASH_RENESAS_RA_LP_BGO)
#define FLASH_FLAG_ERASE_COMPLETE BIT(0)
#define FLASH_FLAG_WRITE_COMPLETE BIT(1)
#define FLASH_FLAG_GET_ERROR      BIT(2)
#endif /* CONFIG_FLASH_RENESAS_RA_LP_BGO */

struct flash_lp_ra_controller {
	struct st_flash_lp_instance_ctrl flash_ctrl;
	struct k_sem ctrl_sem;
	struct st_flash_cfg fsp_config;
	atomic_t flags;
};

struct flash_lp_ra_data {
	struct flash_lp_ra_controller *controller;
	enum flash_region FlashRegion;
	uint32_t area_address;
	uint32_t area_size;
};

struct flash_lp_ra_config {
	struct flash_parameters flash_ra_parameters;
};

#endif /* ZEPHYR_DRIVERS_FLASH_SOC_FLASH_RENESAS_RA_LP_H_ */

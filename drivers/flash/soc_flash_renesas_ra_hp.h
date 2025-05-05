/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_SOC_FLASH_RENESAS_RA_HP_H_
#define ZEPHYR_DRIVERS_FLASH_SOC_FLASH_RENESAS_RA_HP_H_

#include <zephyr/drivers/flash.h>
#include <zephyr/sys/atomic.h>
#include <instances/r_flash_hp.h>
#include <api/r_flash_api.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>

#define FLASH_HP_CF_START_ADDRESS DT_REG_ADDR(DT_NODELABEL(flash0))
#define FLASH_HP_DF_START_ADDRESS DT_REG_ADDR(DT_NODELABEL(flash1))

#define FLASH_HP_CF_SIZE DT_REG_SIZE(DT_NODELABEL(flash0))
#define FLASH_HP_DF_SIZE DT_REG_SIZE(DT_NODELABEL(flash1))

#define FLASH_HP_VERSION DT_PROP(DT_PARENT(DT_NODELABEL(flash0)), flash_hardware_version)

#if (FLASH_HP_VERSION == 40)

#define FLASH_HP_CF_REGION0_BLOCKS_COUNT                                                           \
	DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 0, pages_count)
#define FLASH_HP_CF_REGION0_BLOCK_SIZE                                                             \
	DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 0, pages_size)
#define FLASH_HP_CF_REGION0_SIZE (FLASH_HP_CF_REGION0_BLOCKS_COUNT * FLASH_HP_CF_REGION0_BLOCK_SIZE)

BUILD_ASSERT(FLASH_HP_CF_REGION0_BLOCK_SIZE == BSP_FEATURE_FLASH_HP_CF_REGION0_BLOCK_SIZE,
	     "erase-block-size expected to be equal with block size");

#define FLASH_HP_CF_REGION1_BLOCKS_COUNT                                                           \
	DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 1, pages_count)
#define FLASH_HP_CF_REGION1_BLOCK_SIZE                                                             \
	DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 1, pages_size)

BUILD_ASSERT(FLASH_HP_CF_REGION1_BLOCK_SIZE == BSP_FEATURE_FLASH_HP_CF_REGION1_BLOCK_SIZE,
	     "erase-block-size expected to be equal with block size");

#define FLASH_HP_CF_LAYOUT_SIZE (2UL)

#define FLASH_HP_CF_END_BLOCK (FLASH_HP_CF_REGION0_BLOCKS_COUNT + FLASH_HP_CF_REGION1_BLOCKS_COUNT)

#elif (FLASH_HP_VERSION == 4)

#define FLASH_HP_CF_REGION0_BLOCKS_COUNT                                                           \
	DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 0, pages_count)
#define FLASH_HP_CF_REGION0_BLOCK_SIZE                                                             \
	DT_PHA_BY_IDX(DT_NODELABEL(flash0), erase_blocks, 0, pages_size)
#define FLASH_HP_CF_REGION0_SIZE (FLASH_HP_CF_REGION0_BLOCKS_COUNT * FLASH_HP_CF_REGION0_BLOCK_SIZE)

BUILD_ASSERT(FLASH_HP_CF_REGION0_BLOCK_SIZE == BSP_FEATURE_FLASH_HP_CF_REGION0_BLOCK_SIZE,
	     "erase-block-size expected to be equal with block size");

#define FLASH_HP_CF_LAYOUT_SIZE (1UL)

#define FLASH_HP_CF_END_BLOCK FLASH_HP_CF_REGION0_BLOCKS_COUNT

#endif

#define FLASH_HP_DF_LAYOUT_SIZE  (1UL)
#define FLASH_HP_DF_BLOCK_SIZE   DT_PROP(DT_NODELABEL(flash1), erase_block_size)
#define FLASH_HP_DF_BLOCKS_COUNT (FLASH_HP_DF_SIZE / FLASH_HP_DF_BLOCK_SIZE)
#define FLASH_HP_DF_END_BLOCK    FLASH_HP_DF_BLOCKS_COUNT

BUILD_ASSERT(FLASH_HP_DF_BLOCK_SIZE == BSP_FEATURE_FLASH_HP_DF_BLOCK_SIZE,
	     "erase-block-size expected to be equal with block size");

#if defined(CONFIG_FLASH_EX_OP_ENABLED)
#define FLASH_HP_FCU_CONFIG_SET_BPS     (0x1300A1C0U)
#define FLASH_HP_FCU_CONFIG_SET_BPS_SEC (0x0300A240U)
#define FLASH_HP_FCU_CONFIG_SET_BPS_SEL (0x0300A2C0U)

#define FLASH_HP_FCU_CONFIG_SET_PBPS     (0x1300A1E0U)
#define FLASH_HP_FCU_CONFIG_SET_PBPS_SEC (0x0300A260U)
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

/* Zero based offset into g_configuration_area_data[] for BPS */
#define FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET (0U)

enum flash_region {
	CODE_FLASH,
	DATA_FLASH,
};

#if defined(CONFIG_FLASH_RENESAS_RA_HP_BGO)
#define FLASH_FLAG_ERASE_COMPLETE BIT(0)
#define FLASH_FLAG_WRITE_COMPLETE BIT(1)
#define FLASH_FLAG_GET_ERROR      BIT(2)

#if defined(CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING)
#define FLASH_FLAG_BLANK          BIT(3)
#define FLASH_FLAG_NOT_BLANK      BIT(4)
#endif /* CONFIG_FLASH_RENESAS_RA_HP_CHECK_BEFORE_READING */

#endif /* CONFIG_FLASH_RENESAS_RA_HP_BGO */

struct flash_hp_ra_controller {
	struct st_flash_hp_instance_ctrl flash_ctrl;
	struct k_sem ctrl_sem;
	struct st_flash_cfg fsp_config;
	atomic_t flags;
};

struct flash_hp_ra_data {
	struct flash_hp_ra_controller *controller;
	enum flash_region FlashRegion;
	uint32_t area_address;
	uint32_t area_size;
};

struct flash_hp_ra_config {
	struct flash_parameters flash_ra_parameters;
};

#if defined(CONFIG_FLASH_RENESAS_RA_HP_WRITE_PROTECT)
int flash_ra_ex_op_write_protect(const struct device *dev, const uintptr_t in, void *out);
#endif /* CONFIG_FLASH_RENESAS_RA_HP_WRITE_PROTECT */

#endif /* ZEPHYR_DRIVERS_FLASH_SOC_FLASH_RENESAS_RA_HP_H_ */

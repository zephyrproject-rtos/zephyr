/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_RA_HP_H_
#define ZEPHYR_DRIVERS_FLASH_RA_HP_H_

#include <zephyr/drivers/flash.h>
#include <instances/r_flash_hp.h>
#include <api/r_flash_api.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>

#define CHECK_EQ(val1, val2)                 ((val1) == (val2) ? 1 : 0)
#define GET_SIZE(COND, value, default_value) ((COND) ? (value) : (default_value))

#define FLASH_HP_BANK2_OFFSET                                                                      \
	(BSP_FEATURE_FLASH_HP_CF_DUAL_BANK_START - BSP_FEATURE_FLASH_CODE_FLASH_START)

#define FLASH_HP_CF_BLOCK_8KB_SIZE  BSP_FEATURE_FLASH_HP_CF_REGION0_BLOCK_SIZE
#define FLASH_HP_CF_BLOCK_32KB_SIZE BSP_FEATURE_FLASH_HP_CF_REGION1_BLOCK_SIZE
#define FLASH_HP_DF_BLOCK_SIZE      BSP_FEATURE_FLASH_HP_DF_BLOCK_SIZE
#define FLASH_HP_DF_START           BSP_FEATURE_FLASH_DATA_FLASH_START

#define FLASH_HP_CF_BLOCK_8KB_LOW_START  (0)
#define FLASH_HP_CF_BLOCK_8KB_LOW_END    (7)
#define FLASH_HP_CF_BLOCK_8KB_HIGH_START (70)
#define FLASH_HP_CF_BLOCK_8KB_HIGH_END   (77)

#define FLASH_HP_CF_BLOCK_32KB_LINEAR_START (8)
#define FLASH_HP_CF_BLOCK_32KB_LINEAR_END   (DT_PROP(DT_NODELABEL(flash), block_32kb_linear_end))

#define FLASH_HP_DF_BLOCK_END (DT_REG_SIZE(DT_NODELABEL(flash1)) / FLASH_HP_DF_BLOCK_SIZE)

#if defined(CONFIG_DUAL_BANK_MODE)
#define FLASH_HP_CF_NUM_BLOCK_RESERVED         (DT_PROP(DT_NODELABEL(flash), reserved_area_num))
#define FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_START  (8)
#define FLASH_HP_CF_BLOCK_32KB_DUAL_HIGH_START (78)

#define FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END (DT_PROP(DT_NODELABEL(flash), block_32kb_dual_low_end))
#define FLASH_HP_CF_BLOCK_32KB_DUAL_HIGH_END                                                       \
	(DT_PROP(DT_NODELABEL(flash), block_32kb_dual_high_end))

#define FLASH_HP_CF_DUAL_HIGH_START_ADDRESS BSP_FEATURE_FLASH_HP_CF_DUAL_BANK_START

#define FLASH_HP_CF_DUAL_LOW_END_ADDRESS                                                           \
	(DT_REG_SIZE(DT_NODELABEL(flash0)) -                                                       \
	 ((FLASH_HP_CF_BLOCK_32KB_LINEAR_END - FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END) *              \
	  FLASH_HP_CF_BLOCK_32KB_SIZE))

#define FLASH_HP_CF_DUAL_HIGH_END_ADDRESS                                                          \
	(DT_REG_SIZE(DT_NODELABEL(flash0)) +                                                       \
	 (FLASH_HP_CF_NUM_BLOCK_RESERVED * FLASH_HP_CF_BLOCK_32KB_SIZE))
#endif

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

typedef void (*irq_config_func_t)(const struct device *dev);

struct flash_hp_ra_controller {
	struct st_flash_hp_instance_ctrl flash_ctrl;
	struct k_sem ctrl_sem;
	struct st_flash_cfg fsp_config;
};

struct flash_hp_ra_controller_config {
	irq_config_func_t irq_config;
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

struct event_flash {
	volatile bool erase_complete;
	volatile bool write_complete;
};

#if defined(CONFIG_FLASH_RA_WRITE_PROTECT)
int flash_ra_ex_op_write_protect(const struct device *dev, const uintptr_t in, void *out);
#endif /*CONFIG_FLASH_RA_WRITE_PROTECT*/

#endif /* ZEPHYR_DRIVERS_FLASH_RA_HP_H_ */

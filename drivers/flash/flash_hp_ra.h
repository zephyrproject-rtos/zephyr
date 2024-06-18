/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_RA_HP_H_
#define ZEPHYR_DRIVERS_FLASH_RA_HP_H_

#include <zephyr/drivers/flash.h>
#include <instances/r_flash_hp.h>
#include <api/r_flash_api.h>
#include <zephyr/drivers/flash/ra_flash_api_extensions.h>

#if DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#define FLASH_RA_WRITE_BLOCK_SIZE DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#else
#error Flash write block size not available
/* Flash Write block size is extracted from device tree */
/* as flash node property 'write-block-size' */
#endif

#if defined(CONFIG_DUAL_BANK_MODE)
#define SOC_NV_FLASH_SIZE 0x2F8000
#else
#define SOC_NV_FLASH_SIZE DT_REG_SIZE(DT_INST(0, soc_nv_flash))
#endif

#define VECTOR_NUMBER_FCU_FRDYI ((IRQn_Type) 0) /* FCU FRDYI (Flash ready interrupt) */
#define VECTOR_NUMBER_FCU_FIFERR ((IRQn_Type) 1) /* FCU FIFERR (Flash access error interrupt) */

struct flash_hp_ra_data {
	struct st_flash_hp_instance_ctrl flash_ctrl;
	struct st_flash_cfg fsp_config;
};

struct flash_hp_ra_config {
	struct flash_parameters flash_ra_parameters;
};

#define FLASH_HP_BANK2_OFFSET                                                                      \
	(BSP_FEATURE_FLASH_HP_CF_DUAL_BANK_START - BSP_FEATURE_FLASH_CODE_FLASH_START)

#define FLASH_HP_CF_BLOCK_8KB_SIZE  BSP_FEATURE_FLASH_HP_CF_REGION0_BLOCK_SIZE
#define FLASH_HP_CF_BLOCK_32KB_SIZE BSP_FEATURE_FLASH_HP_CF_REGION1_BLOCK_SIZE

#define FLASH_HP_CF_BLOCK_8KB_LOW_START (0)
#define FLASH_HP_CF_BLOCK_8KB_LOW_END   (7)
#define FLASH_HP_CF_BLOCK_8KB_HIGH_START (70)
#define FLASH_HP_CF_BLOCK_8KB_HIGH_END   (77)

#define FLASH_HP_CF_BLOCK_32KB_LINEAR_START    (8)

#define FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_START  (8)
#define FLASH_HP_CF_BLOCK_32KB_DUAL_HIGH_START (78)

#if defined(CONFIG_SOC_R7FA8M1AHECBD) || defined(CONFIG_SOC_R7FA8D1BHECBD)                         \
|| defined(CONFIG_SOC_R7FA8T1AHECBD)
#define FLASH_RESERVED_AREA_NUM				   (33)
#define FLASH_HP_CF_BLOCK_32KB_LINEAR_END      (68)
#define FLASH_HP_CF_BLOCK_32KB_DUAL_LOW_END    (36)
#define FLASH_HP_CF_BLOCK_32KB_DUAL_HIGH_END   (106)
#endif

#if defined(CONFIG_FLASH_EX_OP_ENABLED)
#define FLASH_HP_FCU_CONFIG_SET_BPS     (0x1300A1C0U)
#define FLASH_HP_FCU_CONFIG_SET_BPS_SEC (0x0300A240U)
#define FLASH_HP_FCU_CONFIG_SET_BPS_SEL (0x0300A2C0U)

#define FLASH_HP_FCU_CONFIG_SET_PBPS     (0x1300A1E0U)
#define FLASH_HP_FCU_CONFIG_SET_PBPS_SEC (0x0300A260U)

/* Zero based offset into g_configuration_area_data[] for BPS */
#define FLASH_HP_FCU_CONFIG_SET_BPS_OFFSET (0U)

#if defined(CONFIG_FLASH_RA_WRITE_PROTECT)
int flash_ra_block_protect_set(const struct device *dev,
			       const struct flash_ra_ex_write_protect_in_t *request);
int flash_ra_block_protect_get(const struct device *dev,
			       struct flash_ra_ex_write_protect_out_t *response);
int flash_ra_ex_op_write_protect(const struct device *dev, const uintptr_t in, void *out);
#endif /*CONFIG_FLASH_RA_WRITE_PROTECT*/

fsp_err_t R_FLASH_HP_BlockProtectSet(flash_ctrl_t *const p_api_ctrl, uint8_t *bps_val_ns,
				     uint8_t *bps_val_sec, uint8_t *bps_val_sel,
				     uint8_t *pbps_val_ns, uint8_t *pbps_val_sec, uint32_t size);
fsp_err_t R_FLASH_HP_BlockProtectGet(flash_ctrl_t *const p_api_ctrl, uint32_t *bps_val_ns,
				     uint32_t *bps_val_sec, uint8_t *bps_val_sel,
				     uint8_t *pbps_val_ns, uint8_t *pbps_val_sec, uint32_t *size);
#endif

#endif /* ZEPHYR_DRIVERS_FLASH_RA_HP_H_ */

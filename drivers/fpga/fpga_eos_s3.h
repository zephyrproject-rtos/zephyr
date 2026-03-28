/*
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FPGA_EOS_S3_H_
#define ZEPHYR_DRIVERS_FPGA_EOS_S3_H_

#include <eoss3_dev.h>

struct PIF_struct {
	/* Fabric Configuration Control Register, offset: 0x000 */
	__IO uint32_t CFG_CTL;
	/* Maximum Bit Length Count, offset: 0x004 */
	__IO uint32_t MAX_BL_CNT;
	/* Maximum Word Length Count, offset: 0x008 */
	__IO uint32_t MAX_WL_CNT;
	uint32_t reserved[1020];
	/* Configuration Data, offset: 0xFFC */
	__IO uint32_t CFG_DATA;
};

#define PIF ((struct PIF_struct *)PIF_CTRL_BASE)

#define FB_CFG_ENABLE					((uint32_t)(0x00000200))
#define FB_CFG_DISABLE					((uint32_t)(0x00000000))

#define CFG_CTL_APB_CFG_WR				((uint32_t)(0x00008000))
#define CFG_CTL_APB_CFG_RD				((uint32_t)(0x00004000))
#define CFG_CTL_APB_WL_DIN				((uint32_t)(0x00003C00))
#define CFG_CTL_APB_PARTIAL_LOAD			((uint32_t)(0x00000200))
#define CFG_CTL_APB_BL_SEL				((uint32_t)(0x00000100))
#define CFG_CTL_APB_BLM_SEL				((uint32_t)(0x00000080))
#define CFG_CTL_APB_BR_SEL				((uint32_t)(0x00000040))
#define CFG_CTL_APB_BRM_SEL				((uint32_t)(0x00000020))
#define CFG_CTL_APB_TL_SEL				((uint32_t)(0x00000010))
#define CFG_CTL_APB_TLM_SEL				((uint32_t)(0x00000008))
#define CFG_CTL_APB_TR_SEL				((uint32_t)(0x00000004))
#define CFG_CTL_APB_TRM_SEL				((uint32_t)(0x00000002))
#define CFG_CTL_APB_SEL_CFG				((uint32_t)(0x00000001))

#define FB_ISOLATION_ENABLE				((uint32_t)(0x00000001))
#define FB_ISOLATION_DISABLE				((uint32_t)(0x00000000))

#define PMU_FFE_FB_PF_SW_PD_FB_PD			((uint32_t)(0x00000002))
#define PMU_FB_PWR_MODE_CFG_FB_SD			((uint32_t)(0x00000002))
#define PMU_FB_PWR_MODE_CFG_FB_DP			((uint32_t)(0x00000001))

#define FPGA_INFO                                                                                  \
	"eos_s3 eFPGA features:\n"                                                                 \
	"891 Logic Cells\n"                                                                        \
	"8 FIFO Controllers\n"                                                                     \
	"32 Configurable Interfaces\n"                                                             \
	"2x32x32(or 4x16x16) Multiplier\n"                                                         \
	"64Kbit SRAM\n"

#define PAD_ENABLE                                                                                 \
	(PAD_E_4MA | PAD_P_PULLDOWN | PAD_OEN_NORMAL | PAD_SMT_DISABLE | PAD_REN_DISABLE |         \
	 PAD_SR_SLOW | PAD_CTRL_SEL_AO_REG)

#define PAD_DISABLE                                                                                \
	(PAD_SMT_DISABLE | PAD_REN_DISABLE | PAD_SR_SLOW | PAD_E_4MA | PAD_P_PULLDOWN |            \
	 PAD_OEN_NORMAL | PAD_CTRL_SEL_AO_REG)

#define CFG_CTL_LOAD_ENABLE                                                                        \
	(CFG_CTL_APB_CFG_WR | CFG_CTL_APB_WL_DIN | CFG_CTL_APB_BL_SEL | CFG_CTL_APB_BLM_SEL |      \
	 CFG_CTL_APB_BR_SEL | CFG_CTL_APB_BRM_SEL | CFG_CTL_APB_TL_SEL | CFG_CTL_APB_TLM_SEL |     \
	 CFG_CTL_APB_TR_SEL | CFG_CTL_APB_TRM_SEL | CFG_CTL_APB_SEL_CFG)

#define CFG_CTL_LOAD_DISABLE 0

#endif /* ZEPHYR_DRIVERS_FPGA_EOS_S3_H_ */

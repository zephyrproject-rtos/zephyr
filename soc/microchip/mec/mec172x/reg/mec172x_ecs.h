/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _MEC172X_ECS_H
#define _MEC172X_ECS_H

/* AHB Error Address, write any value to clear */
#define MCHP_ECS_AHB_ERR_ADDR_OFS	0x04u

/* AHB Error Control */
#define MCHP_ECS_AHB_ERR_CTRL_OFS	0x14u
#define MCHP_ECS_AHB_ERR_CTRL_DIS_POS	0
#define MCHP_ECS_AHB_ERR_CTRL_DIS	BIT(MCHP_ECS_AHB_ERR_CTRL_DIS_POS)

/* Interrupt Control */
#define MCHP_ECS_ICTRL_OFS		0x18u
#define MCHP_ECS_ICTRL_DIRECT_POS	0
#define MCHP_ECS_ICTRL_DIRECT_EN	BIT(MCHP_ECS_ICTRL_DIRECT_POS)

/* ETM Control Register */
#define MCHP_ECS_ETM_CTRL_OFS		0x1cu
#define MCHP_ECS_ETM_CTRL_EN_POS	0
#define MCHP_ECS_ETM_CTRL_EN		BIT(MCHP_ECS_ETM_CTRL_EN_POS)

/* Debug Control Register */
#define MCHP_ECS_DCTRL_OFS		0x20u
#define MCHP_ECS_DCTRL_MASK		0x1fu
#define MCHP_ECS_DCTRL_DBG_EN_POS	0u
#define MCHP_ECS_DCTRL_DBG_EN		BIT(MCHP_ECS_DCTRL_DBG_EN_POS)
#define MCHP_ECS_DCTRL_MODE_POS		1u
#define MCHP_ECS_DCTRL_MODE_MASK0	0x03u
#define MCHP_ECS_DCTRL_MODE_MASK \
	SHLU32(MCHP_ECS_DCTRL_DBG_MODE_MASK0, MCHP_ECS_DCTRL_DBG_MODE_POS)

#define MCHP_ECS_DCTRL_DBG_MODE_POS	1u
#define MCHP_ECS_DCTRL_MODE_JTAG	0x00u
#define MCHP_ECS_DCTRL_MODE_SWD		SHLU32(0x02u, 1u)
#define MCHP_ECS_DCTRL_MODE_SWD_SWV	SHLU32(0x01u, 1u)
#define MCHP_ECS_DCTRL_PUEN_POS		3u
#define MCHP_ECS_DCTRL_PUEN		BIT(MCHP_ECS_DCTRL_PUEN_POS)
#define MCHP_ECS_DCTRL_BSCAN_POS	4u
#define MCHP_ECS_DCTRL_BSCAN_EN		BIT(MCHP_ECS_DCTRL_BSCAN_POS)

/* WDT Event Count */
#define MCHP_ECS_WDT_EVC_OFS		0x28u
#define MCHP_ECS_WDT_EVC_MASK		0x0fu

/* PECI Disable */
#define MCHP_ECS_PECI_DIS_OFS		0x40u
#define MCHP_ECS_PECI_DIS_MASK		0x01u
#define MCHP_ECS_PECI_DIS_POS		0
#define MCHP_ECS_PECI_DISABLE		BIT(0)

/* VCI FW Override */
#define MCHP_ECS_VCI_FW_OVR_OFS		0x50u
#define MCHP_ECS_VCI_FW_OVR_MASK	0x01u
#define MCHP_ECS_VCI_FW_OVR_SSHD_POS	0
#define MCHP_ECS_VCI_FW_OVR_SSHD	BIT(0)

/* Boot-ROM Status */
#define MCHP_ECS_BROM_STS_OFS		0x54u
#define MCHP_ECS_BROM_STS_MASK		0x03u
#define MCHP_ECS_BROM_STS_VTR_POS	0
#define MCHP_ECS_BROM_STS_WDT_POS	1
#define MCHP_ECS_BROM_STS_VTR		BIT(0)
#define MCHP_ECS_BROM_STS_WDT		BIT(1)

/* JTAG Controller Config */
#define MCHP_ECS_JTCC_OFS		0x70u
#define MCHP_ECS_JTCC_MASK		0x0fu
#define MCHP_ECS_JTCC_CLK_POS		0
#define MCHP_ECS_JTCC_CLK_DFLT		3u
#define MCHP_ECS_JTCC_CLK_24M		1u
#define MCHP_ECS_JTCC_CLK_12M		2u
#define MCHP_ECS_JTCC_CLK_6M		3u
#define MCHP_ECS_JTCC_CLK_3M		4u
#define MCHP_ECS_JTCC_CLK_1500K		5u
#define MCHP_ECS_JTCC_CLK_750K		6u
#define MCHP_ECS_JTCC_CLK_375K		7u
#define MCHP_ECS_JTCC_MS_POS		3
#define MCHP_ECS_JTCC_M			BIT(3)

/* JTAG Controller Status */
#define MCHP_ECS_JTST_OFS		0x74u
#define MCHP_ECS_JTST_MASK		0x01u
#define MCHP_ECS_JTST_DONE		BIT(0)

#define MCHP_ECS_JT_TDO_OFS		0x78u
#define MCHP_ECS_JT_TDI_OFS		0x7cu
#define MCHP_ECS_JT_TMS_OFS		0x80u

/* JTAG Controller Command */
#define MCHP_ECS_JT_CMD_OFS		0x84u
#define MCHP_ECS_JT_CMD_MASK		0x1fu

/* VWire Source Configuration */
#define MCHP_ECS_VWSC_OFS		0x90u
#define MCHP_ECS_VWSC_MASK		0x07u
#define MCHP_ECS_VWSC_DFLT		0x07u
#define MCHP_ECS_VWSC_EC_SCI_DIS	BIT(0)
#define MCHP_ECS_VWSC_MBH_SMI_DIS	BIT(1)

/* Analog Comparator Control */
#define MCHP_ECS_ACC_OFS		0x94u
#define MCHP_ECS_ACC_MASK		0x15u
#define MCHP_ECS_ACC_EN0		BIT(0)
#define MCHP_ECS_ACC_CFG_LOCK0		BIT(2)
#define MCHP_ECS_ACC_EN1		BIT(4)

/* Analog Comparator Sleep Control */
#define MCHP_ECS_ACSC_OFS		0x98u
#define MCHP_ECS_ACSC_MASK		0x03u
#define MCHP_ECS_ACSC_DSLP_EN0		BIT(0)
#define MCHP_ECS_ACSC_DSLP_EN1		BIT(1)

/* Embedded Reset Enable */
#define MCHP_ECS_EMBR_EN_OFS		0xb0u
#define MCHP_ECS_EMBR_EN_MASK		0x01u
#define MCHP_ECS_EMBR_EN_ON		BIT(0)

/* Embedded Reset Timeout value */
#define MCHP_ECS_EMBR_TMOUT_OFS		0xb4u
#define MCHP_ECS_EMBR_TMOUT_MASK	0x07u
#define MCHP_ECS_EMBR_TMOUT_6S		0u
#define MCHP_ECS_EMBR_TMOUT_7S		1u
#define MCHP_ECS_EMBR_TMOUT_8S		2u
#define MCHP_ECS_EMBR_TMOUT_9S		3u
#define MCHP_ECS_EMBR_TMOUT_10S		4u
#define MCHP_ECS_EMBR_TMOUT_11S		5u
#define MCHP_ECS_EMBR_TMOUT_12S		6u
#define MCHP_ECS_EMBR_TMOUT_14S		7u

/* Embedded Reset Status */
#define MCHP_ECS_EMBR_STS_OFS		0xb8u
#define MCHP_ECS_EMBR_STS_MASK		0x01u
#define MCHP_ECS_EMBR_STS_RST		BIT(0)

/* Embedded Reset Count (RO) */
#define MCHP_ECS_EMBR_CNT_OFS		0xbcu
#define MCHP_ECS_EMBR_CNT_MASK		0x7ffffu

/**  @brief EC Subsystem (ECS) */
struct ecs_regs {
	uint32_t RSVD1[1];
	volatile uint32_t AHB_ERR_ADDR;		/* +0x04 */
	uint32_t RSVD2[2];
	volatile uint32_t OSC_ID;		/* +0x10 */
	volatile uint32_t AHB_ERR_CTRL;		/* +0x14 */
	volatile uint32_t INTR_CTRL;		/* +0x18 */
	volatile uint32_t ETM_CTRL;		/* +0x1c */
	volatile uint32_t DEBUG_CTRL;		/* +0x20 */
	volatile uint32_t OTP_LOCK;		/* +0x24 */
	volatile uint32_t WDT_CNT;		/* +0x28 */
	uint32_t RSVD3[5];
	volatile uint32_t PECI_DIS;		/* +0x40 */
	uint32_t RSVD4[3];
	volatile uint32_t VCI_FW_OVR;		/* +0x50 */
	volatile uint32_t BROM_STS;		/* +0x54 */
	volatile uint32_t CRYPTO_CFG;		/* +0x58 */
	uint32_t RSVD6[5];
	volatile uint32_t JTAG_MCFG;		/* +0x70 */
	volatile uint32_t JTAG_MSTS;		/* +0x74 */
	volatile uint32_t JTAG_MTDO;		/* +0x78 */
	volatile uint32_t JTAG_MTDI;		/* +0x7c */
	volatile uint32_t JTAG_MTMS;		/* +0x80 */
	volatile uint32_t JTAG_MCMD;		/* +0x84 */
	volatile uint32_t VCI_OUT_SEL;		/* +0x88 */
	uint32_t RSVD7[1];
	volatile uint32_t VW_FW_OVR;		/* +0x90 */
	volatile uint32_t CMP_CTRL;		/* +0x94 */
	volatile uint32_t CMP_SLP_CTRL;		/* +0x98 */
	uint32_t RSVD8[(0xb0 - 0x9c) / 4];
	volatile uint32_t EMB_RST_EN1;		/* +0xb0 */
	volatile uint32_t EMB_RST_TMOUT1;	/* +0xb4 */
	volatile uint32_t EMB_RST_STS;		/* +0xb8 */
	volatile uint32_t EMB_RST_CNT;		/* +0xbc */
	uint32_t RSVD9[(0x144 - 0xc0) / 4];
	volatile uint32_t SLP_STS_MIRROR;	/* +0x144 */
};

#endif /* #ifndef _MEC172X_ECS_H */

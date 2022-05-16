/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Macros for the Andes ATCSMU
 */

#ifndef SOC_RISCV_ANDES_V5_SMU_H_
#define SOC_RISCV_ANDES_V5_SMU_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/*
 * SMU Register Base Address
 */

#if DT_NODE_EXISTS(DT_INST(0, andestech_atcsmu100))
#define SMU_BASE			DT_REG_ADDR(DT_INST(0, andestech_atcsmu100))
#endif /* DT_NODE_EXISTS(DT_INST(0, andestech_atcsmu100)) */

/*
 * SMU register offset
 */

/* Basic */
#define SMU_SYSTEMVER			0x00
#define SMU_BOARDVER			0x04
#define SMU_SYSTEMCFG			0x08
#define SMU_SMUVER			0x0C

/* Reset vectors */
#define SMU_HARTn_RESET_VECTOR(n)	(0x50 +  0x8 * (n))

/* Power control slot */
#define SMU_PCSn_CFG(n)			(0x80 + 0x20 * (n))
#define SMU_PCSn_SCRATCH(n)		(0x80 + 0x20 * (n))
#define SMU_PCSn_MISC(n)		(0x80 + 0x20 * (n))
#define SMU_PCSn_MISC2(n)		(0x80 + 0x20 * (n))
#define SMU_PCSn_WE(n)			(0x80 + 0x20 * (n))
#define SMU_PCSn_CTL(n)			(0x80 + 0x20 * (n))
#define SMU_PCSn_STATUS(n)		(0x80 + 0x20 * (n))

/* Pinmux */
#define SMU_PINMUX_CTRL0		0x1000
#define SMU_PINMUX_CTRL1		0x1004

/*
 * SMU helper constant
 */

#define SMU_SYSTEMCFG_CORENUM_MASK	0xFF
#define SMU_SYSTEMCFG_L2C		BIT(8)
#define SMU_SYSTEMCFG_DFS		BIT(9)
#define SMU_SYSTEMCFG_DC_COHEN		BIT(10)

#endif /* SOC_RISCV_ANDES_V5_SMU_H_ */

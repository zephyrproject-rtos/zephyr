/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_KSCAN_H
#define _MEC_KSCAN_H

#include <stdint.h>
#include <stddef.h>

/* KSO_SEL */
#define MCHP_KSCAN_KSO_SEL_REG_MASK	0xffu
#define MCHP_KSCAN_KSO_LINES_POS	0u
#define MCHP_KSCAN_KSO_LINES_MASK0	0x1fu
#define MCHP_KSCAN_KSO_LINES_MASK	0x1fu
#define MCHP_KSCAN_KSO_ALL_POS		5u
#define MCHP_KSCAN_KSO_ALL		BIT(5)
#define MCHP_KSCAN_KSO_EN_POS		6u
#define MCHP_KSCAN_KSO_EN		BIT(6)
#define MCHP_KSCAN_KSO_INV_POS		7u
#define MCHP_KSCAN_KSO_INV		BIT(7)

/* KSI_IN */
#define MCHP_KSCAN_KSI_IN_REG_MASK	0xffu

/* KSI_STS */
#define MCHP_KSCAN_KSI_STS_REG_MASK	0xffu

/* KSI_IEN */
#define MCHP_KSCAN_KSI_IEN_REG_MASK	0xffu

/* EXT_CTRL */
#define MCHP_KSCAN_EXT_CTRL_REG_MASK	0x01u
#define MCHP_KSCAN_EXT_CTRL_PREDRV_EN	0x01u

/** @brief Keyboard scan matrix controller. Size = 24(0x18) */
struct kscan_regs {
	uint32_t RSVD[1];
	volatile uint32_t KSO_SEL;
	volatile uint32_t KSI_IN;
	volatile uint32_t KSI_STS;
	volatile uint32_t KSI_IEN;
	volatile uint32_t EXT_CTRL;
};

#endif	/* #ifndef _MEC_KSCAN_H */

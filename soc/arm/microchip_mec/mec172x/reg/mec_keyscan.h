/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_KSCAN_H
#define _MEC_KSCAN_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_KSCAN_BASE_ADDR	0x40009C00ul

/* KSCAN interrupts */
#define MCHP_KSCAN_GIRQ		21u
#define MCHP_KSCAN_GIRQ_NVIC	13u
/* Direct mode connection to NVIC */
#define MCHP_KSCAN_NVIC		135u
/* backwards compatibility misspelling */
#define MCHP_KSAN_NVIC		(MCHP_KSCAN_NVIC)

#define MCHP_KSCAN_GIRQ_POS	25u
#define MCHP_KSCAN_GIRQ_VAL	BIT(25)

/* KSO_SEL */
#define MCHP_KSCAN_KSO_SEL_REG_MASK	0xFFul
#define MCHP_KSCAN_KSO_LINES_POS	0u
#define MCHP_KSCAN_KSO_LINES_MASK0	0x1Ful
#define MCHP_KSCAN_KSO_LINES_MASK	0x1Ful
#define MCHP_KSCAN_KSO_ALL_POS		5u
#define MCHP_KSCAN_KSO_ALL		BIT(5)
#define MCHP_KSCAN_KSO_EN_POS		6u
#define MCHP_KSCAN_KSO_EN		BIT(6)
#define MCHP_KSCAN_KSO_INV_POS		7u
#define MCHP_KSCAN_KSO_INV		BIT(7)

/* KSI_IN */
#define MCHP_KSCAN_KSI_IN_REG_MASK	0xFFul

/* KSI_STS */
#define MCHP_KSCAN_KSI_STS_REG_MASK	0xFFul

/* KSI_IEN */
#define MCHP_KSCAN_KSI_IEN_REG_MASK	0xFFul

/* EXT_CTRL */
#define MCHP_KSCAN_EXT_CTRL_REG_MASK	0x01ul
#define MCHP_KSCAN_EXT_CTRL_PREDRV_EN	0x01ul

#endif	/* #ifndef _MEC_KSCAN_H */

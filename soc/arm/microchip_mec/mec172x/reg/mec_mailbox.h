/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_MAILBOX_H
#define _MEC_MAILBOX_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_MBOX_BASE_ADDR	0x400F0000ul

/* Mailbox interrupts */
#define MCHP_MBOX_GIRQ		15u
#define MCHP_MBOX_GIRQ_NVIC	7u
#define MCHP_MBOX_NVIC		60u

#define MCHP_KBC_MBOX_GIRQ_POS	20u
#define MCHP_KBC_MBOX_GIRQ	BIT(MCHP_KBC_MBOX_GIRQ_POS)

/* SMI Source register */
#define MCHP_MBOX_SMI_SRC_EC_WR_POS	0u
#define MCHP_MBOX_SMI_SRC_EC_WR		BIT(MCHP_MBOX_SMI_SRC_WR_POS)
#define MCHP_MBOX_SMI_SRC_SWI_POS	1u
#define MCHP_MBOX_SMI_SRC_SWI_MASK0	0x7Ful
#define MCHP_MBOX_SMI_SRC_SWI_MASK	0xFEul
#define MCHP_MBOX_SMI_SRC_SWI0		BIT(1)
#define MCHP_MBOX_SMI_SRC_SWI1		BIT(2)
#define MCHP_MBOX_SMI_SRC_SWI2		BIT(3)
#define MCHP_MBOX_SMI_SRC_SWI3		BIT(4)
#define MCHP_MBOX_SMI_SRC_SWI4		BIT(5)
#define MCHP_MBOX_SMI_SRC_SWI5		BIT(6)
#define MCHP_MBOX_SMI_SRC_SWI6		BIT(7)

/* SMI Mask register */
#define MCHP_MBOX_SMI_MASK_WR_EN_POS	0u
#define MCHP_MBOX_SMI_MASK_WR_EN	BIT(MCHP_MBOX_SMI_MASK_WR_EN_POS)
#define MCHP_MBOX_SMI_SWI_EN_POS	1u
#define MCHP_MBOX_SMI_SWI_EN_MASK0	0x7Ful
#define MCHP_MBOX_SMI_SWI_EN_MASK	0xFEul
#define MCHP_MBOX_SMI_SRC_EN_SWI0	BIT(1)
#define MCHP_MBOX_SMI_SRC_EN_SWI1	BIT(2)
#define MCHP_MBOX_SMI_SRC_EN_SWI2	BIT(3)
#define MCHP_MBOX_SMI_SRC_EN_SWI3	BIT(4)
#define MCHP_MBOX_SMI_SRC_EN_SWI4	BIT(5)
#define MCHP_MBOX_SMI_SRC_EN_SWI5	BIT(6)
#define MCHP_MBOX_SMI_SRC_EN_SWI6	BIT(7)

#endif /* #ifndef _MEC_MAILBOX_H */

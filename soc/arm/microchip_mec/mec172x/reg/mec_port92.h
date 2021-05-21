/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PORT92_H
#define _MEC_PORT92_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"

#define MCHP_PORT92_BASE_ADDR	0x400F2000ul

/* HOST_P92 */
#define MCHP_PORT92_HOST_MASK			0x03ul
#define MCHP_PORT92_HOST_ALT_CPU_RST_POS	0u
#define MCHP_PORT92_HOST_ALT_CPU_RST		BIT(0)
#define MCHP_PORT92_HOST_ALT_GA20_POS		1u
#define MCHP_PORT92_HOST_ALT_GA20		BIT(1)

/* GATEA20_CTRL */
#define MCHP_PORT92_GA20_CTRL_MASK	0x01ul
#define MCHP_PORT92_GA20_CTRL_VAL_POS	0u
#define MCHP_PORT92_GA20_CTRL_VAL_MASK	BIT(0)
#define MCHP_PORT92_GA20_CTRL_VAL_HI	BIT(0)
#define MCHP_PORT92_GA20_CTRL_VAL_LO	0ul

/*
 * SETGA20L - writes of any data to this register causes
 * GATEA20 latch to be set.
 */
#define MCHP_PORT92_SETGA20L_MASK	0x01ul
#define MCHP_PORT92_SETGA20L_SET_POS	0u
#define MCHP_PORT92_SETGA20L_SET	BIT(0)

/*
 * RSTGA20L - writes of any data to this register causes
 * the GATEA20 latch to be reset
 */
#define MCHP_PORT92_RSTGA20L_MASK	0x01ul
#define MCHP_PORT92_RSTGA20L_SET_POS	0u
#define MCHP_PORT92_RSTGA20L_RST	BIT(0)

/* ACTV */
#define MCHP_PORT92_ACTV_MASK		0x01ul
#define MCHP_PORT92_ACTV_ENABLE		0x01ul

#endif /* #ifndef _MEC_PORT92_H */

/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_H_
#define _NUVOTON_NPCX_SOC_H_

#include <cmsis_core_m_defaults.h>

/* NPCX7 SCFG multi-registers offset */
#define NPCX_DEVALT_OFFSET(n)		(0x010 + n)
#define NPCX_PUPD_EN_OFFSET(n)		(0x028 + n)
#define NPCX_LV_GPIO_CTL_OFFSET(n)	((n < 5) ? (0x02a + n) : (0x021 + n))
#define NPCX_DEVALT_LK_OFFSET(n)	(0x210 + n)

/* NPCX7 MIWU multi-registers offset */
#define NPCX_WKEDG_OFFSET(n)		(0x000 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKAEDG_OFFSET(n)		(0x001 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKMOD_OFFSET(n)		(0x070 + n)
#define NPCX_WKPND_OFFSET(n)		(0x00a + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKPCL_OFFSET(n)		(0x00c + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKEN_OFFSET(n)		(0x01e + (n * 2) + ((n < 5) ? 0 : 0x012))
#define NPCX_WKINEN_OFFSET(n)		(0x01f + (n * 2) + ((n < 5) ? 0 : 0x012))

/* NPCX7 ADC multi-registers offset */
#define NPCX_CHNDAT_OFFSET(n)		(0x040 + (n * 2))
#define NPCX_THRCTL_OFFSET(n)		(0x014 + (n * 2))

/* NPCX7 ADC register fields */
#define NPCX_THRCTL_THEN		15
#define NPCX_THRCTL_L_H			14
#define NPCX_THRCTL_CHNSEL		FIELD(10, 4)
#define NPCX_THRCTL_THRVAL		FIELD(0, 10)

/* NPCX7 supported group mask of DEVALT_LK */
#define NPCX_DEVALT_LK_GROUP_MASK \
	(BIT(0) | BIT(2) | BIT(3) | BIT(4) | \
	 BIT(6) | BIT(11) | BIT(15))	/* DEVALT0_LK - DEVALTF_LK */

/* NPCX7 Clock configuration */
#define MAX_OFMCLK 100000000

#include "reg_def.h"
#include "clock_def.h"
#include <soc_dt.h>
#include <soc_pins.h>
#include <soc_power.h>

#endif /* _NUVOTON_NPCX_SOC_H_ */

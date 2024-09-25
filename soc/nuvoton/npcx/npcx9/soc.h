/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_H_
#define _NUVOTON_NPCX_SOC_H_

#include <cmsis_core_m_defaults.h>

/* NPCX9 SCFG multi-registers */
#define NPCX_DEVALT_OFFSET(n)		(0x010 + n)
#define NPCX_PUPD_EN_OFFSET(n)		(0x028 + n)
#define NPCX_LV_GPIO_CTL_OFFSET(n)	((n < 5) ? (0x02a + n) : (0x021 + n))
#define NPCX_DEVALT_LK_OFFSET(n)	(0x210 + n)

/* NPCX9 MIWU multi-registers */
#define NPCX_WKEDG_OFFSET(n)		(0x000 + (n * 0x010))
#define NPCX_WKAEDG_OFFSET(n)		(0x001 + (n * 0x010))
#define NPCX_WKMOD_OFFSET(n)		(0x002 + (n * 0x010))
#define NPCX_WKPND_OFFSET(n)		(0x003 + (n * 0x010))
#define NPCX_WKPCL_OFFSET(n)		(0x004 + (n * 0x010))
#define NPCX_WKEN_OFFSET(n)		(0x005 + (n * 0x010))
#define NPCX_WKST_OFFSET(n)		(0x006 + (n * 0x010))
#define NPCX_WKINEN_OFFSET(n)		(0x007 + (n * 0x010))

/* NPCX9 ADC multi-registers */
#define NPCX_CHNDAT_OFFSET(n)		(0x040 + (n * 2))
#define NPCX_THRCTL_OFFSET(n)		(0x060 + (n * 2))

/* NPCX9 ADC register fields */
#define NPCX_THRCTL_THEN		15
#define NPCX_THRCTL_L_H			14
#define NPCX_THRCTL_CHNSEL		FIELD(10, 4)
#define NPCX_THRCTL_THRVAL		FIELD(0, 10)

/* NPCX9 FIU register fields */
#define NPCX_FIU_EXT_CFG_SPI1_2DEV	7

/* NPCX9 supported group mask of DEVALT_LK */
#define NPCX_DEVALT_LK_GROUP_MASK \
	(BIT(0) | BIT(2) | BIT(3) | BIT(4) | \
	 BIT(5) | BIT(6) | BIT(11) | BIT(13) | \
	 BIT(15) | BIT(16) | BIT(17) | BIT(18))	/* DEVALT0_LK - DEVALTJ_LK */

/* NPCX9 Clock configuration and limitation */
#define MAX_OFMCLK 100000000

#include <reg/reg_access.h>
#include <reg/reg_def.h>
#include <soc_dt.h>
#include <soc_clock.h>
#include <soc_pins.h>
#include <soc_power.h>

#endif /* _NUVOTON_NPCX_SOC_H_ */

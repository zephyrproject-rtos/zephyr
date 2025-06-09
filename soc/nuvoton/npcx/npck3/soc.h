/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_H_
#define _NUVOTON_NPCX_SOC_H_

#include <cmsis_core_m_defaults.h>

/* NPCK3 SCFG multi-registers */
#define NPCX_DEVALT_OFFSET(n)		((n < 0x10) ? (0x010 + n) : \
					((n < 0x13) ? (0x0b + (n - 0x10)) : \
					(0x024 + (n - 0x13))))
#define NPCX_PUPD_EN_OFFSET(n)		(0x028 + n)
#define NPCX_LV_GPIO_CTL_OFFSET(n)	(0x02a + n)
#define NPCX_DEVALT_LK_OFFSET(n)	(0)

/* NPCK3 MIWU multi-registers */
#define NPCX_WKEDG_OFFSET(n)		(0x000 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKAEDG_OFFSET(n)		(0x001 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKMOD_OFFSET(n)		(0x070 + n)
#define NPCX_WKPND_OFFSET(n)		(0x00a + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKPCL_OFFSET(n)		(0x00c + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKEN_OFFSET(n)		(0x01e + (n * 2) + ((n < 5) ? 0 : 0x012))
#define NPCX_WKINEN_OFFSET(n)		(0x01f + (n * 2) + ((n < 5) ? 0 : 0x012))

/* NPCK3 PMC multi-registers */
#define NPCX_PWDWN_CTL_OFFSET(n)	(0x007 + n)

/* NPCK3 ADC multi-registers */
#define NPCX_CHNDAT_OFFSET(n)		(0x040 + n * 2)
#define NPCX_THRCTL_OFFSET(n)		(0x014 + n * 2)
#define NPCX_TCHNDAT_OFFSET(n)		(0x10E + n * 2)
#define NPCX_TEMP_THRCTL_OFFSET(n)	(0x180 + n * 2)
#define NPCX_TEMP_THR_DCTL_OFFSET(n)	(0x1A0 + n * 2)

/* NPCK3 ADC register fields */
#define NPCX_THRCTL_THEN		15
#define NPCX_THRCTL_L_H			14
#define NPCX_THRCTL_CHNSEL		FIELD(10, 4)
#define NPCX_THRCTL_THRVAL		FIELD(0, 10)

/* NPCK3 FIU register fields */
#define NPCK_FIFO_EN			0
#define NPCK_RX_FIFO_LEVEL		FIELD(6, 2)
#define NPCK_SZ_UART_FIFO		16
#define NPCX_FIU_EXT_CFG_SPI1_2DEV	0

/* NPCK3 GLUE register fields */
#define NPCX_EPURST_CTL_EPUR1_AHI	0
#define NPCX_EPURST_CTL_EPUR1_EN	1
#define NPCX_EPURST_CTL_EPUR2_AHI	2
#define NPCX_EPURST_CTL_EPUR2_EN	3
#define NPCX_EPURST_CTL_EPUR_LK		7

/* NPCK3 TWD register fields */
#define NPCX_T0CSR_T0EN			6

/* No DEVALT_LK mechanism in NPCK3 series */
#define NPCX_DEVALT_LK_GROUP_MASK	0x00000000

/* NPCK3 Clock configuration and limitation */
#define MAX_OFMCLK 100000000

#include "reg_def.h"
#include "clock_def.h"
#include <soc_dt.h>
#include <soc_espi_taf.h>
#include <soc_pins.h>
#include <soc_power.h>

/* NPCK3 Clock prescaler configurations */
#define VAL_HFCGP   ((FPRED_VAL << 4) | AHB6DIV_VAL)
#define VAL_HFCBCD  (APB1DIV_VAL | (APB2DIV_VAL << 4))
#define VAL_HFCBCD1 0 /* Keep the same as reset value 0*/
#define VAL_HFCBCD2 (APB3DIV_VAL | (FIUDIV_VAL << 4))

#endif /* _NUVOTON_NPCX_SOC_H_ */

/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_H_
#define _NUVOTON_NPCM_SOC_H_

#include <cmsis_core_m_defaults.h>

/* NPCM4 SCFG multi-registers */
#define NPCX_DEVALT_OFFSET(n)      (n)
#define NPCX_PUPD_EN_OFFSET(n)     (((n) == 2) ? 0x073 : ((n) == 3) ? 0x07b : (0x028 + (n)))
#define NPCX_LV_GPIO_CTL_OFFSET(n) (((n) == 4) ? 0x06e : 0x2a + (n))

/* NPCM4 MIWU multi-registers */
#define NPCX_WKEDG_OFFSET(n)  (0x000 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKAEDG_OFFSET(n) (0x001 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKMOD_OFFSET(n)  (0x070 + (n))
#define NPCX_WKPND_OFFSET(n)  (0x00a + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKPCL_OFFSET(n)  (0x00c + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKEN_OFFSET(n)   (0x01e + (n * 2) + ((n < 5) ? 0 : 0x012))
#define NPCX_WKINEN_OFFSET(n) (0x01f + (n * 2) + ((n < 5) ? 0 : 0x012))

/* NPCM4 PMC multi-registers */
#define NPCX_PWDWN_CTL_OFFSET(n) (((n - 1) < 6) ? (0x008 + (n - 1)) : (0x01e + (n - 1)))

/* NPCM4 ADC multi-registers */
#define NPCX_CHNDAT_OFFSET(n) (0x040 + n * 2)
#define NPCX_THRCTL_OFFSET(n) (0x080 + n * 2)
#define NPCX_THEN_OFFSET      0x090
#define THEN(base)            (*(volatile uint16_t *)(base + NPCX_THEN_OFFSET))

/* NPCM4 ADC register fields */
#define NPCX_THRCTL_L_H    15
#define NPCX_THRCTL_CHNSEL FIELD(10, 5)
#define NPCX_THRCTL_THRVAL FIELD(0, 10)

/* NPCM4 FIU register fields */
#define NPCX_FIU_EXT_CFG_SPI1_2DEV   6
#define NPCX_FIU_EXT_CFG_LOW_DEV_NUM 7

/* NPCM4 UART register fields */
#define NPCK_FIFO_EN       0
#define NPCK_RX_FIFO_LEVEL FIELD(6, 2)
#define NPCK_SZ_UART_FIFO  16

/* NPCM4 supported group mask of DEVALT_LK */
#define NPCX_DEVALT_LK_GROUP_MASK                                                                  \
	(BIT(0) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6) | BIT(11) | BIT(13) | BIT(15) |       \
	 BIT(16) | BIT(17) | BIT(18) | BIT(19) | BIT(21)) /* DEVALT0_LK - DEVALTN_LK */

/* NPCM4 Clock Configuration */
#define MAX_OFMCLK 96000000

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#include "reg_def.h"
#include "clock_def.h"
#include <soc_dt.h>
#include <soc_pins.h>
#include <soc_power.h>

/* NPCM4 Clock prescaler configurations */
#define VAL_HFCGP   ((FPRED_VAL << 4) | AHB6DIV_VAL)
#define VAL_HFCBCD  (APB1DIV_VAL | (APB2DIV_VAL << 4))
#define VAL_HFCBCD1 ((MCLKD_SL << 2) | FIUDIV_VAL)
#define VAL_HFCBCD2 APB3DIV_VAL

#endif /* _NUVOTON_NPCM_SOC_H_ */

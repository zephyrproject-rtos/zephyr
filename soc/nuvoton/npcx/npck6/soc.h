/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_H_
#define _NUVOTON_NPCX_SOC_H_

#include <cmsis_core_m_defaults.h>

/* NPCK6 SCFG multi-registers */
#define NPCX_DEVALT_OFFSET(n)                                                                      \
	((n < 0x10)                                                                                \
		 ? (0x010 + n)                                                                     \
		 : ((n < 0x13)                                                                     \
			    ? (0x0b + (n - 0x10))                                                  \
			    : ((n < 0x17) ? (0x024 + (n - 0x13))                                   \
					  : ((n < 0x19) ? (0x030 + (n - 0x17))                     \
							: ((n == 0x1d) ? (0x033)                   \
								       : (0x060 + (n - 0x19)))))))
#define NPCX_PUPD_EN_OFFSET(n) (0x028 + n)
#define NPCX_LV_GPIO_CTL_OFFSET(n)                                                                 \
	(((n) < 5) ? (0x02a + (n)) : ((n) < 18) ? (0x065 + (n - 5)) : 0x23)
#define NPCX_DEVALT_LK_OFFSET(n) (0)

/* NPCK6 MIWU multi-registers */
#define NPCX_WKEDG_OFFSET(n)  (0x000 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKAEDG_OFFSET(n) (0x001 + (n * 2) + ((n < 5) ? 0 : 0x01e))
#define NPCX_WKMOD_OFFSET(n)  (0x070 + n)
#define NPCX_WKPND_OFFSET(n)  (0x00a + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKPCL_OFFSET(n)  (0x00c + (n * 4) + ((n < 5) ? 0 : 0x010))
#define NPCX_WKEN_OFFSET(n)   (0x01e + (n * 2) + ((n < 5) ? 0 : 0x012))
#define NPCX_WKINEN_OFFSET(n) (0x01f + (n * 2) + ((n < 5) ? 0 : 0x012))

/* NPCK6 SMB wake-up: SMB1..SMB6 in SMB_SBD/SMB_EEN, SMB7..SMB9 in SMB_SBD1/SMB_EEN1 */
#define NPCX_SMB_WKUP_REG_CTRL_CNT  6
#define NPCX_SMB_WKUP_REG1_CTRL_CNT 3

/* NPCK6 PMC multi-registers */
#define NPCX_PWDWN_CTL_OFFSET(n) (((n) < 7) ? (0x007 + (n)) : (0x014 + ((n) - 7)))

/* NPCK6 ADC multi-registers */
#define NPCX_CHNDAT_OFFSET(n)        (0x040 + n * 2)
#define NPCX_THRCTL_OFFSET(n)        (0x014 + n * 2)
#define NPCX_TCHNDAT_OFFSET(n)       (0x10E + n * 2)
#define NPCX_TEMP_THRCTL_OFFSET(n)   (0x180 + n * 2)
#define NPCX_TEMP_THR_DCTL_OFFSET(n) (0x1A0 + n * 2)

/* NPCK6 ADC register fields */
#define NPCX_THRCTL_THEN   15
#define NPCX_THRCTL_L_H    14
#define NPCX_THRCTL_CHNSEL FIELD(10, 4)
#define NPCX_THRCTL_THRVAL FIELD(0, 10)

/* NPCK6 FIU register fields */
#define NPCK_FIFO_EN               0
#define NPCK_RX_FIFO_LEVEL         FIELD(6, 2)
#define NPCK_SZ_UART_FIFO          16
#define NPCX_FIU_EXT_CFG_SPI1_2DEV 0

/* NPCK6 SCFG register fields */
#define NPCK_DEV_CTL3_WP_IF 3

/* NPCK6 GLUE register fields */
#define NPCX_EPURST_CTL_EPUR1_AHI 0
#define NPCX_EPURST_CTL_EPUR1_EN  1
#define NPCX_EPURST_CTL_EPUR2_AHI 2
#define NPCX_EPURST_CTL_EPUR2_EN  3
#define NPCX_EPURST_CTL_EPUR_LK   7
#define NPCX_EPURST_EPUR_DBC      FIELD(0, 5)

/* NPCK6 TWD register fields */
#define NPCX_T0CSR_T0EN 6

/* No DEVALT_LK mechanism in NPCK6 series */
#define NPCX_DEVALT_LK_GROUP_MASK 0x00000000

/* NPCK6 Clock configuration and limitation */
#define MAX_OFMCLK  160000000
#define MAX_FMCLK   50000000
#define MAX_AHB6CLK 100000000
#define MAX_FIUCLK  100000000
#define XF_4_RANGE_SUPP

#include <reg_def.h>
#include <clock_def.h>
#include <soc_dt.h>
#include <soc_clock.h>
#include <soc_espi_taf.h>
#include <soc_pins.h>
#include <soc_power.h>

/* NPCK6 Clock prescaler configurations */
#define VAL_HFCGP   ((FPRED_VAL << 4) | AHB6DIV_VAL)
#define VAL_HFCBCD  (APB1DIV_VAL | (APB2DIV_VAL << 4))
#define VAL_HFCBCD1 0 /* Keep the same as reset value 0*/
#define VAL_HFCBCD2 (APB3DIV_VAL | (FIUDIV_VAL << 4))

#endif /* _NUVOTON_NPCX_SOC_H_ */

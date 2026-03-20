/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC26_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC26_H_

#include <zephyr/sys/util_macro.h>

/* SPI command word */
#define AIC26_CMD_READ		BIT(15)
#define AIC26_CMD_WRITE		0
#define AIC26_CMD_PAGE		GENMASK(14, 11)
#define AIC26_CMD_ADDR		GENMASK(10, 5)

/* Page numbers */
#define AIC26_PAGE1		1
#define AIC26_PAGE2		2

/* Page 1 registers */
#define AIC26_P1_REFERENCE	0x03
#define AIC26_P1_RESET		0x04

/* Reference Control (Page 1, Reg 0x03) */
#define AIC26_VREFM		BIT(4)
#define AIC26_IREFV		BIT(0)

#define AIC26_RESET_VALUE	0xBB00

/* Page 2 registers */
#define AIC26_P2_AUDIO_CTL1	0x00
#define AIC26_P2_ADC_GAIN	0x01
#define AIC26_P2_DAC_GAIN	0x02
#define AIC26_P2_POWER_CTL	0x05
#define AIC26_P2_AUDIO_CTL3	0x06
#define AIC26_P2_PLL1		0x1B
#define AIC26_P2_PLL2		0x1C

/* Audio Control 1 (Page 2, Reg 0x00) */
#define AIC26_ADCHPF		GENMASK(15, 14)
#define AIC26_ADCIN		GENMASK(13, 12)
#define AIC26_WLEN		GENMASK(11, 10)
#define AIC26_DATFM		GENMASK(9, 8)
#define AIC26_DACFS		GENMASK(5, 3)
#define AIC26_ADCFS		GENMASK(2, 0)

/* ADCHPF values */
#define AIC26_HPF_OFF		0
#define AIC26_HPF_0045FS	1
#define AIC26_HPF_0125FS	2
#define AIC26_HPF_025FS		3

/* ADCIN values */
#define AIC26_ADCIN_MIC		0
#define AIC26_ADCIN_AUX		1

/* WLEN values */
#define AIC26_WLEN_16		0
#define AIC26_WLEN_20		1
#define AIC26_WLEN_24		2
#define AIC26_WLEN_32		3

/* DATFM values */
#define AIC26_DATFM_I2S		0
#define AIC26_DATFM_DSP		1
#define AIC26_DATFM_RJ		2
#define AIC26_DATFM_LJ		3

/* FS divisor values (shared by DACFS and ADCFS) */
#define AIC26_FS_DIV_1		0
#define AIC26_FS_DIV_1_5	1
#define AIC26_FS_DIV_2		2
#define AIC26_FS_DIV_3		3
#define AIC26_FS_DIV_4		4
#define AIC26_FS_DIV_5		5
#define AIC26_FS_DIV_5_5	6
#define AIC26_FS_DIV_6		7

/* ADC Gain Control (Page 2, Reg 0x01) */
#define AIC26_ADMUT		BIT(15)
#define AIC26_ADPGA		GENMASK(14, 8)
#define AIC26_AGCTG		GENMASK(7, 5)
#define AIC26_AGCTC		GENMASK(4, 1)
#define AIC26_AGCEN		BIT(0)

/* AGC target levels */
#define AIC26_AGCTG_M5P5	0
#define AIC26_AGCTG_M8		1
#define AIC26_AGCTG_M10		2
#define AIC26_AGCTG_M12		3
#define AIC26_AGCTG_M14		4
#define AIC26_AGCTG_M17		5
#define AIC26_AGCTG_M20		6
#define AIC26_AGCTG_M24		7

/* AGC time constants (attack_ms / decay_ms) */
#define AIC26_AGCTC_8_100	0
#define AIC26_AGCTC_11_100	1
#define AIC26_AGCTC_16_100	2
#define AIC26_AGCTC_20_100	3

/* DAC Gain (Page 2, Reg 0x02) */
#define AIC26_DALMU		BIT(15)
#define AIC26_DALVL		GENMASK(14, 8)
#define AIC26_DARMU		BIT(7)
#define AIC26_DARVL		GENMASK(6, 0)

/* Power Control (Page 2, Reg 0x05) */
#define AIC26_PWDNC		BIT(15)
#define AIC26_DAODRC		BIT(12)
#define AIC26_DAPWDN		BIT(10)
#define AIC26_ADPWDN		BIT(9)
#define AIC26_ADPWDF		BIT(7)
#define AIC26_DAPWDF		BIT(6)

/* Audio Control 3 (Page 2, Reg 0x06) */
#define AIC26_REFFS		BIT(13)
#define AIC26_SLVMS		BIT(11)
#define AIC26_AGCNL		GENMASK(5, 4)

/* AGC noise threshold */
#define AIC26_AGCNL_M60		0
#define AIC26_AGCNL_M70		1
#define AIC26_AGCNL_M80		2
#define AIC26_AGCNL_M90		3

/* PLL1 (Page 2, Reg 0x1B) */
#define AIC26_PLLSEL		BIT(15)
#define AIC26_QVAL		GENMASK(14, 11)
#define AIC26_PVAL		GENMASK(10, 8)
#define AIC26_JVAL		GENMASK(7, 2)

/* PLL2 (Page 2, Reg 0x1C) */
#define AIC26_DVAL		GENMASK(15, 2)

#endif

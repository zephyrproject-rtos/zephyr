/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_ADC_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_ADC_H

struct adc_regs {
	uint32_t ctrl;
	uint32_t chctrl;
	uint32_t sts;
	uint32_t chdata[12];
	uint32_t coeffa;
	uint32_t coeffb;
};

/* CTRL */
#define ADC_CTRL_EN         BIT(0)
#define ADC_CTRL_START      BIT(1)
#define ADC_CTRL_RST        BIT(2)
#define ADC_CTRL_MDSEL      BIT(3)
#define ADC_CTRL_SGLDNINTEN BIT(4)
#define ADC_CTRL_RPTDNINTEN BIT(5)

/* CHCTRL */
#define ADC_CHCTRL_CH0EN  BIT(0)
#define ADC_CHCTRL_CH1EN  BIT(1)
#define ADC_CHCTRL_CH2EN  BIT(2)
#define ADC_CHCTRL_CH3EN  BIT(3)
#define ADC_CHCTRL_CH4EN  BIT(4)
#define ADC_CHCTRL_CH5EN  BIT(5)
#define ADC_CHCTRL_CH6EN  BIT(6)
#define ADC_CHCTRL_CH7EN  BIT(7)
#define ADC_CHCTRL_CH8EN  BIT(8)
#define ADC_CHCTRL_CH9EN  BIT(9)
#define ADC_CHCTRL_CH10EN BIT(10)
#define ADC_CHCTRL_CH11EN BIT(11)
#define ADC_CHCTRL_LPFBP  BIT(12)
#define ADC_CHCTRL_CALBP  BIT(24)

/* STS */
#define ADC_STS_CH0DN  BIT(0)
#define ADC_STS_CH1DN  BIT(1)
#define ADC_STS_CH2DN  BIT(2)
#define ADC_STS_CH3DN  BIT(3)
#define ADC_STS_CH4DN  BIT(4)
#define ADC_STS_CH5DN  BIT(5)
#define ADC_STS_CH6DN  BIT(6)
#define ADC_STS_CH7DN  BIT(7)
#define ADC_STS_CH8DN  BIT(8)
#define ADC_STS_CH9DN  BIT(9)
#define ADC_STS_CH10DN BIT(10)
#define ADC_STS_CH11DN BIT(11)
#define ADC_STS_SGLDN  BIT(12)
#define ADC_STS_RPTDN  BIT(13)
#define ADC_STS_RDY    BIT(16)
#define ADC_STS_LPFSTB BIT(17)

/* CHDATA */
#define ADC_CHDATA_RESULT_Pos (0U)
#define ADC_CHDATA_RESULT_Msk GENMASK(11, 0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_ADC_H */

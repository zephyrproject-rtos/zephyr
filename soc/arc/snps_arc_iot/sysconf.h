/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _ARC_IOT_SYSCONF_H_
#define _ARC_IOT_SYSCONF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sysconf_reg {
	volatile uint32_t reserved1;	/* 0x0 */
	volatile uint32_t AHBCLKDIV;	/* AHB clock divisor */
	volatile uint32_t APBCLKDIV; 	/* APB clock divisor */
	volatile uint32_t APBCLKEN;	/* APB module clock enable */
	volatile uint32_t CLKODIV;	/* AHB clock output enable and divisor set */
	volatile uint32_t reserved2;      /* 0x14 */
	volatile uint32_t RSTCON;	/* reset contrl */
	volatile uint32_t RSTSTAT;	/* reset status */
	volatile uint32_t AHBCLKDIV_SEL; /* AHB clock divisor select */
	volatile uint32_t CLKSEL;	/* main clock source select */
	volatile uint32_t PLLSTAT;	/* PLL status register */
	volatile uint32_t PLLCON;	/* PLL control register */
	volatile uint32_t reserved3;	/* 0x30 */
	volatile uint32_t AHBCLKEN;	/* AHB module clock enbale */
	volatile uint32_t reserved4[2];  /* 0x38, 0x3c */
	volatile uint32_t I2S_TX_SCLKDIV; /* I2S TX SCLK divisor */
	volatile uint32_t I2S_RX_SCLKDIV; /* I2S RX SCLK divisor */
	volatile uint32_t I2S_RX_SCLKSEL; /* I2S RX SCLK source select */
	volatile uint32_t SDIO_REFCLK_DIV; /* SDIO reference clock divisor */
	volatile uint32_t GPIO4B_DBCLK_DIV; /* GPIO4B DBCLK divisor */
	volatile uint32_t IMAGE_CHK;	/* Image pad status */
	volatile uint32_t PROT_RANGE;	/* PROT range */
	volatile uint32_t SPI_MST_CLKDIV; /* SPI master clock divisor */
	volatile uint32_t DVFS_CLKDIV;	/* DFSS main clock domain divider */
	volatile uint32_t DVFS_VDDSET;	/* VDD setting */
	volatile uint32_t DVFS_VWTIME;	/* VDD adjust waiting time */
	volatile uint32_t PMC_PUWTIME;	/* power up waiting time */
	volatile uint32_t PMOD_MUX;	/* PMOD IO mux */
	volatile uint32_t ARDUINO_MUX;	/* arduino IO mux */
	volatile uint32_t USBPHY_PLL;	/* USBPHY PLL */
	volatile uint32_t USBCFG;	/* USB configuration */
	volatile uint32_t TIMER_PAUSE;	/* PWM timer puse */
	volatile uint32_t GPIO8B_DBCLK_DIV; /* GPIO8B DBCLK divisor */
	volatile uint32_t RESET_PD_VECTOR; /* reset powerdown vector */
	volatile uint32_t UART3SCLK_DIV;  /* UART3SCLK_DIV */
} sysconf_reg_t;

/* CLKSEL_CONST is not described in spec. */
#define CLKSEL_CONST	(0x5A690000)
#define CLKSEL_EXT_16M	(0 | CLKSEL_CONST)
#define CLKSEL_PLL		(1 | CLKSEL_CONST)
#define CLKSEL_EXT_32K	(2 | CLKSEL_CONST)

#define PLLCON_BIT_OFFSET_N 		0
#define PLLCON_BIT_OFFSET_M 		4
#define PLLCON_BIT_OFFSET_OD 		20
#define PLLCON_BIT_OFFSET_BP 		24
#define PLLCON_BIT_OFFSET_PLLRST	26


#define PLLSTAT_BIT_OFFSET_PLLSTB 	2
#define PLLSTAT_BIT_OFFSET_PLLRDY 	3


#define AHBCLKEN_BIT_I2S		0
#define AHBCLKEN_BIT_USB		1
#define AHBCLKEN_BIT_FLASH		2
#define AHBCLKEN_BIT_FMC		3
#define AHBCLKEN_BIT_DVFS		4
#define AHBCLKEN_BIT_PMC		5
#define AHBCLKEN_BIT_BOOT_SPI		6
#define AHBCLKEN_BIT_SDIO		7

#define APBCLKEN_BIT_ADC		0
#define APBCLKEN_BIT_I2S_TX		1
#define APBCLKEN_BIT_I2S_RX		2
#define APBCLKEN_BIT_RTC		3
#define APBCLKEN_BIT_PWM		4
#define APBCLKEN_BIT_I3C		5


#define SPI_MASTER_0			0
#define SPI_MASTER_1			1
#define SPI_MASTER_2			2

#define GPIO8B_BANK0			0
#define GPIO8B_BANK1			1
#define GPIO8B_BANK2			2
#define GPIO8B_BANK3			3

#define GPIO4B_BANK0			0
#define GPIO4B_BANK1			1
#define GPIO4B_BANK2			2

/* reset caused by power on */
#define SYS_RST_SOFTWARE_ON		0x2


#define DVFS_PERF_LEVEL0		0
#define DVFS_PERF_LEVEL1		1
#define DVFS_PERF_LEVEL2		2
#define DVFS_PERF_LEVEL3		3

/* pmode mux definition */
#define PMOD_MUX_PMA			(0x1)
#define PMOD_MUX_PMB			(0x2)
#define PMOD_MUX_PMC			(0x4)

/* arduino mux definition */
#define ARDUINO_MUX_UART		(0x1)
#define ARDUINO_MUX_SPI			(0x2)
#define ARDUINO_MUX_PWM0		(0x4)
#define ARDUINO_MUX_PWM1		(0x8)
#define ARDUINO_MUX_PWM2		(0x10)
#define ARDUINO_MUX_PWM3		(0x20)
#define ARDUINO_MUX_PWM4		(0x40)
#define ARDUINO_MUX_PWM5		(0x80)
#define ARDUINO_MUX_I2C			(0x100)
#define ARDUINO_MUX_ADC0		(0x400)
#define ARDUINO_MUX_ADC1		(0x800)
#define ARDUINO_MUX_ADC2		(0x1000)
#define ARDUINO_MUX_ADC3		(0x2000)
#define ARDUINO_MUX_ADC4		(0x4000)
#define ARDUINO_MUX_ADC5		(0x8000)

#define PWM_TIMER0			0
#define PWM_TIMER1			1
#define PWM_TIMER2			2
#define PWM_TIMER3			3
#define PWM_TIMER4			4
#define PWM_TIMER5			5


extern void arc_iot_pll_conf_reg(uint32_t val);
extern int32_t arc_iot_pll_fout_config(uint32_t freq);
extern void arc_iot_ahb_clk_divisor(uint8_t div);
extern void arc_iot_ahb_clk_enable(uint8_t dev);
extern void arc_iot_ahb_clk_disable(uint8_t dev);
extern void arc_iot_sdio_clk_divisor(uint8_t div);
extern void arc_iot_spi_master_clk_divisor(uint8_t id, uint8_t div);
extern void arc_iot_gpio8b_dbclk_div(uint8_t bank, uint8_t div);
extern void arc_iot_gpio4b_dbclk_div(uint8_t bank, uint8_t div);
extern void arc_iot_i2s_tx_clk_div(uint8_t div);
extern void arc_iot_i2s_rx_clk_div(uint8_t div);
extern void arc_iot_i2s_rx_clk_sel(uint8_t sel);
extern void arc_iot_syscon_reset(void);
extern uint32_t arc_iot_is_poweron_rst(void);
extern void arc_iot_dvfs_clk_divisor(uint8_t level, uint8_t div);
extern void arc_iot_dvfs_vdd_config(uint8_t level, uint8_t val);
extern void arc_iot_dvfs_vwtime_config(uint8_t time);
extern void arc_iot_pmc_pwwtime_config(uint8_t time);
extern void arc_iot_uart3_clk_divisor(uint8_t div);
extern void arc_iot_reset_powerdown_vector(uint32_t addr);
extern void arc_iot_eflash_clk_div(uint8_t div);

#ifdef __cplusplus
}
#endif

#endif /* _ARC_IOT_SYSCONF_H_ */

/*
 * Copyright (c) 2022-2024, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_AGILEX5_LL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_AGILEX5_LL_H_

#include <stdint.h>
#include <assert.h>

#include <zephyr/sys/sys_io.h>

/* Clock control MMIO register base address */
#define CLKCTRL_BASE_ADDR			DT_REG_ADDR(DT_NODELABEL(clock))

/* Clock manager/control register offsets */
#define CLKCTRL_OFFSET				0x00
#define CLKCTRL_CTRL				0x00
#define CLKCTRL_STAT				0x04
#define CLKCTRL_TESTIOCTRL			0x08
#define CLKCTRL_INTRGEN				0x0C
#define CLKCTRL_INTRMSK				0x10
#define CLKCTRL_INTRCLR				0x14
#define CLKCTRL_INTRSTS				0x18
#define CLKCTRL_INTRSTK				0x1C
#define CLKCTRL_INTRRAW				0x20

#define CLKCTRL(x)				(CLKCTRL_BASE_ADDR + CLKCTRL_##_reg)

/* Clock manager/control main PLL group register offsets */
#define CLKCTRL_MAINPLL_OFFSET			0x24
#define CLKCTRL_MAINPLL_EN			0x00
#define CLKCTRL_MAINPLL_ENS			0x04
#define CLKCTRL_MAINPLL_ENR			0x08
#define CLKCTRL_MAINPLL_BYPASS			0x0C
#define CLKCTRL_MAINPLL_BYPASSS			0x10
#define CLKCTRL_MAINPLL_BYPASSR			0x14
#define CLKCTRL_MAINPLL_NOCCLK			0x1C
#define CLKCTRL_MAINPLL_NOCDIV			0x20
#define CLKCTRL_MAINPLL_PLLGLOB			0x24
#define CLKCTRL_MAINPLL_FDBCK			0x28
#define CLKCTRL_MAINPLL_MEM			0x2C
#define CLKCTRL_MAINPLL_MEMSTAT			0x30
#define CLKCTRL_MAINPLL_VCOCALIB		0x34
#define CLKCTRL_MAINPLL_PLLC0			0x38
#define CLKCTRL_MAINPLL_PLLC1			0x3C
#define CLKCTRL_MAINPLL_PLLC2			0x40
#define CLKCTRL_MAINPLL_PLLC3			0x44
#define CLKCTRL_MAINPLL_PLLM			0x48
#define CLKCTRL_MAINPLL_FHOP			0x4C
#define CLKCTRL_MAINPLL_SSC			0x50
#define CLKCTRL_MAINPLL_LOSTLOCK		0x54

#define CLKCTRL_MAINPLL_BASE_ADDR		(CLKCTRL_BASE_ADDR + CLKCTRL_MAINPLL_OFFSET)
#define CLKCTRL_MAINPLL(_reg)			(CLKCTRL_MAINPLL_BASE_ADDR + CLKCTRL_MAINPLL_##_reg)

/* Clock manager/control peripheral PLL group register offsets */
#define CLKCTRL_PERPLL_OFFSET			0x7C
#define CLKCTRL_PERPLL_EN			0x00
#define CLKCTRL_PERPLL_ENS			0x04
#define CLKCTRL_PERPLL_ENR			0x08
#define CLKCTRL_PERPLL_BYPASS			0x0C
#define CLKCTRL_PERPLL_BYPASSS			0x10
#define CLKCTRL_PERPLL_BYPASSR			0x14
#define CLKCTRL_PERPLL_EMACCTL			0x18
#define CLKCTRL_PERPLL_GPIODIV			0x1C
#define CLKCTRL_PERPLL_PLLGLOB			0x20
#define CLKCTRL_PERPLL_FDBCK			0x24
#define CLKCTRL_PERPLL_MEM			0x28
#define CLKCTRL_PERPLL_MEMSTAT			0x2C
#define CLKCTRL_PERPLL_VCOCALIB			0x30
#define CLKCTRL_PERPLL_PLLC0			0x34
#define CLKCTRL_PERPLL_PLLC1			0x38
#define CLKCTRL_PERPLL_PLLC2			0x3C
#define CLKCTRL_PERPLL_PLLC3			0x40
#define CLKCTRL_PERPLL_PLLM			0x44
#define CLKCTRL_PERPLL_FHOP			0x48
#define CLKCTRL_PERPLL_SSC			0x4C
#define CLKCTRL_PERPLL_LOSTLOCK			0x50

#define CLKCTRL_PERPLL_BASE_ADDR		(CLKCTRL_BASE_ADDR + CLKCTRL_PERPLL_OFFSET)
#define CLKCTRL_PERPLL(_reg)			(CLKCTRL_PERPLL_BASE_ADDR + CLKCTRL_PERPLL_##_reg)

/* Clock manager/control controller group register offsets */
#define CLKCTRL_CTLGRP_OFFSET			0xD0
#define CLKCTRL_CTLGRP_JTAG			0x00
#define CLKCTRL_CTLGRP_EMACACTR			0x04
#define CLKCTRL_CTLGRP_EMACBCTR			0x08
#define CLKCTRL_CTLGRP_EMACPTPCTR		0x0C
#define CLKCTRL_CTLGRP_GPIODBCTR		0x10
#define CLKCTRL_CTLGRP_S2FUSER0CTR		0x18
#define CLKCTRL_CTLGRP_S2FUSER1CTR		0x1C
#define CLKCTRL_CTLGRP_PSIREFCTR		0x20
#define CLKCTRL_CTLGRP_EXTCNTRST		0x24
#define CLKCTRL_CTLGRP_USB31CTR			0x28
#define CLKCTRL_CTLGRP_DSUCTR			0x2C
#define CLKCTRL_CTLGRP_CORE01CTR		0x30
#define CLKCTRL_CTLGRP_CORE23CTR		0x34
#define CLKCTRL_CTLGRP_CORE2CTR			0x38
#define CLKCTRL_CTLGRP_CORE3CTR			0x3C
#define CLKCTRL_CTLGRP_SRL_CON_PLLCTR		0x40

#define CLKCTRL_CTLGRP_BASE_ADDR		(CLKCTRL_BASE_ADDR + CLKCTRL_CTLGRP_OFFSET)
#define CLKCTRL_CTLGRP(_reg)			(CLKCTRL_CTLGRP_BASE_ADDR + CLKCTRL_CTLGRP_##_reg)


/* Clock manager/control macros */
#define CLKCTRL_CTRL_BOOTMODE_SET_MSK		0x00000001U
#define CLKCTRL_STAT_BUSY_E_BUSY		0x1
#define CLKCTRL_STAT_BUSY(x)			(((x) & 0x00000001U) >> 0)
#define CLKCTRL_STAT_MAINPLLLOCKED(x)		(((x) & 0x00000100U) >> 8)
#define CLKCTRL_STAT_PERPLLLOCKED(x)		(((x) & 0x00010000U) >> 16)
#define CLKCTRL_INTRCLR_MAINLOCKLOST_SET_MSK	0x00000004U
#define CLKCTRL_INTRCLR_PERLOCKLOST_SET_MSK	0x00000008U
#define CLKCTRL_MAINPLL_L4SPDIV(x)		(((x) >> 16) & 0x3)
#define CLKCTRL_INTOSC_HZ			460000000U

#define CLKCTRL_CLKSRC_MASK			GENMASK(18, 16)
#define CLKCTRL_CLKSRC_OFFSET			16
#define CLKCTRL_CLKSRC_MAIN			0
#define CLKCTRL_CLKSRC_PER			1
#define CLKCTRL_CLKSRC_OSC1			2
#define CLKCTRL_CLKSRC_INTOSC			3
#define CLKCTRL_CLKSRC_FPGA			4
#define CLKCTRL_PLLCX_DIV_MSK			GENMASK(10, 0)
#define GET_CLKCTRL_CLKSRC(x)			(((x) & CLKCTRL_CLKSRC_MASK) >> \
							CLKCTRL_CLKSRC_OFFSET)

/* Shared Macros */
#define CLKCTRL_PSRC(x)				(((x) & 0x00030000U) >> 16)
#define CLKCTRL_PSRC_MAIN			0
#define CLKCTRL_PSRC_PER			1

#define CLKCTRL_PLLGLOB_PSRC_EOSC1		0x0
#define CLKCTRL_PLLGLOB_PSRC_INTOSC		0x1
#define CLKCTRL_PLLGLOB_PSRC_F2S		0x2

#define CLKCTRL_PLLM_MDIV(x)			((x) & 0x000003FFU)
#define CLKCTRL_PLLGLOB_PD_SET_MSK		0x00000001U
#define CLKCTRL_PLLGLOB_RST_SET_MSK		0x00000002U

#define CLKCTRL_PLLGLOB_REFCLKDIV(x)		(((x) & 0x00003F00) >> 8)
#define CLKCTRL_PLLGLOB_AREFCLKDIV(x)		(((x) & 0x00000F00) >> 8)
#define CLKCTRL_PLLGLOB_DREFCLKDIV(x)		(((x) & 0x00003000) >> 12)

#define CLKCTRL_VCOCALIB_HSCNT_SET(x)		(((x) << 0) & 0x000003FF)
#define CLKCTRL_VCOCALIB_MSCNT_SET(x)		(((x) << 16) & 0x00FF0000)

#define CLKCTRL_CLR_LOSTLOCK_BYPASS		0x20000000U
#define CLKCTRL_PLLC_DIV(x)			((x) & 0x7FF)
#define CLKCTRL_CTRL_SDMMC_CNT(x)		(((x) & 0x7FF) + 1)

#define CLKCTRL_CPU_ID_CORE0			0
#define CLKCTRL_CPU_ID_CORE1			1
#define CLKCTRL_CPU_ID_CORE2			2
#define CLKCTRL_CPU_ID_CORE3			3


#define CLKCTRL_MAINPLL_NOCDIV_L4MP_MASK	GENMASK(5, 4)
#define CLKCTRL_MAINPLL_NOCDIV_L4MP_OFFSET	4
#define GET_CLKCTRL_MAINPLL_NOCDIV_L4MP(x)	(((x) & CLKCTRL_MAINPLL_NOCDIV_L4MP_MASK) >> \
							CLKCTRL_MAINPLL_NOCDIV_L4MP_OFFSET)

#define CLKCTRL_MAINPLL_NOCDIV_L4SP_MASK	GENMASK(7, 6)
#define CLKCTRL_MAINPLL_NOCDIV_L4SP_OFFSET	6
#define GET_CLKCTRL_MAINPLL_NOCDIV_L4SP(x)	(((x) & CLKCTRL_MAINPLL_NOCDIV_L4SP_MASK) >> \
							CLKCTRL_MAINPLL_NOCDIV_L4SP_OFFSET)


#define CLKCTRL_MAINPLL_NOCDIV_SPHY_MASK	GENMASK(17, 16)
#define CLKCTRL_MAINPLL_NOCDIV_SPHY_OFFSET	16
#define GET_CLKCTRL_MAINPLL_NOCDIV_SPHY(x)	(((x) & CLKCTRL_MAINPLL_NOCDIV_SPHY_MASK) >> \
						CLKCTRL_MAINPLL_NOCDIV_SPHY_OFFSET)

#define CLKCTRL_MAINPLL_NOCDIV_L4SYSFREE_MASK	GENMASK(3, 2)
#define CLKCTRL_MAINPLL_NOCDIV_L4SYSFREE_OFFSET	2
#define GET_CLKCTRL_MAINPLL_NOCDIV_L4SYSFREE(x)	(((x) & CLKCTRL_MAINPLL_NOCDIV_L4SYSFREE_MASK) >> \
						CLKCTRL_MAINPLL_NOCDIV_L4SYSFREE_OFFSET)

/**
 *  @brief  Get MPU(Micro Processor Unit) clock value
 *
 *  @param  void
 *
 *  @return returns MPU clock value
 */
uint32_t get_mpu_clk(void);

/**
 *  @brief  Get Watchdog peripheral clock value
 *
 *  @param  void
 *
 *  @return returns Watchdog clock value
 */
uint32_t get_wdt_clk(void);

/**
 *  @brief  Get UART peripheral clock value
 *
 *  @param  void
 *
 *  @return returns UART clock value
 */
uint32_t get_uart_clk(void);

/**
 *  @brief  Get MMC peripheral clock value
 *
 *  @param  void
 *
 *  @return returns MMC clock value
 */
uint32_t get_sdmmc_clk(void);

/**
 *  @brief  Get Timer peripheral clock value
 *
 *  @param  void
 *
 *  @return returns Timer clock value
 */
uint32_t get_timer_clk(void);

/**
 *  @brief  Get QSPI peripheral clock value
 *
 *  @param  void
 *
 *  @return returns QSPI clock value
 */
uint32_t get_qspi_clk(void);

/**
 *  @brief  Get I2C peripheral clock value
 *
 *  @param  void
 *
 *  @return returns I2C clock value
 */
uint32_t get_i2c_clk(void);

/**
 *  @brief  Get I3C peripheral clock value
 *
 *  @param  void
 *
 *  @return returns I3C clock value
 */
uint32_t get_i3c_clk(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_AGILEX5_LL_H_ */

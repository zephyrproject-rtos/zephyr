/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"
#include "sysconf.h"


#define PLL_CLK_IN	(SYSCLK_DEFAULT_IOSC_HZ / 1000000)  /* PLL clock in */


#define sysconf_reg_ptr ((sysconf_reg_t *)(BASE_ADDR_SYSCONFIG))


typedef struct pll_conf {
	uint32_t fout;
	uint32_t pll;
} pll_conf_t;

#define PLL_CONF_VAL(n, m, od) \
	(((n) << PLLCON_BIT_OFFSET_N) | \
	((m) << (PLLCON_BIT_OFFSET_M)) | \
	((od) << PLLCON_BIT_OFFSET_OD))


/* the following configuration is based on Fin = 16 Mhz */
static const pll_conf_t pll_configuration[] = {
	{100, PLL_CONF_VAL(1, 25, 2)},  /* 100 Mhz */
	{50,  PLL_CONF_VAL(1, 25, 3)},  /* 50 Mhz */
	{150, PLL_CONF_VAL(4, 75, 1)},  /* 150 Mhz */
	{75,  PLL_CONF_VAL(4, 75, 2)},  /* 75 Mhz */
	{25,  PLL_CONF_VAL(2, 25, 3)},  /* 25 Mhz */
	{72,  PLL_CONF_VAL(8, 144, 2)}, /* 72 Mhz */
	{144, PLL_CONF_VAL(8, 144, 1)}, /* 144 Mhz */
};


/**
 * PLL Fout = Fin * M/ (N *n NO)
 *
 * Fref = Fin / N; Fvco = Fref * M Fout = Fvco / NO
 *
 *  N = input divider value (1, 2, 3 … 15)
 *  M = feedback divider value (4, 5, 6 … 16383)
 *  NO = output divider value (1, 2, 4, or 8)
 *
 *  1 Mhz <= Fref <= 50 Mhz
 *  200 Mhz <= Fvco <= 400 Mhz
 *
 */
void arc_iot_pll_conf_reg(uint32_t val)
{

	sysconf_reg_ptr->CLKSEL = CLKSEL_EXT_16M;
	/* 0x52000000 is not described in spec. */
	sysconf_reg_ptr->PLLCON = val | (0x52000000);

	sysconf_reg_ptr->PLLCON = val | (1 << PLLCON_BIT_OFFSET_PLLRST);
	sysconf_reg_ptr->PLLCON = val & (~(1 << PLLCON_BIT_OFFSET_PLLRST));

	while (!(sysconf_reg_ptr->PLLSTAT & (1 << PLLSTAT_BIT_OFFSET_PLLSTB))) {
		;
	}

	sysconf_reg_ptr->CLKSEL = CLKSEL_PLL;
	/* from AHB_CLK_DIVIDER, not from DVFSS&PMC */
	sysconf_reg_ptr->AHBCLKDIV_SEL |= 1;
	/* AHB clk divisor = 1 */
	sysconf_reg_ptr->AHBCLKDIV = 0x1;
}

int32_t arc_iot_pll_fout_config(uint32_t freq)
{
	uint32_t i;

	if (freq == PLL_CLK_IN) {
		sysconf_reg_ptr->CLKSEL = CLKSEL_EXT_16M;
	}

	for (i = 0U; i < ARRAY_SIZE(pll_configuration); i++) {
		if (pll_configuration[i].fout == freq) {
			break;
		}
	}

	if (i >= ARRAY_SIZE(pll_configuration)) {
		return -1;
	}

	/* config eflash clk, must be < 100 Mhz */
	if (freq > 100) {
		arc_iot_eflash_clk_div(2);
	} else {
		arc_iot_eflash_clk_div(1);
	}

	arc_iot_pll_conf_reg(pll_configuration[i].pll);

	return 0;
}

void arc_iot_ahb_clk_divisor(uint8_t div)
{
	sysconf_reg_ptr->AHBCLKDIV = div;
}

void arc_iot_ahb_clk_enable(uint8_t dev)
{
	if (dev > AHBCLKEN_BIT_SDIO) {
		return;
	}

	sysconf_reg_ptr->AHBCLKEN |= (1 << dev);
}

void arc_iot_ahb_clk_disable(uint8_t dev)
{
	if (dev > AHBCLKEN_BIT_SDIO) {
		return;
	}

	sysconf_reg_ptr->AHBCLKEN &= (~(1 << dev));
}

void arc_iot_apb_clk_divisor(uint8_t div)
{
	sysconf_reg_ptr->APBCLKDIV = div;
}

void arc_iot_apb_clk_enable(uint8_t dev)
{
	if (dev > APBCLKEN_BIT_I3C) {
		return;
	}

	sysconf_reg_ptr->APBCLKEN |= (1 << dev);
}

void arc_iot_apb_clk_disable(uint8_t dev)
{
	if (dev > APBCLKEN_BIT_I3C) {
		return;
	}

	sysconf_reg_ptr->APBCLKEN &= (~(1 << dev));
}

void arc_iot_dio_clk_divisor(uint8_t div)
{
	sysconf_reg_ptr->SDIO_REFCLK_DIV;
}

void arc_iot_spi_master_clk_divisor(uint8_t id, uint8_t div)
{
	if (id == SPI_MASTER_0) {
		sysconf_reg_ptr->SPI_MST_CLKDIV =
		    (sysconf_reg_ptr->SPI_MST_CLKDIV & 0xffffff00) | div;
	} else if (id == SPI_MASTER_1) {
		sysconf_reg_ptr->SPI_MST_CLKDIV =
		    (sysconf_reg_ptr->SPI_MST_CLKDIV & 0xffff00ff) | (div << 8);
	} else if (id == SPI_MASTER_2) {
		sysconf_reg_ptr->SPI_MST_CLKDIV =
		    (sysconf_reg_ptr->SPI_MST_CLKDIV & 0xff00ffff) | (div << 16);
	}
}

void arc_iot_gpio8b_dbclk_div(uint8_t bank, uint8_t div)
{
	if (bank == GPIO8B_BANK0) {
		sysconf_reg_ptr->GPIO8B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO8B_DBCLK_DIV & 0xffffff00) | div;
	} else if (bank == GPIO8B_BANK1) {
		sysconf_reg_ptr->GPIO8B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO8B_DBCLK_DIV & 0xffff00ff) | (div << 8);
	} else if (bank == GPIO8B_BANK2) {
		sysconf_reg_ptr->GPIO8B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO8B_DBCLK_DIV & 0xff00ffff) | (div << 16);
	} else if (bank == GPIO8B_BANK3) {
		sysconf_reg_ptr->GPIO8B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO8B_DBCLK_DIV & 0x00ffffff) | (div << 24);
	}
}

void arc_iot_gpio4b_dbclk_div(uint8_t bank, uint8_t div)
{
	if (bank == GPIO4B_BANK0) {
		sysconf_reg_ptr->GPIO4B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO4B_DBCLK_DIV & 0xffffff00) | div;
	} else if (bank == GPIO4B_BANK1) {
		sysconf_reg_ptr->GPIO4B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO4B_DBCLK_DIV & 0xffff00ff) | (div << 8);
	} else if (bank == GPIO4B_BANK2) {
		sysconf_reg_ptr->GPIO4B_DBCLK_DIV =
		    (sysconf_reg_ptr->GPIO4B_DBCLK_DIV & 0xff00ffff) | (div << 16);
	}
}

void arc_iot_i2s_tx_clk_div(uint8_t div)
{
	sysconf_reg_ptr->I2S_TX_SCLKDIV = div;
}

void arc_iot_i2s_rx_clk_div(uint8_t div)
{
	sysconf_reg_ptr->I2S_RX_SCLKDIV = div;
}

void arc_iot_i2s_rx_clk_sel(uint8_t sel)
{
	sysconf_reg_ptr->I2S_RX_SCLKSEL = sel;
}

void arc_iot_syscon_reset(void)
{
	sysconf_reg_ptr->RSTCON = 0x55AA6699;
}

uint32_t arc_iot_is_poweron_rst(void)
{
	if (sysconf_reg_ptr->RSTSTAT & SYS_RST_SOFTWARE_ON) {
		return 0;
	} else {
		return 1;
	}
}

void arc_iot_dvfs_clk_divisor(uint8_t level, uint8_t div)
{
	if (level == DVFS_PERF_LEVEL0) {
		sysconf_reg_ptr->DVFS_CLKDIV =
		    (sysconf_reg_ptr->DVFS_CLKDIV & 0xffffff00) | div;
	} else if (level == DVFS_PERF_LEVEL1) {
		sysconf_reg_ptr->DVFS_CLKDIV =
		    (sysconf_reg_ptr->DVFS_CLKDIV & 0xffff00ff) | (div << 8);
	} else if (level == DVFS_PERF_LEVEL2) {
		sysconf_reg_ptr->DVFS_CLKDIV =
		    (sysconf_reg_ptr->DVFS_CLKDIV & 0xff00ffff) | (div << 16);
	} else if (level == DVFS_PERF_LEVEL3) {
		sysconf_reg_ptr->DVFS_CLKDIV =
		    (sysconf_reg_ptr->DVFS_CLKDIV & 0x00ffffff) | (div << 24);
	}
}

void arc_iot_dvfs_vdd_config(uint8_t level, uint8_t val)
{
	val &= 0xf;

	if (level == DVFS_PERF_LEVEL0) {
		sysconf_reg_ptr->DVFS_VDDSET =
		    (sysconf_reg_ptr->DVFS_VDDSET & 0xfffffff0) | val;
	} else if (level == DVFS_PERF_LEVEL1) {
		sysconf_reg_ptr->DVFS_VDDSET =
		    (sysconf_reg_ptr->DVFS_VDDSET & 0xffffff0f) | (val << 4);
	} else if (level == DVFS_PERF_LEVEL2) {
		sysconf_reg_ptr->DVFS_VDDSET =
		    (sysconf_reg_ptr->DVFS_VDDSET & 0xfffff0ff) | (val << 8);
	} else if (level == DVFS_PERF_LEVEL3) {
		sysconf_reg_ptr->DVFS_CLKDIV =
		    (sysconf_reg_ptr->DVFS_CLKDIV & 0xffff0fff) | (val << 12);
	}
}

void arc_iot_dvfs_vwtime_config(uint8_t time)
{
	sysconf_reg_ptr->DVFS_VWTIME = time;
}

void arc_iot_pmc_pwwtime_config(uint8_t time)
{
	sysconf_reg_ptr->PMC_PUWTIME = time;
}

void arc_iot_uart3_clk_divisor(uint8_t div)
{
	sysconf_reg_ptr->UART3SCLK_DIV = div;
}

void arc_iot_reset_powerdown_vector(uint32_t addr)
{
	sysconf_reg_ptr->RESET_PD_VECTOR = addr;
}

void arc_iot_pwm_timer_pause(uint32_t id, uint32_t pause)
{
	uint32_t val = sysconf_reg_ptr->TIMER_PAUSE;

	if (id > PWM_TIMER5) {
		return;
	}

	if (pause) {
		val |= (1 << id);
	} else {
		val &= (~(1 << id));
	}

	sysconf_reg_ptr->TIMER_PAUSE = val;
}

void arc_iot_eflash_clk_div(uint8_t div)
{
	sysconf_reg_ptr->AHBCLKDIV |= (div << 8);
}

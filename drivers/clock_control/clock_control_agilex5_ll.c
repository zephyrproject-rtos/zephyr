/*
 * Copyright (c) 2022-2024, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/cpu.h>
#include <socfpga_system_manager.h>
#include <zephyr/sys/__assert.h>

#include "clock_control_agilex5_ll.h"

LOG_MODULE_REGISTER(clock_control_agilex5_ll, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Extract reference clock from platform clock source */
static uint32_t get_ref_clk(mm_reg_t pllglob_reg, mm_reg_t pllm_reg)
{
	uint32_t arefclkdiv = 0U;
	uint32_t ref_clk = 0U;
	uint32_t mdiv = 0U;
	uint32_t pllglob_val = 0U;
	uint32_t pllm_val = 0U;

	/* Read pllglob and pllm registers */
	pllglob_val = sys_read32(pllglob_reg);
	pllm_val = sys_read32(pllm_reg);

	/*
	 * Based on the clock source, read the values from System Manager boot
	 * scratch registers. These values are filled by boot loader based on
	 * hand-off data.
	 */
	switch (CLKCTRL_PSRC(pllglob_val)) {
	case CLKCTRL_PLLGLOB_PSRC_EOSC1:
		ref_clk = sys_read32(SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_1));
		break;

	case CLKCTRL_PLLGLOB_PSRC_INTOSC:
		ref_clk = CLKCTRL_INTOSC_HZ;
		break;

	case CLKCTRL_PLLGLOB_PSRC_F2S:
		ref_clk = sys_read32(SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_2));
		break;

	default:
		ref_clk = 0;
		__ASSERT(0, "Invalid input clock source");
		break;
	}

	/* Get reference clock divider */
	arefclkdiv = CLKCTRL_PLLGLOB_AREFCLKDIV(pllglob_val);
	__ASSERT(arefclkdiv != 0, "Reference clock divider is zero");
	ref_clk /= arefclkdiv;

	/* Feedback clock divider */
	mdiv = CLKCTRL_PLLM_MDIV(pllm_val);
	ref_clk *= mdiv;

	LOG_DBG("%s: ref_clk %u\n", __func__, ref_clk);

	return ref_clk;
}

/* Calculate clock frequency based on parameter */
static uint32_t get_clk_freq(mm_reg_t psrc_reg, mm_reg_t mainpllc_reg,
			     mm_reg_t perpllc_reg)
{
	uint32_t clock_val = 0U;
	uint32_t clk_psrc = 0U;
	uint32_t pllcx_div = 0U;

	/*
	 * Select source for the active 5:1 clock selection when the PLL
	 * is not bypassed
	 */
	clk_psrc = sys_read32(psrc_reg);
	switch (GET_CLKCTRL_CLKSRC(clk_psrc)) {
	case CLKCTRL_CLKSRC_MAIN:
		clock_val = get_ref_clk(CLKCTRL_MAINPLL(PLLGLOB), CLKCTRL_MAINPLL(PLLM));
		pllcx_div = (sys_read32(mainpllc_reg) & CLKCTRL_PLLCX_DIV_MSK);
		__ASSERT(pllcx_div != 0, "Main PLLC clock divider is zero");
		clock_val /= pllcx_div;
		break;

	case CLKCTRL_CLKSRC_PER:
		clock_val = get_ref_clk(CLKCTRL_PERPLL(PLLGLOB), CLKCTRL_PERPLL(PLLM));
		pllcx_div = (sys_read32(perpllc_reg) & CLKCTRL_PLLCX_DIV_MSK);
		__ASSERT(pllcx_div != 0, "Peripheral PLLC clock divider is zero");
		clock_val /= pllcx_div;
		break;

	case CLKCTRL_CLKSRC_OSC1:
		clock_val = sys_read32(SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_1));
		break;

	case CLKCTRL_CLKSRC_INTOSC:
		clock_val = CLKCTRL_INTOSC_HZ;
		break;

	case CLKCTRL_CLKSRC_FPGA:
		clock_val = sys_read32(SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_2));
		break;

	default:
		__ASSERT(0, "Invalid clock source select");
		break;
	}

	LOG_DBG("%s: clock source %lu and its value %u\n",
		__func__, GET_CLKCTRL_CLKSRC(clk_psrc), clock_val);

	return clock_val;
}

/* Get L3 free clock */
static uint32_t get_l3_main_free_clk(void)
{
	return get_clk_freq(CLKCTRL_MAINPLL(NOCCLK),
			    CLKCTRL_MAINPLL(PLLC3),
			    CLKCTRL_PERPLL(PLLC1));
}

/* Get L4 mp clock */
static uint32_t get_l4_mp_clk(void)
{
	uint32_t l3_main_free_clk = get_l3_main_free_clk();
	uint32_t mainpll_nocdiv_l4mp = BIT(GET_CLKCTRL_MAINPLL_NOCDIV_L4MP(
					sys_read32(CLKCTRL_MAINPLL(NOCDIV))));

	uint32_t l4_mp_clk = (l3_main_free_clk / mainpll_nocdiv_l4mp);

	return l4_mp_clk;
}

/*
 * Get L4 sp clock.
 * "l4_sp_clk" (100MHz) will be used for slow peripherals like UART, I2C,
 * Timers ...etc.
 */
static uint32_t get_l4_sp_clk(void)
{
	uint32_t l3_main_free_clk = get_l3_main_free_clk();
	uint32_t mainpll_nocdiv_l4sp = BIT(GET_CLKCTRL_MAINPLL_NOCDIV_L4SP(
					sys_read32(CLKCTRL_MAINPLL(NOCDIV))));

	uint32_t l4_sp_clk = (l3_main_free_clk / mainpll_nocdiv_l4sp);

	return l4_sp_clk;
}

/* Get MPU clock */
uint32_t get_mpu_clk(void)
{
	uint8_t cpu_id = arch_curr_cpu()->id;
	uint32_t ctr_reg = 0U;
	uint32_t clock_val = 0U;

	if (cpu_id > CLKCTRL_CPU_ID_CORE1) {
		clock_val = get_clk_freq(CLKCTRL_CTLGRP(CORE23CTR),
				     CLKCTRL_MAINPLL(PLLC0),
				     CLKCTRL_PERPLL(PLLC0));
	} else {
		clock_val = get_clk_freq(CLKCTRL_CTLGRP(CORE01CTR),
				     CLKCTRL_MAINPLL(PLLC1),
				     CLKCTRL_PERPLL(PLLC0));
	}

	switch (cpu_id) {
	case CLKCTRL_CPU_ID_CORE0:
	case CLKCTRL_CPU_ID_CORE1:
		ctr_reg = CLKCTRL_CTLGRP(CORE01CTR);
		break;

	case CLKCTRL_CPU_ID_CORE2:
		ctr_reg = CLKCTRL_CTLGRP(CORE2CTR);
		break;

	case CLKCTRL_CPU_ID_CORE3:
		ctr_reg = CLKCTRL_CTLGRP(CORE3CTR);
		break;

	default:
		break;
	}

	/* Division setting for ping pong counter in clock slice */
	clock_val /= 1 + (sys_read32(ctr_reg) & CLKCTRL_PLLCX_DIV_MSK);

	return clock_val;
}

/* Calculate clock frequency to be used for watchdog timer */
uint32_t get_wdt_clk(void)
{
	uint32_t l3_main_free_clk = get_l3_main_free_clk();
	uint32_t mainpll_nocdiv_l4sysfreeclk = BIT(GET_CLKCTRL_MAINPLL_NOCDIV_L4SYSFREE(
						sys_read32(CLKCTRL_MAINPLL(NOCDIV))));
	uint32_t l4_sys_free_clk = (l3_main_free_clk / mainpll_nocdiv_l4sysfreeclk);

	return l4_sys_free_clk;
}

/* Get clock frequency to be used for UART driver */
uint32_t get_uart_clk(void)
{
	return get_l4_sp_clk();
}

/* Calculate clock frequency to be used for SDMMC driver */
uint32_t get_sdmmc_clk(void)
{
	uint32_t l4_mp_clk = get_l4_mp_clk();
	uint32_t mainpll_nocdiv = sys_read32(CLKCTRL_MAINPLL(NOCDIV));
	uint32_t sdmmc_clk = l4_mp_clk / BIT(GET_CLKCTRL_MAINPLL_NOCDIV_SPHY(mainpll_nocdiv));

	return sdmmc_clk;
}

/* Calculate clock frequency to be used for Timer driver */
uint32_t get_timer_clk(void)
{
	return get_l4_sp_clk();
}

/* Calculate clock frequency to be used for QSPI driver */
uint32_t get_qspi_clk(void)
{
	uint32_t scr_reg, ref_clk;

	scr_reg = SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_0);
	ref_clk = sys_read32(scr_reg);

	/*
	 * In ATF, the qspi clock is divided by 1000 and loaded in scratch cold register 0
	 * So in Zephyr, reverting back the clock frequency by multiplying by 1000.
	 */
	return (ref_clk * 1000);
}

/* Calculate clock frequency to be used for I2C driver */
uint32_t get_i2c_clk(void)
{
	return get_l4_sp_clk();
}

/* Calculate clock frequency to be used for I3C driver */
uint32_t get_i3c_clk(void)
{
	return get_l4_mp_clk();
}

/*
 * Copyright (c) 2019-2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_agilex_ll.h>
#include <socfpga_system_manager.h>
#include <zephyr/kernel.h>

/* Clock manager individual group base addresses. */
struct clock_ll_params {
	mm_reg_t base_addr;
	mm_reg_t mainpll_addr;
	mm_reg_t peripll_addr;
	mm_reg_t ctl_addr;
};

/* Clock manager low layer(ll) params object. */
static struct clock_ll_params clock_ll;

/* Initialize the clock ll with the given base address */
void clock_agilex_ll_init(mm_reg_t base_addr)
{
	/* Clock manager module base address. */
	clock_ll.base_addr = base_addr;

	/* Clock manager main PLL base address. */
	clock_ll.mainpll_addr = clock_ll.base_addr + CLKMGR_MAINPLL_OFFSET;

	/* Clock manager peripheral PLL base address. */
	clock_ll.peripll_addr = clock_ll.base_addr + CLKMGR_PERPLL_OFFSET;

	/* Clock manager control module base address. */
	clock_ll.ctl_addr = clock_ll.base_addr + CLKMGR_INTEL_OFFSET;
}

/* Extract reference clock from platform clock source */
static uint32_t get_ref_clk(uint32_t pllglob)
{
	uint32_t arefclkdiv, ref_clk;
	uint32_t scr_reg;

	switch (CLKMGR_PSRC(pllglob)) {
	case CLKMGR_PLLGLOB_PSRC_EOSC1:
		scr_reg = SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_1);
		ref_clk = sys_read32(scr_reg);
		break;

	case CLKMGR_PLLGLOB_PSRC_INTOSC:
		ref_clk = CLKMGR_INTOSC_HZ;
		break;

	case CLKMGR_PLLGLOB_PSRC_F2S:
		scr_reg = SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_2);
		ref_clk = sys_read32(scr_reg);
		break;

	default:
		ref_clk = 0;
		break;
	}

	arefclkdiv = CLKMGR_PLLGLOB_AREFCLKDIV(pllglob);
	ref_clk /= arefclkdiv;

	return ref_clk;
}

/* Calculate clock frequency based on parameter */
static uint32_t get_clk_freq(uint32_t psrc_reg,
					uint32_t main_pllc,
					uint32_t per_pllc)
{
	uint32_t clk_psrc, mdiv, ref_clk;
	uint32_t pllm_reg, pllc_reg, pllc_div, pllglob_reg;

	clk_psrc = sys_read32(clock_ll.mainpll_addr + psrc_reg);

	switch (CLKMGR_PSRC(clk_psrc)) {
	case CLKMGR_PSRC_MAIN:
		pllm_reg = clock_ll.mainpll_addr + CLKMGR_MAINPLL_PLLM;
		pllc_reg = clock_ll.mainpll_addr + main_pllc;
		pllglob_reg = clock_ll.mainpll_addr + CLKMGR_MAINPLL_PLLGLOB;
		break;

	case CLKMGR_PSRC_PER:
		pllm_reg = clock_ll.peripll_addr + CLKMGR_PERPLL_PLLM;
		pllc_reg = clock_ll.peripll_addr + per_pllc;
		pllglob_reg = clock_ll.peripll_addr + CLKMGR_PERPLL_PLLGLOB;
		break;

	default:
		return 0;
	}

	ref_clk = get_ref_clk(sys_read32(pllglob_reg));
	mdiv = CLKMGR_PLLM_MDIV(sys_read32(pllm_reg));
	ref_clk *= mdiv;


	pllc_div = CLKMGR_PLLC_DIV(sys_read32(pllc_reg));

	return ref_clk / pllc_div;
}

/* Return L3 interconnect clock */
static uint32_t get_l3_clk(void)
{
	uint32_t l3_clk;

	l3_clk = get_clk_freq(CLKMGR_MAINPLL_NOCCLK,
						CLKMGR_MAINPLL_PLLC1,
						CLKMGR_PERPLL_PLLC1);

	return l3_clk;
}

/* Calculate clock frequency to be used for MPU */
uint32_t get_mpu_clk(void)
{
	uint32_t mpu_clk = 0;

	mpu_clk = get_clk_freq(CLKMGR_MAINPLL_MPUCLK,
						CLKMGR_MAINPLL_PLLC0,
						CLKMGR_PERPLL_PLLC0);

	return mpu_clk;
}

/* Calculate clock frequency to be used for watchdog timer */
uint32_t get_wdt_clk(void)
{
	uint32_t l3_clk, l4_sys_clk;

	l3_clk = get_l3_clk();
	l4_sys_clk = l3_clk / 4;

	return l4_sys_clk;
}

/* Calculate clock frequency to be used for UART driver */
uint32_t get_uart_clk(void)
{
	uint32_t reg, l3_clk, l4_sp_clk;

	l3_clk = get_l3_clk();

	reg = sys_read32(clock_ll.mainpll_addr + CLKMGR_MAINPLL_NOCDIV);
	reg = CLKMGR_MAINPLL_L4SPDIV(reg);

	l4_sp_clk = l3_clk >> reg;

	return l4_sp_clk;
}

/* Calculate clock frequency to be used for SDMMC driver */
uint32_t get_mmc_clk(void)
{
	uint32_t reg, mmc_clk;

	mmc_clk = get_clk_freq(CLKMGR_INTEL_SDMMCCTR,
						CLKMGR_MAINPLL_PLLC3,
						CLKMGR_PERPLL_PLLC3);

	reg = sys_read32(clock_ll.ctl_addr + CLKMGR_INTEL_SDMMCCTR);
	reg = CLKMGR_INTEL_SDMMC_CNT(reg);

	mmc_clk = (mmc_clk / reg) / 4;

	return mmc_clk;
}

/* Calculate clock frequency to be used for Timer driver */
uint32_t get_timer_clk(void)
{
	uint32_t l3_clk, l4_sys_clk;

	l3_clk = get_l3_clk();
	l4_sys_clk = l3_clk / 4;

	return l4_sys_clk;
}

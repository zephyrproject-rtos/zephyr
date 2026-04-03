/*
 * Copyright (c) 2019-2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/drivers/clock_control/clock_agilex_ll.h>
#include <socfpga_system_manager.h>

/*
 * Intel SoC re-use Arm Trusted Firmware (ATF) driver code in Zephyr.
 * The migrated ATF driver code uses mmio_X macro to access the register.
 * The following macros map mmio_X to Zephyr compatible function for
 * register access. This allow Zephyr to re-use the ATF driver codes
 * without massive changes.
 */
#define mmio_write_32(addr, data)	sys_write32((data), (addr))
#define mmio_read_32(addr)		sys_read32((addr))
#define mmio_setbits_32(addr, mask)	sys_set_bits((addr), (mask))
#define mmio_clrbits_32(addr, mask)	sys_clear_bits((addr), (mask))

/* Extract reference clock from platform clock source */
uint32_t get_ref_clk(uint32_t pllglob)
{
	uint32_t arefclkdiv, ref_clk;
	uint32_t scr_reg;

	switch (CLKMGR_PSRC(pllglob)) {
	case CLKMGR_PLLGLOB_PSRC_EOSC1:
		scr_reg = SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_1);
		ref_clk = mmio_read_32(scr_reg);
		break;
	case CLKMGR_PLLGLOB_PSRC_INTOSC:
		ref_clk = CLKMGR_INTOSC_HZ;
		break;
	case CLKMGR_PLLGLOB_PSRC_F2S:
		scr_reg = SOCFPGA_SYSMGR(BOOT_SCRATCH_COLD_2);
		ref_clk = mmio_read_32(scr_reg);
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
uint32_t get_clk_freq(uint32_t psrc_reg, uint32_t main_pllc, uint32_t per_pllc)
{
	uint32_t clk_psrc, mdiv, ref_clk;
	uint32_t pllm_reg, pllc_reg, pllc_div, pllglob_reg;

	clk_psrc = mmio_read_32(CLKMGR_MAINPLL + psrc_reg);

	switch (CLKMGR_PSRC(clk_psrc)) {
	case CLKMGR_PSRC_MAIN:
		pllm_reg = CLKMGR_MAINPLL + CLKMGR_MAINPLL_PLLM;
		pllc_reg = CLKMGR_MAINPLL + main_pllc;
		pllglob_reg = CLKMGR_MAINPLL + CLKMGR_MAINPLL_PLLGLOB;
		break;
	case CLKMGR_PSRC_PER:
		pllm_reg = CLKMGR_PERPLL + CLKMGR_PERPLL_PLLM;
		pllc_reg = CLKMGR_PERPLL + per_pllc;
		pllglob_reg = CLKMGR_PERPLL + CLKMGR_PERPLL_PLLGLOB;
		break;
	default:
		return 0;
	}

	ref_clk = get_ref_clk(mmio_read_32(pllglob_reg));
	mdiv = CLKMGR_PLLM_MDIV(mmio_read_32(pllm_reg));
	ref_clk *= mdiv;

	pllc_div = mmio_read_32(pllc_reg) & 0x7ff;

	return ref_clk / pllc_div;
}

/* Return L3 interconnect clock */
uint32_t get_l3_clk(void)
{
	uint32_t l3_clk;

	l3_clk = get_clk_freq(CLKMGR_MAINPLL_NOCCLK, CLKMGR_MAINPLL_PLLC1,
				CLKMGR_PERPLL_PLLC1);
	return l3_clk;
}

/* Calculate clock frequency to be used for mpu */
uint32_t get_mpu_clk(void)
{
	uint32_t mpu_clk = 0;

	mpu_clk = get_clk_freq(CLKMGR_MAINPLL_MPUCLK, CLKMGR_MAINPLL_PLLC0,
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
	uint32_t data32, l3_clk, l4_sp_clk;

	l3_clk = get_l3_clk();

	data32 = mmio_read_32(CLKMGR_MAINPLL + CLKMGR_MAINPLL_NOCDIV);
	data32 = (data32 >> 16) & 0x3;

	l4_sp_clk = l3_clk >> data32;

	return l4_sp_clk;
}

/* Calculate clock frequency to be used for SDMMC driver */
uint32_t get_mmc_clk(void)
{
	uint32_t data32, mmc_clk;

	mmc_clk = get_clk_freq(CLKMGR_ALTERA_SDMMCCTR,
		CLKMGR_MAINPLL_PLLC3, CLKMGR_PERPLL_PLLC3);

	data32 = mmio_read_32(CLKMGR_ALTERA + CLKMGR_ALTERA_SDMMCCTR);
	data32 = (data32 & 0x7ff) + 1;
	mmc_clk = (mmc_clk / data32) / 4;

	return mmc_clk;
}

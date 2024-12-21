/*
 * Copyright (c) 2021-2024 ATL Electronics
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab RISC-V MCU series initialization code
 */

#include <assert.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#include <soc.h>
#include <clic.h>

#include <bflb_soc.h>
#include <aon_reg.h>
#include <ef_ctrl_reg.h>
#include <extra_defines.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <l1c_reg.h>
#include <pds_reg.h>
#include <tzc_sec_reg.h>

/* clang-format off */

/* Set Embedded Flash Pullup */
static void system_bor_init(void)
{
	uint32_t tmp = 0;

	tmp = sys_read32(HBN_BASE + HBN_BOR_CFG_OFFSET);
	tmp = (tmp & HBN_BOR_VTH_UMSK)		/* borThreshold = 1 */
	    | (1U << HBN_BOR_VTH_POS);
	tmp = (tmp & HBN_BOR_SEL_UMSK)		/* enablePorInBor true */
	    | (1U << HBN_BOR_SEL_POS);
	tmp = (tmp & HBN_PU_BOR_UMSK)		/* enableBor true */
	    | (1U << HBN_PU_BOR_POS);
	sys_write32(tmp, HBN_BASE + HBN_BOR_CFG_OFFSET);

	/* enableBorInt false */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	tmp = tmp & HBN_IRQ_BOR_EN_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);
}

static uint32_t mtimer_get_clk_src_div(void)
{
	uint32_t bclk_div = -1;

	bclk_div = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	bclk_div = (bclk_div & GLB_REG_BCLK_DIV_MSK) >> GLB_REG_BCLK_DIV_POS;

	return ((sys_read32(CORECLOCKREGISTER) / (bclk_div + 1)) / 1000 / 1000 - 1);
}

static void system_clock_settle(void)
{
	__asm__ volatile (".rept 15 ; nop ; .endr");
}

static void system_clock_delay_32M_ms(uint32_t ms)
{
	uint32_t count = 0;

	do {
		__asm__ volatile (".rept 32 ; nop ; .endr");
		++count;
	} while (count < ms);
}

static uint32_t is_pds_busy(void)
{
	uint32_t tmp = 0;

	tmp = sys_read32(BFLB_EF_CTRL_BASE + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	if (tmp & EF_CTRL_EF_IF_0_BUSY_MSK) {
		return 1;
	}
	return 0;
}

/* /!\ only use when running on 32Mhz Oscillator Clock
 * (system_set_root_clock(0);
 * system_set_root_clock_dividers(0, 0);
 * sys_write32(MHZ(32), CORECLOCKREGISTER);)
 * Only Use with IRQs off
 * returns 0 when error
 */
static uint32_t system_efuse_read_32M(void)
{
	uint32_t tmp = 0;
	uint32_t tmp2 = 0;
	uint32_t *pefuse_start = (uint32_t *)(BFLB_EF_CTRL_BASE);
	uint32_t timeout = 0;

	do {
		system_clock_delay_32M_ms(1);
		timeout++;
	} while (timeout < EF_CTRL_DFT_TIMEOUT_VAL && is_pds_busy() > 0);

	/* do a 'ahb clock' setup */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
	    | (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
	    | (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
	    | (EF_CTRL_SAHB_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
	    | (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
	    | (0 << EF_CTRL_EF_IF_POR_DIG_POS)
	    | (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
	    | (0 << EF_CTRL_EF_IF_0_RW_POS)
	    | (0 << EF_CTRL_EF_IF_0_TRIG_POS);
	sys_write32(tmp, BFLB_EF_CTRL_BASE + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	system_clock_settle();

	/* clear PDS cache registry */
	for (uint32_t i = 0; i < EF_CTRL_EFUSE_R0_SIZE / 4; ++i) {
		pefuse_start[i] = 0;
	}

	/* Load efuse region0 */
	/* not ahb clock setup */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
	    | (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
	    | (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
	    | (EF_CTRL_EF_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
	    | (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
	    | (0 << EF_CTRL_EF_IF_POR_DIG_POS)
	    | (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
	    | (0 << EF_CTRL_EF_IF_0_RW_POS)
	    | (0 << EF_CTRL_EF_IF_0_TRIG_POS);
	sys_write32(tmp, BFLB_EF_CTRL_BASE + EF_CTRL_EF_IF_CTRL_0_OFFSET);

	/* trigger read */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
	    | (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
	    | (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
	    | (EF_CTRL_EF_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
	    | (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
	    | (0 << EF_CTRL_EF_IF_POR_DIG_POS)
	    | (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
	    | (0 << EF_CTRL_EF_IF_0_RW_POS)
	    | (1 << EF_CTRL_EF_IF_0_TRIG_POS);
	sys_write32(tmp, BFLB_EF_CTRL_BASE + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	system_clock_delay_32M_ms(5);

	/* wait for read to complete */
	do {
		system_clock_delay_32M_ms(1);
		tmp = sys_read32(BFLB_EF_CTRL_BASE + EF_CTRL_EF_IF_CTRL_0_OFFSET);
	} while ((tmp & EF_CTRL_EF_IF_0_BUSY_MSK)
	      || !(tmp && EF_CTRL_EF_IF_0_AUTOLOAD_DONE_MSK));

	/* do a 'ahb clock' setup */
	tmp =  EF_CTRL_EFUSE_CTRL_PROTECT
	    | (EF_CTRL_OP_MODE_AUTO << EF_CTRL_EF_IF_0_MANUAL_EN_POS)
	    | (EF_CTRL_PARA_DFT << EF_CTRL_EF_IF_0_CYC_MODIFY_POS)
	    | (EF_CTRL_SAHB_CLK << EF_CTRL_EF_CLK_SAHB_DATA_SEL_POS)
	    | (1 << EF_CTRL_EF_IF_AUTO_RD_EN_POS)
	    | (0 << EF_CTRL_EF_IF_POR_DIG_POS)
	    | (1 << EF_CTRL_EF_IF_0_INT_CLR_POS)
	    | (0 << EF_CTRL_EF_IF_0_RW_POS)
	    | (0 << EF_CTRL_EF_IF_0_TRIG_POS);
	sys_write32(tmp, BFLB_EF_CTRL_BASE + EF_CTRL_EF_IF_CTRL_0_OFFSET);

	/* get trim
	 * .name = "rc32m",
	 * .en_addr = 0x0C * 8 + 19,
	 * .parity_addr = 0x0C * 8 + 18,
	 * .value_addr = 0x0C * 8 + 10,
	 * .value_len = 8,
	 */
	tmp2 = 0x0c * 8 + 19;
	tmp = sys_read32(BFLB_EF_CTRL_BASE + (tmp2 / 32) * 4);
	if (!(tmp & (1 << (tmp2 % 32)))) {
		return 0;
	}

	tmp2 = 0x0C * 8 + 10;
	tmp = sys_read32(BFLB_EF_CTRL_BASE + (tmp2 / 32) * 4);
	tmp = tmp >> (tmp2 % 32);
	return tmp & ((1 << 8) - 1);
	/* TODO: check trim parity */
}

/* /!\ only use when running on 32Mhz Oscillator Clock
 */
static void system_clock_trim_32M(void)
{
	uint32_t tmp = 0;
	uint32_t trim = 0;

	trim    = system_efuse_read_32M();
	tmp = sys_read32(PDS_BASE + PDS_RC32M_CTRL0_OFFSET);
	tmp = (tmp & PDS_RC32M_EXT_CODE_EN_UMSK)
	    | 1 << PDS_RC32M_EXT_CODE_EN_POS;
	tmp = (tmp & PDS_RC32M_CODE_FR_EXT_UMSK)
	    | trim << PDS_RC32M_CODE_FR_EXT_POS;
	sys_write32(tmp, PDS_BASE + PDS_RC32M_CTRL0_OFFSET);

	system_clock_settle();
}

/* 32 Mhz Oscillator: 0
 * crystal: 1
 * PLL and 32M: 2
 * PLL and crystal: 3
 */
static void system_set_root_clock(uint32_t clock)
{
	uint32_t tmp = 0;

	/* invalid value, fallback to internal 32M */
	if (clock < 0 || clock > 3) {
		clock = 0;
	}

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp = (tmp & HBN_ROOT_CLK_SEL_UMSK)
	    | (clock << HBN_ROOT_CLK_SEL_POS);
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	system_clock_settle();
}

static void system_set_root_clock_dividers(uint32_t hclk_div, uint32_t bclk_div)
{
	uint32_t tmp = 0;

	/* set dividers */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_HCLK_DIV_UMSK)
	    | (hclk_div << GLB_REG_HCLK_DIV_POS);
	tmp = (tmp & GLB_REG_BCLK_DIV_UMSK)
	    | (bclk_div << GLB_REG_BCLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* do something undocumented, probably acknowledging clock change by
	 * disabling then reenabling bclk
	 */
	sys_write32(0x00000001, 0x40000FFC);
	sys_write32(0x00000000, 0x40000FFC);

	/* set core clock ? */
	sys_write32(sys_read32(CORECLOCKREGISTER) / (hclk_div + 1),
		    CORECLOCKREGISTER);

	system_clock_settle();

	/* enable clocks */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_EN_UMSK)
	    | (1U << GLB_REG_BCLK_EN_POS);
	tmp = (tmp & GLB_REG_HCLK_EN_UMSK)
	    | (1U << GLB_REG_HCLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	system_clock_settle();
}

/* HCLK: 0
 * PLL120M: 1
 */
static void system_set_PKA_clock(uint32_t pka_clock)
{
	uint32_t tmp = 0;

	tmp = sys_read32(GLB_BASE + GLB_SWRST_CFG2_OFFSET);
	tmp = (tmp & GLB_PKA_CLK_SEL_UMSK)
	    | (pka_clock << GLB_PKA_CLK_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_SWRST_CFG2_OFFSET);
}

static void system_set_machine_timer_clock_enable(uint32_t enable)
{
	uint32_t tmp = 0;

	if (enable) {
		enable = 1;
	}

	tmp = sys_read32(GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
	tmp = (tmp & GLB_CPU_RTC_EN_UMSK)
	    | (enable << GLB_CPU_RTC_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
}

/* clock:
 * 0: BCLK
 * 1: 32Khz Oscillator (RC32*K*)
 */
static void system_set_machine_timer_clock(uint32_t enable, uint32_t clock,
					   uint32_t divider)
{
	uint32_t tmp = 0;

	if (divider > 0x1FFFF) {
		divider = 0x1FFFF;
	}

	if (clock) {
		clock = 1;
	}

	/* disable mtime clock */
	system_set_machine_timer_clock_enable(0);

	tmp = sys_read32(GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);
	tmp = (tmp & GLB_CPU_RTC_SEL_UMSK)
	    | (clock << GLB_CPU_RTC_SEL_POS);
	tmp = (tmp & GLB_CPU_RTC_DIV_UMSK)
	    | (divider << GLB_CPU_RTC_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CPU_CLK_CFG_OFFSET);

	system_set_machine_timer_clock_enable(enable);
}

/* 0: RC32M
 * 1: Crystal
 */
static void system_setup_PLL_set_reference(uint32_t ref)
{
	uint32_t tmp = 0;

	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);
	if (ref > 0) {
		tmp = (tmp & PDS_CLKPLL_REFCLK_SEL_UMSK)
		    | (1U << PDS_CLKPLL_REFCLK_SEL_POS);
		tmp = (tmp & PDS_CLKPLL_XTAL_RC32M_SEL_UMSK)
		    | ((uint32_t)(0) << PDS_CLKPLL_XTAL_RC32M_SEL_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_REFCLK_SEL_UMSK)
		    | ((uint32_t)(0) << PDS_CLKPLL_REFCLK_SEL_POS);
		tmp = (tmp & PDS_CLKPLL_XTAL_RC32M_SEL_UMSK)
		    | (1U << PDS_CLKPLL_XTAL_RC32M_SEL_POS);
	}
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);
}

/* No Crystal: 0
 * 24M: 1
 * 26M: 2
 * 32M: 3
 * 38P4M: 4
 * 40M: 5
 * 32MHz Oscillator : 32
 */
static void system_setup_PLL(uint32_t crystal)
{
	uint32_t tmp = 0;

	/* TODO: if RC32M, use RC32M as PLL source */
	if (crystal == 32) {
		/* make sure we are on RC32M before trim */
		system_set_root_clock(0);
		system_set_root_clock_dividers(0, 0);
		sys_write32(MHZ(32), CORECLOCKREGISTER);

		/* Trim RC32M */
		system_clock_trim_32M();

		/* set PLL ref as RC32M */
		system_setup_PLL_set_reference(0);

	} else {
		/* PLL ref is crystal */
		system_setup_PLL_set_reference(1);
	}

	/* PLL Off */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_PU_CLKPLL_SFREG_UMSK)
	    | ((uint32_t)(0) << PDS_PU_CLKPLL_SFREG_POS);
	tmp = (tmp & PDS_PU_CLKPLL_UMSK)
	    | ((uint32_t)(0) << PDS_PU_CLKPLL_POS);
	 sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	 /* needs 2 steps ? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_PU_CP_UMSK)
	    | ((uint32_t)(0) << PDS_CLKPLL_PU_CP_POS);
	tmp = (tmp & PDS_CLKPLL_PU_PFD_UMSK)
	    | ((uint32_t)(0) << PDS_CLKPLL_PU_PFD_POS);
	tmp = (tmp & PDS_CLKPLL_PU_FBDV_UMSK)
	    | ((uint32_t)(0) << PDS_CLKPLL_PU_FBDV_POS);
	tmp = (tmp & PDS_CLKPLL_PU_POSTDIV_UMSK)
	    | ((uint32_t)(0) << PDS_CLKPLL_PU_POSTDIV_POS);
	 sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	/* set PLL Parameters */
	/* 26M special treatment */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_CP_OFFSET);
	if (crystal == 2) {
		tmp = (tmp & PDS_CLKPLL_ICP_1U_UMSK)
		    | (1U << PDS_CLKPLL_ICP_1U_POS);
		tmp = (tmp & PDS_CLKPLL_ICP_5U_UMSK)
		    | ((uint32_t)(0) << PDS_CLKPLL_ICP_5U_POS);
		tmp = (tmp & PDS_CLKPLL_INT_FRAC_SW_UMSK)
		    | (1U << PDS_CLKPLL_INT_FRAC_SW_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_ICP_1U_UMSK)
		    | ((uint32_t)(0) << PDS_CLKPLL_ICP_1U_POS);
		tmp = (tmp & PDS_CLKPLL_ICP_5U_UMSK)
		    | ((uint32_t)(2) << PDS_CLKPLL_ICP_5U_POS);
		tmp = (tmp & PDS_CLKPLL_INT_FRAC_SW_UMSK)
		    | ((uint32_t)(0) << PDS_CLKPLL_INT_FRAC_SW_POS);
	}
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_CP_OFFSET);

	/* More 26M special treatment */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_RZ_OFFSET);
	if (crystal == 2) {
		tmp = (tmp & PDS_CLKPLL_C3_UMSK)
		    | ((uint32_t)(2) << PDS_CLKPLL_C3_POS);
		tmp = (tmp & PDS_CLKPLL_CZ_UMSK)
		    | ((uint32_t)(2) << PDS_CLKPLL_CZ_POS);
		tmp = (tmp & PDS_CLKPLL_RZ_UMSK)
		    | ((uint32_t)(5) << PDS_CLKPLL_RZ_POS);
		tmp = (tmp & PDS_CLKPLL_R4_SHORT_UMSK)
		    | ((uint32_t)(0) << PDS_CLKPLL_R4_SHORT_POS);
	} else {
		tmp = (tmp & PDS_CLKPLL_C3_UMSK)
		    | ((uint32_t)(3) << PDS_CLKPLL_C3_POS);
		tmp = (tmp & PDS_CLKPLL_CZ_UMSK)
		    | (1U << PDS_CLKPLL_CZ_POS);
		tmp = (tmp & PDS_CLKPLL_RZ_UMSK)
		    | (1U << PDS_CLKPLL_RZ_POS);
		tmp = (tmp & PDS_CLKPLL_R4_SHORT_UMSK)
		    | (1U << PDS_CLKPLL_R4_SHORT_POS);
	}
	tmp = (tmp & PDS_CLKPLL_R4_UMSK)
	    | ((uint32_t)(2) << PDS_CLKPLL_R4_POS);
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_RZ_OFFSET);

	/* set pll dividers */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_POSTDIV_UMSK)
	    | ((uint32_t)(0x14) << PDS_CLKPLL_POSTDIV_POS);
	tmp = (tmp & PDS_CLKPLL_REFDIV_RATIO_UMSK)
	    | ((uint32_t)(2) << PDS_CLKPLL_REFDIV_RATIO_POS);
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_TOP_CTRL_OFFSET);

	/* set PLL SDMIN */
	/* Isn't this already set at boot by the rom settings
	 * and we can query the value?
	 */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_SDM_OFFSET);
	switch (crystal) {
	case 0:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x3C0000) << PDS_CLKPLL_SDMIN_POS);
		break;
	case 1:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x500000) << PDS_CLKPLL_SDMIN_POS);
		break;
	case 3:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x3C0000) << PDS_CLKPLL_SDMIN_POS);
		break;
	case 4:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x320000) << PDS_CLKPLL_SDMIN_POS);
		break;
	case 5:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x300000) << PDS_CLKPLL_SDMIN_POS);
		break;
	case 2:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x49D39D) << PDS_CLKPLL_SDMIN_POS);
		break;
	case 32:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x3C0000) << PDS_CLKPLL_SDMIN_POS);
		break;
	default:
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | ((uint32_t)(0x3C0000) << PDS_CLKPLL_SDMIN_POS);
		break;
	}
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_SDM_OFFSET);

	/* phase comparator settings? */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_FBDV_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SEL_FB_CLK_UMSK)
	    | (1U << PDS_CLKPLL_SEL_FB_CLK_POS);
	tmp = (tmp & PDS_CLKPLL_SEL_SAMPLE_CLK_UMSK)
	    | (1U << PDS_CLKPLL_SEL_SAMPLE_CLK_POS);
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_FBDV_OFFSET);

	 /* Turn PLL back ON */
	 /* frequency stabilization ? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_PU_CLKPLL_SFREG_UMSK)
	    | (1U << PDS_PU_CLKPLL_SFREG_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	 /* let settle for a while (5 us in SDK), we may not be
	  * running at 32Mhz right now
	  */
	system_clock_delay_32M_ms(2);

	 /* enable PPL clock actual? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_PU_CLKPLL_UMSK)
	    | (1U << PDS_PU_CLKPLL_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	/* More power up sequencing*/
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_PU_CP_UMSK)
	    | (1U << PDS_CLKPLL_PU_CP_POS);
	tmp = (tmp & PDS_CLKPLL_PU_PFD_UMSK)
	    | (1U << PDS_CLKPLL_PU_PFD_POS);
	tmp = (tmp & PDS_CLKPLL_PU_FBDV_UMSK)
	    | (1U << PDS_CLKPLL_PU_FBDV_POS);
	tmp = (tmp & PDS_CLKPLL_PU_POSTDIV_UMSK)
	    | (1U << PDS_CLKPLL_PU_POSTDIV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	system_clock_delay_32M_ms(2);

	/* reset couple things one by one? */
	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SDM_RESET_UMSK)
	    | (1U << PDS_CLKPLL_SDM_RESET_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_RESET_FBDV_UMSK)
	    | (1U << PDS_CLKPLL_RESET_FBDV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_RESET_FBDV_UMSK)
	    | ((uint32_t)(0) << PDS_CLKPLL_RESET_FBDV_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);

	tmp = sys_read32(PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SDM_RESET_UMSK)
	    | ((uint32_t)(0) << PDS_CLKPLL_SDM_RESET_POS);
	sys_write32(tmp, PDS_BASE + PDS_PU_RST_CLKPLL_OFFSET);
}

/* copied over from driver for convenience */
static uint32_t system_uart_bflb_get_crystal_frequency(void)
{
	uint32_t tmp;

	/* get clkpll_sdmin */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_SDM_OFFSET);
	tmp = (tmp & PDS_CLKPLL_SDMIN_MSK) >> PDS_CLKPLL_SDMIN_POS;

	/*
	 * The 'Sigma Delta Modulator Input' scales the PLL output relative to the
	 * input frequency (eg value is x2, output frequency doubles), it's a
	 * direct indicator of the XCLK (crystal or 32M osc) frequency to go in the
	 * PLL.
	 *
	 * This value can be read from efuse or from the firmware header which will
	 * be implemented in the future.
	 */
	switch (tmp) {
	case 0x500000:
		return MHZ(24);
	case 0x3C0000:
		return MHZ(32);
	case 0x320000:
		return MHZ(38.4);
	case 0x300000:
		return MHZ(40);
	case 0x49D39D:
		return MHZ(26);
	default:
		return MHZ(32);
	}
}

/* Frequency Source:
 * No Crystal: 0
 * 24M: 1
 * 26M: 2
 * 32M: 3
 * 38P4M: 4
 * 40M: 5
 * 32MHz Oscillator: 32
 *
 * /!\ When Frequency Source is 32M, we do not power crystal
 *
 * Clock Frequency:
 * Crystal: 0
 * PLL 48MHz: 1
 * PLL 120Mhz: 2
 * PLL 160Mhz: 3
 * PLL 192Mhz: 4
 * 32MHz Oscillator : 32
 *
 *  /!\ When Clock Frequency is 32M, we do not power crystal or PLL
 */
static void system_init_root_clock(uint32_t crystal,
				   uint32_t clock_frequency_source)
{
	uint32_t tmp = 0;
	uint32_t xtal_power_timeout = 0;


	/* make sure all clocks are enabled */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_BCLK_EN_UMSK)
	    | (1U << GLB_REG_BCLK_EN_POS);
	tmp = (tmp & GLB_REG_HCLK_EN_UMSK)
	    | (1U << GLB_REG_HCLK_EN_POS);
	tmp = (tmp & GLB_REG_FCLK_EN_UMSK)
	    | (1U << GLB_REG_FCLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* set clock to internal 32MHz Oscillator as failsafe */
	system_set_root_clock(0);
	system_set_root_clock_dividers(0, 0);
	sys_write32(MHZ(32), CORECLOCKREGISTER);

	system_set_PKA_clock(0);

	if (clock_frequency_source == 32) {
		return;
	}

	if (crystal != 32) {
		/* power crystal */
		tmp = sys_read32(AON_BASE + AON_RF_TOP_AON_OFFSET);
		tmp = (tmp & AON_PU_XTAL_AON_UMSK)
		    | (1U << AON_PU_XTAL_AON_POS);
		tmp = (tmp & AON_PU_XTAL_BUF_AON_UMSK)
		    | (1U << AON_PU_XTAL_BUF_AON_POS);
		sys_write32(tmp, AON_BASE + AON_RF_TOP_AON_OFFSET);

		/* wait for crystal to be powered on */
		/* TODO: figure way to communicate this failed */
		do {
			system_clock_delay_32M_ms(1);
			tmp = sys_read32(AON_BASE + AON_TSEN_OFFSET);
			xtal_power_timeout++;
		} while (!(tmp & AON_XTAL_RDY_MSK) && xtal_power_timeout < 120);
	}

	/* power PLL
	 * This code path only when PLL!
	 */
	system_setup_PLL(crystal);
	/* Big settle, 55us in SDK */
	system_clock_delay_32M_ms(10);

	/* enable all 'PDS' clocks */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_OUTPUT_EN_OFFSET);
	tmp |= 0x1FF;
	sys_write32(tmp, PDS_BASE + PDS_CLKPLL_OUTPUT_EN_OFFSET);

	/* glb enable pll actual? */
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_EN_UMSK)
	    | (1U << GLB_REG_PLL_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* set PLL to use in GLB*/
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG0_OFFSET);
	tmp = (tmp & GLB_REG_PLL_SEL_UMSK)
	    | ((uint32_t)(clock_frequency_source - 1) << GLB_REG_PLL_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG0_OFFSET);

	/* set root clock settings */
	switch (clock_frequency_source) {
	case 32:
		__ASSERT(clock_frequency_source == 32, "not supposed to be here, returned earlier");
		break;
	case 0:
		system_set_root_clock(1);
		sys_write32(system_uart_bflb_get_crystal_frequency(), CORECLOCKREGISTER);
		break;
	case 1:
		system_set_root_clock(crystal == 32 ? 2 : 3);
		sys_write32(MHZ(48), CORECLOCKREGISTER);
		break;
	case 2:
		system_set_root_clock_dividers(0, 1);
		system_set_root_clock(crystal == 32 ? 2 : 3);
		sys_write32(MHZ(120), CORECLOCKREGISTER);
		break;
	case 3:
		/* TODO: enable rom access 2T*/
		system_set_root_clock_dividers(0, 1);
		system_set_root_clock(crystal == 32 ? 2 : 3);
		sys_write32(MHZ(160), CORECLOCKREGISTER);
		break;
	case 4:
		/* TODO: enable rom access 2T*/
		system_set_root_clock_dividers(0, 1);
		system_set_root_clock(crystal == 32 ? 2 : 3);
		sys_write32(MHZ(192), CORECLOCKREGISTER);
		break;
	default:
		__ASSERT(false, "Invalid root clock frequency source");
		break;
	}

	/* settle */
	system_clock_delay_32M_ms(10);

	/* we have PLL now */
	system_set_PKA_clock(1);
}

static void uart_set_clock_enable(uint32_t enable)
{
	uint32_t tmp = 0;

	if (enable) {
		enable = 1;
	}

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp = (tmp & GLB_UART_CLK_EN_UMSK)
	    | (enable << GLB_UART_CLK_EN_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG2_OFFSET);
}

/* Clock:
 * FCLK: 0
 * 160 Mhz PLL: 1
 * When using PLL root clock, we can use either setting, when using the 32Mhz
 * Oscillator with a uninitialized PLL, only FCLK will be available.
 */
static void uart_set_clock(uint32_t enable, uint32_t clock, uint32_t divider)
{
	uint32_t tmp = 0;

	if (divider > 0x7) {
		divider = 0x7;
	}

	if (clock) {
		clock = 1;
	}
	/* disable uart clock */
	uart_set_clock_enable(0);

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	tmp = (tmp & GLB_UART_CLK_DIV_UMSK)
	    | (divider << GLB_UART_CLK_DIV_POS);
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG2_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_GLB_OFFSET);
	tmp = (tmp & HBN_UART_CLK_SEL_UMSK)
	    | (clock << HBN_UART_CLK_SEL_POS);
	sys_write32(tmp, HBN_BASE + HBN_GLB_OFFSET);

	uart_set_clock_enable(enable);
}


/* TODO: should take crystal type, defaults to 40Mhz crystal for BL602 as seems
 * the most common
 */
#define USE_CRYSTALL_32MHZ 0
static void system_clock_init(void)
{
#if USE_CRYSTALL_32MHZ
	system_init_root_clock(0, 32);
	system_set_root_clock_dividers(0, 0);
	system_clock_trim_32M();
#else
	system_init_root_clock(5, 4);
	system_set_root_clock_dividers(0, 2);
#endif
	system_set_machine_timer_clock(1, 0, mtimer_get_clk_src_div());
}

static void peripheral_clock_init(void)
{
	uint32_t regval = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);
	/* enable UART0 clock routing */
	regval |= (1 << 16);
	/* enable I2C0 clock routing */
	regval |= (1 << 19);
	sys_write32(regval, GLB_BASE + GLB_CGEN_CFG1_OFFSET);
	uart_set_clock(1, 1, 0);
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	uint32_t key;
	uint32_t *p;
	uint32_t i = 0;
	uint32_t tmp = 0;

	key = irq_lock();

	/* disable hardware_pullup_pull_down (reg_en_hw_pu_pd = 0) */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	/* "BL_CLR_REG_BIT" */
	tmp = tmp & HBN_REG_EN_HW_PU_PD_UMSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	/* 'seam' 0kb, undocumented */
	tmp = sys_read32(GLB_BASE + GLB_SEAM_MISC_OFFSET);
	tmp = (tmp & GLB_EM_SEL_UMSK)
	    | ((uint32_t)(0) << GLB_EM_SEL_POS);
	sys_write32(tmp, GLB_BASE + GLB_SEAM_MISC_OFFSET);

	/* Fix 26M xtal clkpll_sdmin */
	tmp = sys_read32(PDS_BASE + PDS_CLKPLL_SDM_OFFSET);

	if ((tmp & PDS_CLKPLL_SDMIN_MSK) == 0x49D39D) {
		tmp = (tmp & PDS_CLKPLL_SDMIN_UMSK)
		    | (uint32_t)(0x49D89E);
		sys_write32(tmp, PDS_BASE + PDS_CLKPLL_SDM_OFFSET);
	}

	tmp = sys_read32(GLB_BASE + GLB_PARM_OFFSET);
	/* GLB_UART_Sig_Swap_Set(UART_SIG_SWAP_NONE);
	 * no swap = 0
	 * see bl602_glb.h for other possible values
	 */
	tmp = (tmp & GLB_UART_SWAP_SET_UMSK)
	    | ((uint32_t)(0) << GLB_UART_SWAP_SET_POS);
	/* GLB_JTAG_Sig_Swap_Set(JTAG_SIG_SWAP_NONE); */
	tmp = (tmp & GLB_JTAG_SWAP_SET_UMSK)
	    | ((uint32_t)(0) << GLB_JTAG_SWAP_SET_POS);
	sys_write32(tmp, GLB_BASE + GLB_PARM_OFFSET);

	/* CLear all interrupt */
	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIE);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	p = (uint32_t *)(CLIC_HART0_ADDR + CLIC_INTIP);

	for (i = 0; i < (IRQn_LAST + 3) / 4; i++) {
		p[i] = 0;
	}

	/* init bor for all platform */
	system_bor_init();

	system_clock_init();
	peripheral_clock_init();

	irq_unlock(key);

	/* wait 10 ms for peripherals to be ready */
	k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(10));

	while (!sys_timepoint_expired(end_timeout)) {
	}
}

/* clang-format on */

/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 2025 ispace, inc.
 *
 * These functions are adapted from TI Halcogen generated code.
 */

/*
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com
*
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include "soc_internal.h"
#include "soc_defaults.h"

#define DRV_SYS   			DT_NODELABEL(sys)
#define DRV_DCC				DT_REG_ADDR_BY_IDX(DRV_SYS, 9)

#define ESM_SR1_PLL1SLIP		0x400U
#define ESM_SR4_PLL2SLIP 		0x400U

#define dcc1CNT1_CLKSRC_PLL1 		0x0000A000U
#define dcc1CNT1_CLKSRC_PLL2 		0x0000A001U

enum dcc1clocksource {
	DCC1_CNT0_HF_LPO	= 0x5U,    /**< Alias for DCC1 CNT 0 CLOCK SOURCE 0 */
	DCC1_CNT0_TCK		= 0xAU,    /**< Alias for DCC1 CNT 0 CLOCK SOURCE 1 */
	DCC1_CNT0_OSCIN		= 0xFU,    /**< Alias for DCC1 CNT 0 CLOCK SOURCE 2 */

	DCC1_CNT1_PLL1		= 0x0U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 0 */
	DCC1_CNT1_PLL2		= 0x1U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 1 */
	DCC1_CNT1_LF_LPO	= 0x2U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 2 */
	DCC1_CNT1_HF_LPO	= 0x3U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 3 */
	DCC1_CNT1_EXTCLKIN1	= 0x5U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 4 */
	DCC1_CNT1_EXTCLKIN2	= 0x6U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 6 */
	DCC1_CNT1_VCLK		= 0x8U,    /**< Alias for DCC1 CNT 1 CLOCK SOURCE 8 */
	DCC1_CNT1_N2HET1_31	= 0xAU     /**< Alias for DCC1 CNT 1 CLOCK SOURCE 9 */
};

#define REG_DCC_GCTRL		(DRV_DCC + 0x0000)  /**< 0x0000: DCC Control Register */
#define REG_DCC_REV		(DRV_DCC + 0x0004)  /**< 0x0004: DCC Revision Id Register */
#define REG_DCC_CNT0SEED	(DRV_DCC + 0x0008)  /**< 0x0008: DCC Counter0 Seed Register */
#define REG_DCC_VALID0SEED	(DRV_DCC + 0x000C)  /**< 0x000C: DCC Valid0 Seed Register */
#define REG_DCC_CNT1SEED	(DRV_DCC + 0x0010)  /**< 0x0010: DCC Counter1 Seed Register */
#define REG_DCC_STAT 		(DRV_DCC + 0x0014)  /**< 0x0014: DCC Status Register */
#define REG_DCC_CNT0		(DRV_DCC + 0x0018)  /**< 0x0018: DCC Counter0 Value Register */
#define REG_DCC_VALID0		(DRV_DCC + 0x001C)  /**< 0x001C: DCC Valid0 Value Register */
#define REG_DCC_CNT1		(DRV_DCC + 0x0020)  /**< 0x0020: DCC Counter1 Value Register */
#define REG_DCC_CNT1CLKSRC	(DRV_DCC + 0x0024)  /**< 0x0024: DCC Counter1 Clock Source Selection Register */
#define REG_DCC_CNT0CLKSRC	(DRV_DCC + 0x0028)  /**< 0x0028: DCC Counter0 Clock Source Selection Register */

static uint32_t disable_plls(uint32_t plls)
{
	uint32_t timeout, failCode;

	sys_write32(plls, REG_SYS1_CSDISSET);
	failCode = 0U;
	timeout = 0x10U;
	timeout--;

	while (((sys_read32(REG_SYS1_CSVSTAT) & (plls)) != 0U) && (timeout != 0U)) {
		/* Clear ESM and GLBSTAT PLL slip flags */
		sys_write32(0x00000300U, REG_SYS1_GBLSTAT);

		if ((plls & CSDIS_SRC_PLL1) == CSDIS_SRC_PLL1) {
			sys_write32(ESM_SR1_PLL1SLIP, REG_ESM_SR1_0);
		}
		if ((plls & CSDIS_SRC_PLL2) == CSDIS_SRC_PLL2) {
			sys_write32(ESM_SR4_PLL2SLIP, REG_ESM_SR4_0);
		}

		timeout--;
		/* Wait */
	}

	if (timeout == 0U) {
		failCode = 4U;
	} else {
		failCode = 0U;
	}

	return failCode;
}

static uint32_t check_frequency(uint32_t cnt1_clksrc)
{
	uint32_t val;

	/* Setup DCC1 */
	/** DCC1 Global Control register configuration */
	val = (uint32_t)0x5U |			 /** Disable  DCC1 */
	      (uint32_t)((uint32_t)0x5U << 4U) | /** No Error Interrupt */
	      (uint32_t)((uint32_t)0xAU << 8U) | /** Single Shot mode */
	      (uint32_t)((uint32_t)0x5U << 12U); /** No Done Interrupt */
	sys_write32(val, REG_DCC_GCTRL);

	/* Clear ERR and DONE bits */
	sys_write32(3, REG_DCC_STAT);

	/** DCC1 Clock0 Counter Seed value configuration */
	sys_write32(68U, REG_DCC_CNT0SEED);

	/** DCC1 Clock0 Valid Counter Seed value configuration */
	sys_write32(4U, REG_DCC_VALID0SEED);

	/** DCC1 Clock1 Counter Seed value configuration */
	sys_write32(972U, REG_DCC_CNT1SEED);

	/** DCC1 Clock1 Source 1 Select */
	val = (uint32_t)((uint32_t)10U << 12U) |	/** DCC Enable / Disable Key */
	      (uint32_t) cnt1_clksrc;			/** DCC1 Clock Source 1 */
	sys_write32(val, REG_DCC_CNT1CLKSRC);

	val = (uint32_t)DCC1_CNT0_OSCIN;		/** DCC1 Clock Source 0 */
	sys_write32(val, REG_DCC_CNT0CLKSRC);

	/** DCC1 Global Control register configuration */
	val = (uint32_t)0xAU |       /** Enable  DCC1 */
	      (uint32_t)((uint32_t)0x5U << 4U) | /** No Error Interrupt */
	      (uint32_t)((uint32_t)0xAU << 8U) | /** Single Shot mode */
	      (uint32_t)((uint32_t)0x5U << 12U); /** No Done Interrupt */
	sys_write32(val, REG_DCC_GCTRL);

	while (sys_read32(REG_DCC_STAT) == 0U) {
		/* Wait */
	}

	return (sys_read32(REG_DCC_STAT) & 0x01U);
}

uint32_t _errata_SSWF021_45_both_plls(uint32_t count)
{
	uint32_t fail_code, retries, clk_cntl_sav;

	clk_cntl_sav = sys_read32(REG_SYS1_CLKCNTL);

	/* First set VCLK2 = HCLK */
	sys_write32(clk_cntl_sav & 0x000F0100U, REG_SYS1_CLKCNTL);
	/* Now set VCLK = HCLK and enable peripherals */
	sys_write32(CLKCNTL_PENA, REG_SYS1_CLKCNTL);

	fail_code = 0U;

	for (retries = 0U; retries < count; retries++) {
		fail_code = 0U;

		/* Disable PLL1 and PLL2 */
		fail_code = disable_plls(CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2);

		if (fail_code != 0U) {
			break;
		}

		/* Clear Global Status Register */
		sys_write32(0x00000301U, REG_SYS1_GBLSTAT);
		/* Clear the ESM PLL slip flags */
		sys_write32(ESM_SR1_PLL1SLIP, REG_ESM_SR1_0);
		sys_write32(ESM_SR4_PLL2SLIP, REG_ESM_SR4_0);
		/* set both PLLs to OSCIN/1*27/(2*1) */
		sys_write32(0x20001A00U, REG_SYS1_PLLCTL1);
		sys_write32(0x3FC0723DU, REG_SYS1_PLLCTL2);
		sys_write32(0x20001A00U, REG_SYS2_PLLCTL3);
		sys_write32(CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2, REG_SYS1_CSDISCLR);

		/* Check for (PLL1 valid or PLL1 slip) and (PLL2 valid or PLL2 slip) */
		while  ((((sys_read32(REG_SYS1_CSVSTAT) & CSDIS_SRC_PLL1) == 0U) && ((sys_read32(REG_ESM_SR1_0) & ESM_SR1_PLL1SLIP) == 0U)) ||
			(((sys_read32(REG_SYS1_CSVSTAT) & CSDIS_SRC_PLL2) == 0U) && ((sys_read32(REG_ESM_SR4_0) & ESM_SR4_PLL2SLIP) == 0U))) {
			/* Wait */
		}

		/* If PLL1 valid, check the frequency */
		if (((sys_read32(REG_ESM_SR1_0) & ESM_SR1_PLL1SLIP) != 0U) || ((sys_read32(REG_SYS1_GBLSTAT) & 0x00000300U) != 0U)) {
			fail_code |= 1U;
		} else {
			fail_code |= check_frequency(dcc1CNT1_CLKSRC_PLL1);
		}

		/* If PLL2 valid, check the frequency */
		if (((sys_read32(REG_ESM_SR4_0) & ESM_SR4_PLL2SLIP) != 0U) || ((sys_read32(REG_SYS1_GBLSTAT) & 0x00000300U) != 0U)) {
			fail_code |= 2U;
		} else {
			fail_code |= (check_frequency(dcc1CNT1_CLKSRC_PLL2) << 1U);
		}

		if (fail_code == 0U) {
			break;
		}
	}

	/* To avoid MISRA violation 382S (void)missing for discarded return value */
	fail_code = disable_plls(CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2);
	/* restore CLKCNTL, VCLKR and PENA first */
	sys_write32(clk_cntl_sav & 0x000F0100U, REG_SYS1_CLKCNTL);
	/* restore CLKCNTL, VCLK2R */
	sys_write32(clk_cntl_sav, REG_SYS1_CLKCNTL);

	return fail_code;
}

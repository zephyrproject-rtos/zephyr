/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Copyright (c) 2025 ispace, inc.
 * 
 * Some portions adapted from TI Halcogen generated code.
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

#define DRV_SYS		DT_NODELABEL(sys)

#define DRV_SYS1	DT_REG_ADDR_BY_IDX(DRV_SYS, 0)
#define DRV_SYS2	DT_REG_ADDR_BY_IDX(DRV_SYS, 1)
#define DRV_PCR1	DT_REG_ADDR_BY_IDX(DRV_SYS, 2)
#define DRV_PCR2	DT_REG_ADDR_BY_IDX(DRV_SYS, 3)
#define DRV_PCR3	DT_REG_ADDR_BY_IDX(DRV_SYS, 4)
#define DRV_FCR		DT_REG_ADDR_BY_IDX(DRV_SYS, 5)

#ifndef CONFIG_PLATFORM_SPECIFIC_INIT
#warning CONFIG_PLATFORM_SPECIFIC_INIT not define, device initialization will not be done
#endif

/**
 * TODO list
 * - MPU
 */
void system_init(void)
{
	uint32_t _csvstat, _csdis, _clkcntl;

	/* disable PLL1 and PLL2 */
	sys_write32(CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2, REG_SYS1_CSDISSET);
	while ((sys_read32(REG_SYS1_CSDIS) & (CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2))
		!= (CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2)) {
		/* wait */
	}

	sys_write32((GLBSTAT_OSCFAIL | GLBSTAT_RFSLIP | GLBSTAT_FBSLIP),
		    REG_SYS1_GBLSTAT);
	sys_write32(PLLCTL1_INIT_VALUE, REG_SYS1_PLLCTL1);
	sys_write32(PLLCTL2_INIT_VALUE, REG_SYS1_PLLCTL2);
	sys_write32(PLLCTL3_INIT_VALUE, REG_SYS2_PLLCTL3);

	/* Enable PLL(s) to start up or Lock */
	sys_write32((~(CSDIS_SRC_OSC | CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2 |
			CSDIS_SRC_LFLPO | CSDIS_SRC_HFLPO)) & CSDIS_SRC_MASK,
			REG_SYS1_CSDIS);
	sys_write32(CSDIS_SRC_PLL1 | CSDIS_SRC_PLL2, REG_SYS1_CSDISCLR);

	/* Disable Peripherals before peripheral powerup */
	sys_write32(sys_read32(REG_SYS1_CLKCNTL) & ~CLKCNTL_PENA,
	REG_SYS1_CLKCNTL);

	/* Release peripherals from reset, enable clocks to all peripherals */
	/* Power-up all peripherals */
	sys_write32(0xffffffff, REG_PCR1_PSPWRDWNCLR0);
	sys_write32(0xffffffff, REG_PCR1_PSPWRDWNCLR1);
	sys_write32(0xffffffff, REG_PCR1_PSPWRDWNCLR2);
	sys_write32(0xffffffff, REG_PCR1_PSPWRDWNCLR3);

	sys_write32(0xffffffff, REG_PCR2_PSPWRDWNCLR0);
	sys_write32(0xffffffff, REG_PCR2_PSPWRDWNCLR1);
	sys_write32(0xffffffff, REG_PCR2_PSPWRDWNCLR2);
	sys_write32(0xffffffff, REG_PCR2_PSPWRDWNCLR3);

	sys_write32(0xffffffff, REG_PCR3_PSPWRDWNCLR0);
	sys_write32(0xffffffff, REG_PCR3_PSPWRDWNCLR1);
	sys_write32(0xffffffff, REG_PCR3_PSPWRDWNCLR2);
	sys_write32(0xffffffff, REG_PCR3_PSPWRDWNCLR3);

	/* Enable Peripherals */
	sys_write32(sys_read32(REG_SYS1_CLKCNTL) | CLKCNTL_PERIPHENA,
			REG_SYS1_CLKCNTL);

	/* Setup flash read mode, address wait states and data wait states */
	sys_write32(FRDCNTL_INIT_VALUE, REG_FCR_FRDCNTL);
	sys_write32(FSM_WR_ENA_ENABLE_VAL, REG_FCR_FSM_WR_ENA);
	sys_write32(EEPROM_CONFIG_INIT_VALUE, REG_FCR_EEPROM_CONFIG);
	sys_write32(FSM_WR_ENA_DISABLE_VAL, REG_FCR_FSM_WR_ENA);
	/** - Setup flash bank power modes */
	sys_write32(FBPWRMODE_INIT_VALUE, REG_FCR_FBPWRMODE);

	/* Initialize Clock Tree */
	/* Setup system clock divider for HCLK */
	sys_write32(1 << HCLKCNTL_HCLKR_OFFSET, REG_SYS2_HCLKCNTL);
	/* Disable / Enable clock domain */
	sys_write32(CDDIS_VCLKA2, REG_SYS1_CDDIS);
	/* Wait for until clocks are locked */
	do {
		_csvstat = sys_read32(REG_SYS1_CSVSTAT);
		_csdis = sys_read32(REG_SYS1_CSDIS);
	} while ((_csvstat & ((_csdis ^ 0xFF) & 0xFF))
		!= ((_csdis ^ 0xFFU) & 0xFFU));

	/* All clock domains are working off the default clock sources until now */
	/* Setup GCLK, HCLK and VCLK clock source for normal operation, power down mode and after wakeup */
	sys_write32(GHVSRC_INIT_VALUE, REG_SYS1_GHVSRC);
	/* Setup RTICLK1 */
	sys_write32(RCLKSRC_INIT_VALUE, REG_SYS1_RCLKSRC);
	/* Setup asynchronous peripheral clock sources for AVCLK1 and AVCLK2 */
	sys_write32(VCLKASRC_INIT_VALUE, REG_SYS1_VCLKASRC);

	/* Setup synchronous peripheral clock dividers for VCLK1, VCLK2, VCLK3 */
	/**
	 * Please check the note about VCLK and VCLK2 clock ratio
	 * restrictions in TRM. The default value is the same as what we are
	 * setting it to, but the order of setting matters, and both cannot be
	 * set together.
	 */
	_clkcntl = sys_read32(REG_SYS1_CLKCNTL);
	sys_write32((1 << CLKCNTL_VCLKR_OFFSET) | (_clkcntl & ~CLKCNTL_VCLK2R_MASK), REG_SYS1_CLKCNTL);
	sys_write32((1 << CLKCNTL_VCLK2R_OFFSET) | (_clkcntl & ~CLKCNTL_VCLKR_MASK), REG_SYS1_CLKCNTL);
	sys_write32((1 << CLK2CNTRL_VCLK3R_OFFSET) | (sys_read32(REG_SYS2_CLK2CNTRL) & ~CLK2CNTRL_VCLK3R_MASK), REG_SYS2_CLK2CNTRL);
	sys_write32(VCLKACON1_INIT_VALUE, REG_SYS2_VCLKACON1);

	/* Now the PLLs are locked and the PLL outputs can be sped up */
	/* The R-divider was programmed to be 0xF. Now this divider is changed to programmed value */
	sys_write32((1 << PLLCTL1_PLLDIV_OFFSET) | (sys_read32(REG_SYS1_PLLCTL1) & PLLCTL1_PLLDIV_MASK), REG_SYS1_PLLCTL1);
	sys_write32((0 << PLLCTL3_PLLDIV2_OFFSET) | (sys_read32(REG_SYS2_PLLCTL3) & PLLCTL3_PLLDIV2_MASK), REG_SYS2_PLLCTL3);
}

resetSource_t get_reset_source(void)
{
	resetSource_t rst_source;

	if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)POWERON_RESET) != 0U) {
		/* power-on reset condition */
		rst_source = POWERON_RESET;
		/* Clear all exception status Flag and proceed since it's power up */
		sys_write32(0x0000FFFFU, REG_SYS_EXCEPTION);
	}

	else if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)EXT_RESET) != 0U) {
		sys_write32((uint32_t)EXT_RESET, REG_SYS_EXCEPTION);

		/*** Check for other causes of EXT_RESET that would take precedence **/
		if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)OSC_FAILURE_RESET) != 0U) {
			/* Reset caused due to oscillator failure. Add user code here to handle oscillator failure */
			rst_source = OSC_FAILURE_RESET;
			sys_write32((uint32_t)OSC_FAILURE_RESET, REG_SYS_EXCEPTION);
		} else if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)WATCHDOG_RESET) != 0U) {
			/* Reset caused due watchdog violation */
			rst_source = WATCHDOG_RESET;
			sys_write32((uint32_t)WATCHDOG_RESET, REG_SYS_EXCEPTION);
		} else if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)WATCHDOG2_RESET) != 0U) {
			/* Reset caused due watchdog violation */
			rst_source = WATCHDOG2_RESET;
			sys_write32((uint32_t)WATCHDOG2_RESET, REG_SYS_EXCEPTION);
		} else if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)SW_RESET) != 0U) {
			/* Reset caused due to software reset. */
			rst_source = SW_RESET;
			sys_write32((uint32_t)SW_RESET, REG_SYS_EXCEPTION);
		} else {
			/* Reset caused due to External reset. */
			rst_source = EXT_RESET;
		}
	} else if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)DEBUG_RESET) != 0U) {
		/* Reset caused due Debug reset request */
		rst_source = DEBUG_RESET;
		sys_write32((uint32_t)DEBUG_RESET, REG_SYS_EXCEPTION);
	} else if ((sys_read32(REG_SYS_EXCEPTION) & (uint32_t)CPU0_RESET) != 0U) {
		/* Reset caused due to CPU0 reset. CPU reset can be caused by CPU self-test completion, or by toggling the "CPU RESET" bit of the CPU Reset Control Register. */
		rst_source = CPU0_RESET;
		sys_write32((uint32_t)CPU0_RESET, REG_SYS_EXCEPTION);
	} else {
		/* No_reset occured. */
		rst_source = NO_RESET;
	}

	return rst_source;
}

void esm_init(void)
{
	/** - Disable error pin channels */
	sys_write32(0xFFFFFFFFU, REG_ESM_DEPAPR1);
	sys_write32(0xFFFFFFFFU, REG_ESM_IEPCR4);
	sys_write32(0xFFFFFFFFU, REG_ESM_IEPCR7);

	/** - Disable interrupts */
	sys_write32(0xFFFFFFFFU, REG_ESM_IECR1);
	sys_write32(0xFFFFFFFFU, REG_ESM_IECR4);
	sys_write32(0xFFFFFFFFU, REG_ESM_IECR7);

	/** - Clear error status flags */
	sys_write32(0xFFFFFFFFU, REG_ESM_SR1_0);
	sys_write32(0xFFFFFFFFU, REG_ESM_SR1_1);
	sys_write32(0xFFFFFFFFU, REG_ESM_SSR2);
	sys_write32(0xFFFFFFFFU, REG_ESM_SR1_2);
	sys_write32(0xFFFFFFFFU, REG_ESM_SR4_0);
	sys_write32(0xFFFFFFFFU, REG_ESM_SR7_0);

	/** - Setup LPC preload */
	sys_write32(16384U - 1U, REG_ESM_LTCPR);

	/** - Reset error pin */
	if (sys_read32(REG_ESM_EPSR) == 0U) {
		sys_write32(0x00000005U, REG_ESM_EKR);
	} else {
		sys_write32(0x00000000U, REG_ESM_EKR);
	}

	/** - Clear interrupt level */
	sys_write32(0xFFFFFFFFU, REG_ESM_ILCR1);
	sys_write32(0xFFFFFFFFU, REG_ESM_ILCR4);
	sys_write32(0xFFFFFFFFU, REG_ESM_ILCR7);

	/** - Set interrupt level */
	sys_write32(0, REG_ESM_ILSR1);
	sys_write32(0, REG_ESM_ILSR4);
	sys_write32(0, REG_ESM_ILSR7);

	/** - Enable error pin channels */
	sys_write32(0, REG_ESM_EEPAPR1);
	sys_write32(0, REG_ESM_IEPSR4);
	sys_write32(0, REG_ESM_IEPSR7);

	/** - Enable interrupts */
	sys_write32(0, REG_ESM_IESR1);
	sys_write32(0, REG_ESM_IESR4);
	sys_write32(0, REG_ESM_IESR7);
}

void z_arm_platform_init(void)
{
	const int pll_retries = 5;
	resetSource_t rstSrc;

	/* TIP: if you are experimenting with the clock settings, and the early
	init code is wrong, it can make the CPU not accessible by JTAG easily.
	Put a decently long delay here so you can press reset and connect
	debugger. */
	for (volatile uint64_t i = 0; i < 0xfffff; i++) {
		/* wait */
	}

	/* initialize VIM RAM */
	volatile uint32_t *ptr = (volatile uint32_t *) DRV_VIMRAM;
	for (int i = 0; i < DRV_VIMRAM_SZ; i++) {
		*ptr = 0;
	}

	rstSrc = get_reset_source();

	switch (rstSrc) {
	case POWERON_RESET:
		/* Add condition to check whether PLL can be started successfully */
		if (_errata_SSWF021_45_both_plls(pll_retries) != 0U) {
			/* Put system in a safe state */
			/* handlePLLLockFail(); */
		}

	case DEBUG_RESET:
	case EXT_RESET:
		/* Check if there were ESM group3 errors during power-up.
		 * These could occur during eFuse auto-load or during reads from flash OTP
		 * during power-up. Device operation is not reliable and not recommended
		 * in this case. */
		if (sys_read32(REG_ESM_SR1_2) != 0U) {
			/* esmGroup3Notification(esmREG,esmREG->SR1[2]); */
		}

		/* Configure system response to error conditions signaled to the ESM group1 */
		/* This function can be configured from the ESM tab of HALCoGen */
		esm_init();
		break;

	case OSC_FAILURE_RESET:
		break;

	case WATCHDOG_RESET:
	case WATCHDOG2_RESET:
		break;

	case CPU0_RESET:
		/* Enable CPU Event Export */
		/* This allows the CPU to signal any single-bit or double-bit errors detected
		* by its ECC logic for accesses to program flash or data RAM.
		*/
		_coreEnableEventBusExport_();
		break;

	case SW_RESET:
		break;

	default:
		break;
	}

	_cacheEnable_();
	sys_cache_instr_enable();
	sys_cache_data_enable();

	system_init();
}

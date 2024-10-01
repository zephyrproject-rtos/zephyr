/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2023-2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAM3X MCU series initialization code
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Atmel SAM3X series processor.
 */

#include <soc.h>
#include <soc_pmc.h>
#include <soc_supc.h>

/**
 * @brief Setup various clocks on SoC at boot time.
 *
 * Setup Slow, Main, PLLA, Processor and Master clocks during the device boot.
 * It is assumed that the relevant registers are at their reset value.
 */
static ALWAYS_INLINE void clock_init(void)
{
	/* Switch the main clock to the internal OSC with 12MHz */
	soc_pmc_switch_mainck_to_fastrc(SOC_PMC_FAST_RC_FREQ_12MHZ);

	/* Switch MCK (Master Clock) to the main clock */
	soc_pmc_mck_set_source(SOC_PMC_MCK_SRC_MAIN_CLK);

	EFC0->EEFC_FMR = EEFC_FMR_FWS(0);
	EFC1->EEFC_FMR = EEFC_FMR_FWS(0);

	soc_pmc_enable_clock_failure_detector();

	if (IS_ENABLED(CONFIG_SOC_ATMEL_SAM_EXT_SLCK)) {
		soc_supc_slow_clock_select_crystal_osc();
	}

	if (IS_ENABLED(CONFIG_SOC_ATMEL_SAM_EXT_MAINCK)) {
		/*
		 * Setup main external crystal oscillator.
		 */

		/* We select maximum setup time.
		 * While start up time could be shortened
		 * this optimization is not deemed
		 * critical now.
		 */
		soc_pmc_switch_mainck_to_xtal(false, 0xff);
	}

	/*
	 * Set FWS (Flash Wait State) value before increasing Master Clock
	 * (MCK) frequency.
	 * TODO: set FWS based on the actual MCK frequency and VDDCORE value
	 * rather than maximum supported 84 MHz at standard VDDCORE=1.8V
	 */
	EFC0->EEFC_FMR = EEFC_FMR_FWS(4);
	EFC1->EEFC_FMR = EEFC_FMR_FWS(4);

	/*
	 * Setup PLLA
	 */

	/*
	 * PLL clock = Main * (MULA + 1) / DIVA
	 *
	 * By default, MULA == 6, DIVA == 1.
	 * With main crystal running at 12 MHz,
	 * PLL = 12 * (6 + 1) / 1 = 84 MHz
	 *
	 * With Processor Clock prescaler at 1
	 * Processor Clock (HCLK) = 84 MHz.
	 */
	soc_pmc_enable_pllack(CONFIG_SOC_ATMEL_SAM_PLLA_MULA, 0x3Fu,
			      CONFIG_SOC_ATMEL_SAM_PLLA_DIVA);

	/*
	 * Final setup of the Master Clock
	 */

	/* prescaler has to be set before PLL lock */
	soc_pmc_mck_set_prescaler(1);

	/* Select PLL as Master Clock source. */
	soc_pmc_mck_set_source(SOC_PMC_MCK_SRC_PLLA_CLK);

	/* Disable internal fast RC if we have an external crystal oscillator */
	if (IS_ENABLED(CONFIG_SOC_ATMEL_SAM_EXT_MAINCK)) {
		soc_pmc_osc_disable_fastrc();
	}
}

void soc_reset_hook(void)
{
	if (IS_ENABLED(CONFIG_SOC_ATMEL_SAM_WAIT_MODE)) {
		/*
		 * Instruct CPU to enter Wait mode instead of Sleep mode to
		 * keep Processor Clock (HCLK) and thus be able to debug
		 * CPU using JTAG.
		 */
		soc_pmc_enable_waitmode();
	}
	/* Setup system clocks */
	clock_init();
}

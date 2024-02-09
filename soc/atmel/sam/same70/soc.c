/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2023-2024 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM E70 MCU initialization code
 *
 * This file provides routines to initialize and support board-level hardware
 * for the Atmel SAM E70 MCU.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/arch/cache.h>
#include <soc.h>
#include <soc_pmc.h>
#include <soc_supc.h>
#include <cmsis_core.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

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

	EFC->EEFC_FMR = EEFC_FMR_FWS(0) | EEFC_FMR_CLOE;

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
	 * TODO: set FWS based on the actual MCK frequency and VDDIO value
	 * rather than maximum supported 150 MHz at standard VDDIO=2.7V
	 */
	EFC->EEFC_FMR = EEFC_FMR_FWS(5) | EEFC_FMR_CLOE;

	/*
	 * Setup PLLA
	 */

	/*
	 * PLL clock = Main * (MULA + 1) / DIVA
	 *
	 * By default, MULA == 24, DIVA == 1.
	 * With main crystal running at 12 MHz,
	 * PLL = 12 * (24 + 1) / 1 = 300 MHz
	 *
	 * With Processor Clock prescaler at 1
	 * Processor Clock (HCLK)=300 MHz.
	 */
	soc_pmc_enable_pllack(CONFIG_SOC_ATMEL_SAM_PLLA_MULA, 0x3Fu,
			      CONFIG_SOC_ATMEL_SAM_PLLA_DIVA);


	soc_pmc_enable_upllck(0x3Fu);

	/*
	 * Final setup of the Master Clock
	 */

	/* Setting PLLA as MCK, first prescaler, then divider and source last */
	soc_pmc_mck_set_prescaler(1);
	soc_pmc_mck_set_divider(CONFIG_SOC_ATMEL_SAM_MDIV);
	soc_pmc_mck_set_source(SOC_PMC_MCK_SRC_PLLA_CLK);


	/* Disable internal fast RC if we have an external crystal oscillator */
	if (IS_ENABLED(CONFIG_SOC_ATMEL_SAM_EXT_MAINCK)) {
		soc_pmc_osc_disable_fastrc();
	}
}

void z_arm_platform_init(void)
{
	if (IS_ENABLED(CONFIG_SOC_ATMEL_SAM_WAIT_MODE)) {
		/*
		 * Instruct CPU to enter Wait mode instead of Sleep mode to
		 * keep Processor Clock (HCLK) and thus be able to debug
		 * CPU using JTAG.
		 */
		soc_pmc_enable_waitmode();
	}

	/*
	 * DTCM is enabled by default at reset, therefore we have to disable
	 * it first to get the caches into a state where then the
	 * sys_cache*-functions can enable them, if requested by the
	 * configuration.
	 */
	SCB_DisableDCache();

	/*
	 * Enable the caches only if configured to do so.
	 */
	sys_cache_instr_enable();
	sys_cache_data_enable();

	/* Setup system clocks */
	clock_init();
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run at the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int atmel_same70_init(void)
{
	/* Check that the CHIP CIDR matches the HAL one */
	if (CHIPID->CHIPID_CIDR != CHIP_CIDR) {
		LOG_WRN("CIDR mismatch: chip = 0x%08x vs HAL = 0x%08x",
			(uint32_t)CHIPID->CHIPID_CIDR, (uint32_t)CHIP_CIDR);
	}

	return 0;
}

SYS_INIT(atmel_same70_init, PRE_KERNEL_1, 0);

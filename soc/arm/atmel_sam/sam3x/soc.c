/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/irq.h>

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
#define PMC_CKGR_PLLAR_MULA	\
	(CKGR_PLLAR_MULA(CONFIG_SOC_ATMEL_SAM3X_PLLA_MULA))
#define PMC_CKGR_PLLAR_DIVA	\
	(CKGR_PLLAR_DIVA(CONFIG_SOC_ATMEL_SAM3X_PLLA_DIVA))

/**
 * @brief Setup various clocks on SoC at boot time.
 *
 * Setup Slow, Main, PLLA, Processor and Master clocks during the device boot.
 * It is assumed that the relevant registers are at their reset value.
 */
static ALWAYS_INLINE void clock_init(void)
{
	uint32_t reg_val;

#ifdef CONFIG_SOC_ATMEL_SAM3X_EXT_SLCK
	/* Switch slow clock to the external 32 kHz crystal oscillator */
	SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_XTALSEL;

	/* Wait for oscillator to be stabilized */
	while (!(SUPC->SUPC_SR & SUPC_SR_OSCSEL)) {
		;
	}
#endif /* CONFIG_SOC_ATMEL_SAM3X_EXT_SLCK */

#ifdef CONFIG_SOC_ATMEL_SAM3X_EXT_MAINCK
	/*
	 * Setup main external crystal oscillator
	 */

	/* Start the external crystal oscillator */
	PMC->CKGR_MOR =   CKGR_MOR_KEY_PASSWD
			/* Fast RC Oscillator Frequency is at 4 MHz (default) */
			| CKGR_MOR_MOSCRCF_4_MHz
			/* We select maximum setup time. While start up time
			 * could be shortened this optimization is not deemed
			 * critical now.
			 */
			| CKGR_MOR_MOSCXTST(0xFFu)
			/* RC OSC must stay on */
			| CKGR_MOR_MOSCRCEN
			| CKGR_MOR_MOSCXTEN;

	/* Wait for oscillator to be stabilized */
	while (!(PMC->PMC_SR & PMC_SR_MOSCXTS)) {
		;
	}

	/* Select the external crystal oscillator as the main clock source */
	PMC->CKGR_MOR =   CKGR_MOR_KEY_PASSWD
			| CKGR_MOR_MOSCRCF_4_MHz
			| CKGR_MOR_MOSCSEL
			| CKGR_MOR_MOSCXTST(0xFFu)
			| CKGR_MOR_MOSCRCEN
			| CKGR_MOR_MOSCXTEN;

	/* Wait for external oscillator to be selected */
	while (!(PMC->PMC_SR & PMC_SR_MOSCSELS)) {
		;
	}

	/* Turn off RC OSC, not used any longer, to save power */
	PMC->CKGR_MOR =   CKGR_MOR_KEY_PASSWD
			| CKGR_MOR_MOSCSEL
			| CKGR_MOR_MOSCXTST(0xFFu)
			| CKGR_MOR_MOSCXTEN;

	/* Wait for RC OSC to be turned off */
	while (PMC->PMC_SR & PMC_SR_MOSCRCS) {
		;
	}

#ifdef CONFIG_SOC_ATMEL_SAM3X_WAIT_MODE
	/*
	 * Instruct CPU to enter Wait mode instead of Sleep mode to
	 * keep Processor Clock (HCLK) and thus be able to debug
	 * CPU using JTAG
	 */
	PMC->PMC_FSMR |= PMC_FSMR_LPM;
#endif
#else
	/*
	 * Setup main fast RC oscillator
	 */

	/*
	 * NOTE: MOSCRCF must be changed only if MOSCRCS is set in the PMC_SR
	 * register, should normally be the case here
	 */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS)) {
		;
	}

	/* Set main fast RC oscillator to 12 MHz */
	PMC->CKGR_MOR =   CKGR_MOR_KEY_PASSWD
			| CKGR_MOR_MOSCRCF_12_MHz
			| CKGR_MOR_MOSCRCEN;

	/* Wait for oscillator to be stabilized */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS)) {
		;
	}
#endif /* CONFIG_SOC_ATMEL_SAM3X_EXT_MAINCK */

	/*
	 * Setup PLLA
	 */

	/* Switch MCK (Master Clock) to the main clock first */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_CSS_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_CSS_MAIN_CLK;

	/* Wait for clock selection to complete */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
		;
	}

	/* Setup PLLA */
	PMC->CKGR_PLLAR =   CKGR_PLLAR_ONE
			  | PMC_CKGR_PLLAR_MULA
			  | CKGR_PLLAR_PLLACOUNT(0x3Fu)
			  | PMC_CKGR_PLLAR_DIVA;

	/*
	 * NOTE: Both MULA and DIVA must be set to a value greater than 0 or
	 * otherwise PLL will be disabled. In this case we would get stuck in
	 * the following loop.
	 */

	/* Wait for PLL lock */
	while (!(PMC->PMC_SR & PMC_SR_LOCKA)) {
		;
	}

	/*
	 * Final setup of the Master Clock
	 */

	/*
	 * NOTE: PMC_MCKR must not be programmed in a single write operation.
	 * If CSS or PRES are modified we must wait for MCKRDY bit to be
	 * set again.
	 */

	/* Setup prescaler - PLLA Clock / Processor Clock (HCLK) */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_PRES_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_PRES_CLK_1;

	/* Wait for Master Clock setup to complete */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
		;
	}

	/* Finally select PLL as Master Clock source */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_CSS_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_CSS_PLLA_CLK;

	/* Wait for Master Clock setup to complete */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
		;
	}
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This has to be run at the very beginning thus the init priority is set at
 * 0 (zero).
 *
 * @return 0
 */
static int atmel_sam3x_init(void)
{
	uint32_t key;


	key = irq_lock();

	/*
	 * Set FWS (Flash Wait State) value before increasing Master Clock
	 * (MCK) frequency.
	 * TODO: set FWS based on the actual MCK frequency and VDDCORE value
	 * rather than maximum supported 84 MHz at standard VDDCORE=1.8V
	 */
	EFC0->EEFC_FMR = EEFC_FMR_FWS(4);
	EFC1->EEFC_FMR = EEFC_FMR_FWS(4);

	/* Setup system clocks */
	clock_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_sam3x_init, PRE_KERNEL_1, 0);

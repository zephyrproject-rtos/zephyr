/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAM3U MCU series initialization code
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Atmel SAM3U series processor.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/irq.h>

#define PMC_CKGR_PLLAR_MULA (CKGR_PLLAR_MULA(0xFUL))
#define PMC_CKGR_PLLAR_DIVA (CKGR_PLLAR_DIVA(0x1UL))

#define WAIT_FOR_BIT_SET(REG, BIT)                                                                 \
	while (!(REG & BIT)) {                                                                     \
		;                                                                                  \
	}

#define WAIT_FOR_BIT_CLEAR(REG, BIT)                                                               \
	while (REG & BIT) {                                                                        \
		;                                                                                  \
	}

/**
 * @brief Setup various clocks on SoC at boot time.
 *
 * Setup Slow, Main, PLLA, Processor and Master clocks during the device boot.
 * It is assumed that the relevant registers are at their reset value.
 */
static ALWAYS_INLINE void clock_init(void)
{
	/* Disable write protect */
	PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD;

	/* Switch slow clock to the external 32 kHz crystal oscillator */
	SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_XTALSEL;
	WAIT_FOR_BIT_SET(SUPC->SUPC_SR, SUPC_SR_OSCSEL);

	/*
	 * Setup main external crystal oscillator
	 */

	/* Start the external crystal oscillator */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTST(0x8UL) |
			CKGR_MOR_MOSCXTEN;
	WAIT_FOR_BIT_SET(PMC->PMC_SR, PMC_SR_MOSCXTS);

	/* Select the external crystal oscillator as the main clock source */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTST(0x8UL) |
			CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCSEL;
	WAIT_FOR_BIT_SET(PMC->PMC_SR, PMC_SR_MOSCSELS);

	/* Turn off RC OSC, not used any longer, to save power */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTST(0x8UL) | CKGR_MOR_MOSCXTEN |
			CKGR_MOR_MOSCSEL;
	WAIT_FOR_BIT_CLEAR(PMC->PMC_SR, PMC_SR_MOSCRCS);

	/*
	 * Setup PLLA
	 */

	/* Switch MCK (Master Clock) to the main clock first */
	PMC->PMC_MCKR = (PMC->PMC_MCKR & ~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	WAIT_FOR_BIT_SET(PMC->PMC_SR, PMC_SR_MCKRDY);

	/* Setup PLLA */
	PMC->CKGR_PLLAR = CKGR_PLLAR_ONE | PMC_CKGR_PLLAR_MULA | CKGR_PLLAR_PLLACOUNT(0x3FUL) |
			  PMC_CKGR_PLLAR_DIVA;
	WAIT_FOR_BIT_SET(PMC->PMC_SR, PMC_SR_LOCKA);

	/*
	 * Final setup of the Master Clock
	 */

	/* Switch to main clock */
	PMC->PMC_MCKR = PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_MAIN_CLK;
	WAIT_FOR_BIT_SET(PMC->PMC_SR, PMC_SR_MCKRDY);

	/* Switch to PLLA */
	PMC->PMC_MCKR = PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK;
	WAIT_FOR_BIT_SET(PMC->PMC_SR, PMC_SR_MCKRDY);

	/* Enable write protect */
	PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN;
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This has to be run at the very beginning thus the init priority is set at
 * 0 (zero).
 *
 * @return 0
 */
static int atmel_sam3u_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/*
	 * Set FWS (Flash Wait State) value before increasing Master Clock
	 * (MCK) frequency.
	 * TODO: set FWS based on the actual MCK frequency and VDDCORE value
	 * rather than maximum supported 84 MHz at standard VDDCORE=1.8V
	 */
	EFC0->EEFC_FMR = EEFC_FMR_FWS(4);
#if IS_ENABLED(CONFIG_SOC_PART_NUMBER_SAM3U4C) || IS_ENABLED(CONFIG_SOC_PART_NUMBER_SAM3U4E)
	EFC1->EEFC_FMR = EEFC_FMR_FWS(4);
#endif

	/* Setup system clocks */
	clock_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_sam3u_init, PRE_KERNEL_1, 0);

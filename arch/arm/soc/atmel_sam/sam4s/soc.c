/*
 * Copyright (c) 2017 Justin Watson
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAM4S MCU series initialization code
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Atmel SAM4S series processor.
 */

#include <device.h>
#include <init.h>
#include <soc.h>

#include <arch/cpu.h>
#include <cortex_m/exc.h>

/*
 * PLL clock = Main * (MULA + 1) / DIVA
 *
 * By default, MULA == 6, DIVA == 1.
 * With main crystal running at 12 MHz,
 * PLL = 12 * (6 + 1) / 1 = 84 MHz
 *
 * With processor clock prescaler at 1,
 * the processor clock is at 84 MHz.
 */
#define PMC_CKGR_PLLAR_MULA	\
	((CONFIG_SOC_ATMEL_SAM4S_PLLA_MULA) << 16)
#define PMC_CKGR_PLLAR_DIVA	\
	((CONFIG_SOC_ATMEL_SAM4S_PLLA_DIVA) << 0)

/*
 * PLL clock = Main * (MULB + 1) / DIVB
 *
 * By default, MULB == 6, DIVB == 1.
 * With main crystal running at 12 MHz,
 * PLL = 12 * (6 + 1) / 1 = 84 MHz
 *
 * With processor clock prescaler at 1,
 * the processor clock is at 84 MHz.
 */
#define PMC_CKGR_PLLBR_MULB	\
	((CONFIG_SOC_ATMEL_SAM4S_PLLB_MULB) << 16)
#define PMC_CKGR_PLLBR_DIVB	\
	((CONFIG_SOC_ATMEL_SAM4S_PLLB_DIVB) << 0)

#if CONFIG_SOC_ATMEL_SAM4S_MDIV == 1
#define SOC_ATMEL_SAM4S_MDIV PMC_MCKR_MDIV_EQ_PCK
#elif CONFIG_SOC_ATMEL_SAM4S_MDIV == 2
#define SOC_ATMEL_SAM4S_MDIV PMC_MCKR_MDIV_PCK_DIV2
#elif CONFIG_SOC_ATMEL_SAM4S_MDIV == 3
#define SOC_ATMEL_SAM4S_MDIV PMC_MCKR_MDIV_PCK_DIV3
#elif CONFIG_SOC_ATMEL_SAM4S_MDIV == 4
#define SOC_ATMEL_SAM4S_MDIV PMC_MCKR_MDIV_PCK_DIV4
#else
#error "Invalid CONFIG_SOC_ATMEL_SAM4S_MDIV define value"
#endif

/**
 * @brief Setup various clock on SoC at boot time.
 *
 * Setup the SoC clocks according to section 28.12 in datasheet.
 *
 * Setup Slow, Main, PLLA, Procesor and Master clocks during the device boot.
 * It is assumed that the relevant registers are at their reset value.
 */
static ALWAYS_INLINE void clock_init(void)
{
	u32_t reg_val;

#ifdef CONFIG_SOC_ATMEL_SAM4S_EXT_SLCK
	/* Switch slow clock to the external 32 KHz crystal oscillator. */
	SUPC->SUPC_CR = SUPC_CR_KEY | SUPC_CR_XTALSEL;

	/* Wait for oscillator to be stabilized. */
	while (!(SUPC->SUPC_SR & SUPC_SR_OSCSEL))
		;

#endif /* CONFIG_SOC_ATMEL_SAM4S_EXT_SLCK */

#ifdef CONFIG_SOC_ATMEL_SAM4S_EXT_MAINCK
	/*
	 * Setup main external crystal oscillator.
	 */

	/* Start the external crystal oscillator. */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD
			/* Fast RC oscillator frequency is at 4 MHz. */
			| CKGR_MOR_MOSCRCF_4MHz
			/*
			 * We select maximum setup time. While start up time
			 * could be shortened this optimization is not deemed
			 * critical right now.
			 */
			| CKGR_MOR_MOSCXTST(0xFFu)
			/* RC OSC must stay on. */
			| PMC_CKGR_MOR_MOSCRCEN
			| PMC_CKGR_MOR_MOSCXTEN;

	/* Wait for oscillator to be stabilized. */
	while (!(PMC->PMC_SR & PMC_SR_MOSCXTS))
		;

	/* Select the external crystal oscillator as the main clock source. */
	PMC->CKGR_MOR = PMC_CKGR_MOR_KEY
			| PMC_CKGR_MOR_MOSCRCF_4MHZ
			| PMC_CKGR_MOR_MOSCRCEN
			| PMC_CKGR_MOR_MOSCXTEN
			| PMC_CKGR_MOR_MOSCXTST
			| PMC_CKGR_MOR_MOSCSEL;

	/* Wait for external oscillator to be selected. */
	while (!(PMC->PMC_SR & PMC_INT_MOSCSELS))
		;

	/* Turn off RC OSC, not used any longer, to save power */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD
			| CKGR_MOR_MOSCSEL
			| CKGR_MOR_MOSCXTST(0xFFu)
			| CKGR_MOR_MOSCXTEN;

	/* Wait for the RC oscillator to be turned off. */
	while (PMC->PMC_SR & PMC_SR_MOSCRCS)
		;

#ifdef CONFIG_SOC_ATMEL_SAM4S_WAIT_MODE
	/*
	 * Instruct CPU to enter Wait mode instead of Sleep mode to
	 * keep Processor Clock (HCLK) and thus be able to debug
	 * CPU using JTAG
	 */
	PMC->PMC_FSMR |= PMC_FSMR_LPM;
#endif
#else
	/*
	 * Set main fast RC oscillator.
	 */

	/*
	 * NOTE: MOSCREF must be changed only if MOSCRCS is set in the PMC_SR
	 * register, should normally be the case.
	 */
	PMC->CKGR_MOR = CKGR_MOR_KEY_PASSWD
			| CKGR_MOR_MOSCRCF_12MHZ
			| CKGR_MOR_MOSCRCEN;

	/* Wait for RC oscillator to stabilize. */
	while (!(PMC->PMC_SR & PMC_SR_MOSCRCS))
		;
#endif /* CONFIG_SOC_ATMEL_SAM4S_EXT_MAINCK */

	/*
	 * Setup PLLA
	 */

	/* Switch MCK (Master Clock) to the main clock first. */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_CSS_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_CSS_MAIN_CLK;

	/* Wait for clock selection to complete. */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY))
		;

	/* Setup PLLA. */
	PMC->CKGR_PLLAR = CKGR_PLLAR_ONE
			  | PMC_CKGR_PLLAR_MULA
			  | CKGR_PLLAR_PLLACOUNT(0x3Fu)
			  | PMC_CKGR_PLLAR_DIVA;

	/*
	 * NOTE: Both MULA and DIVA must be set to a value greater than 0 or
	 * otherwise PLL will be disabled. In this case we would get stuck in
	 * the following loop.
	 */

	/* Wait for PLL lock. */
	while (!(PMC->PMC_SR & PMC_SR_LOCKA))
		;

	/* Setup PLLB. */
	PMC->CKGR_PLLBR = PMC_CKGR_PLLBR_MULB
			  | CKGR_PLLBR_PLLBCOUNT(0x3Fu)
			  | PMC_CKGR_PLLBR_DIVB;

	/*
	 * NOTE: Both MULB and DIVB must be set to a value greater than 0 or
	 * otherwise PLL will be disabled. In this case we would get stuck in
	 * the following loop.
	 */

	/* Wait for PLL lock. */
	while (!(PMC->PMC_SR & PMC_SR_LOCKB))
		;

	/*
	 * Final setup of the Master Clock
	 */

	/*
	 * NOTE: PMC_MCKR must not be programmed in a single write operation.
	 * If CSS, MDIV or PRES are modified we must wait for MCKRDY bit to be
	 * set again.
	 */

	/* Setup prescaler - PLLA Clock / Processor Clock (HCLK). */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_PRES_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_PRES_CLK_1;

	/* Wait for Master Clock setup to complete */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY))
		;

	/* Setup divider - Processor Clock (HCLK) / Master Clock (MCK). */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_MDIV_Msk;
	PMC->PMC_MCKR = reg_val | SOC_ATMEL_SAM4S_MDIV;

	/* Wait for Master Clock setup to complete. */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY))
		;

	/* Finally select PLL as Master Clock source. */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_CSS_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_CSS_PLLA_CLK;

	/* Wait for Master Clock setup to complete. */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY))
		;
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int atmel_sam4s_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Clear all faults. */
	_ClearFaults();

	/*
	 * Set FWS (Flash Wait State) value before increasing Master Clock
	 * (MCK) frequency.
	 * TODO: set FWS based on the actual MCK frequency and VDDIO value
	 * rather than maximum supported 150 MHz at standard VDDIO=2.7V
	 */
	EFC0->EEFC_FMR = EEFC_FMR_FWS(4);
	EFC1->EEFC_FMR = EEFC_FMR_FWS(4);

	/* Setup master clock */
	clock_init();

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise.
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_sam4s_init, PRE_KERNEL_1, 0);

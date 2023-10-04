/*
 * Copyright (c) 2016 Piotr Mienkowski
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
#include <soc.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/* Power Manager Controller */

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
#define PMC_CKGR_PLLAR_MULA	\
	(CKGR_PLLAR_MULA(CONFIG_SOC_ATMEL_SAME70_PLLA_MULA))
#define PMC_CKGR_PLLAR_DIVA	\
	(CKGR_PLLAR_DIVA(CONFIG_SOC_ATMEL_SAME70_PLLA_DIVA))

#if CONFIG_SOC_ATMEL_SAME70_MDIV == 1
#define SOC_ATMEL_SAME70_MDIV PMC_MCKR_MDIV_EQ_PCK
#elif CONFIG_SOC_ATMEL_SAME70_MDIV == 2
#define SOC_ATMEL_SAME70_MDIV PMC_MCKR_MDIV_PCK_DIV2
#elif CONFIG_SOC_ATMEL_SAME70_MDIV == 3
#define SOC_ATMEL_SAME70_MDIV PMC_MCKR_MDIV_PCK_DIV3
#elif CONFIG_SOC_ATMEL_SAME70_MDIV == 4
#define SOC_ATMEL_SAME70_MDIV PMC_MCKR_MDIV_PCK_DIV4
#else
#error "Invalid CONFIG_SOC_ATMEL_SAME70_MDIV define value"
#endif

/**
 * @brief Setup various clocks on SoC at boot time.
 *
 * Setup Slow, Main, PLLA, Processor and Master clocks during the device boot.
 * It is assumed that the relevant registers are at their reset value.
 */
static ALWAYS_INLINE void clock_init(void)
{
	uint32_t reg_val;

#ifdef CONFIG_SOC_ATMEL_SAME70_EXT_SLCK
	/* Switch slow clock to the external 32 kHz crystal oscillator */
	SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_XTALSEL;

	/* Wait for oscillator to be stabilized */
	while (!(SUPC->SUPC_SR & SUPC_SR_OSCSEL)) {
		;
	}
#endif /* CONFIG_SOC_ATMEL_SAME70_EXT_SLCK */

#ifdef CONFIG_SOC_ATMEL_SAME70_EXT_MAINCK
	/*
	 * Setup main external crystal oscillator if not already done
	 * by a previous program i.e. bootloader
	 */

	if (!(PMC->CKGR_MOR & CKGR_MOR_MOSCSEL_Msk)) {
		/* Start the external crystal oscillator */
		PMC->CKGR_MOR =   CKGR_MOR_KEY_PASSWD
				/* We select maximum setup time.
				 * While start up time could be shortened
				 * this optimization is not deemed
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

		/* Select the external crystal oscillator as main clock */
		PMC->CKGR_MOR =   CKGR_MOR_KEY_PASSWD
				| CKGR_MOR_MOSCSEL
				| CKGR_MOR_MOSCXTST(0xFFu)
				| CKGR_MOR_MOSCRCEN
				| CKGR_MOR_MOSCXTEN;

		/* Wait for external oscillator to be selected */
		while (!(PMC->PMC_SR & PMC_SR_MOSCSELS)) {
			;
		}
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

#ifdef CONFIG_SOC_ATMEL_SAME70_WAIT_MODE
	/*
	 * Instruct CPU to enter Wait mode instead of Sleep mode to
	 * keep Processor Clock (HCLK) and thus be able to debug
	 * CPU using JTAG
	 */
	PMC->PMC_FSMR |= PMC_FSMR_LPM;
#endif
#else
	/* Attempt to change main fast RC oscillator frequency */

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
#endif /* CONFIG_SOC_ATMEL_SAME70_EXT_MAINCK */

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

	/* Setup UPLL */
	PMC->CKGR_UCKR = CKGR_UCKR_UPLLCOUNT(0x3Fu) | CKGR_UCKR_UPLLEN;

	/* Wait for PLL lock */
	while (!(PMC->PMC_SR & PMC_SR_LOCKU)) {
		;
	}

	/*
	 * Final setup of the Master Clock
	 */

	/*
	 * NOTE: PMC_MCKR must not be programmed in a single write operation.
	 * If CSS, MDIV or PRES are modified we must wait for MCKRDY bit to be
	 * set again.
	 */

	/* Setup prescaler - PLLA Clock / Processor Clock (HCLK) */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_PRES_Msk;
	PMC->PMC_MCKR = reg_val | PMC_MCKR_PRES_CLK_1;

	/* Wait for Master Clock setup to complete */
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
		;
	}

	/* Setup divider - Processor Clock (HCLK) / Master Clock (MCK) */
	reg_val = PMC->PMC_MCKR & ~PMC_MCKR_MDIV_Msk;
	PMC->PMC_MCKR = reg_val | SOC_ATMEL_SAME70_MDIV;

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
 * This needs to be run at the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int atmel_same70_init(void)
{
	uint32_t key;


	key = irq_lock();

	SCB_EnableICache();

	if (!(SCB->CCR & SCB_CCR_DC_Msk)) {
		SCB_EnableDCache();
	}

	/*
	 * Set FWS (Flash Wait State) value before increasing Master Clock
	 * (MCK) frequency.
	 * TODO: set FWS based on the actual MCK frequency and VDDIO value
	 * rather than maximum supported 150 MHz at standard VDDIO=2.7V
	 */
	EFC->EEFC_FMR = EEFC_FMR_FWS(5) | EEFC_FMR_CLOE;

	/* Setup system clocks */
	clock_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	/* Check that the CHIP CIDR matches the HAL one */
	if (CHIPID->CHIPID_CIDR != CHIP_CIDR) {
		LOG_WRN("CIDR mismatch: chip = 0x%08x vs HAL = 0x%08x",
			(uint32_t)CHIPID->CHIPID_CIDR, (uint32_t)CHIP_CIDR);
	}

	return 0;
}

SYS_INIT(atmel_same70_init, PRE_KERNEL_1, 0);

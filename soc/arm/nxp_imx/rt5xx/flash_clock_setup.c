/*
 * Copyright (c) 2022,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsl_power.h"
#include "flash_clock_setup.h"

#define FLEXSPI_DLL_LOCK_RETRY (10)

static void flash_deinit(FLEXSPI_Type *base)
{
	/* Enable FLEXSPI clock again */
	CLKCTL0->PSCCTL0_SET = CLKCTL0_PSCCTL0_SET_FLEXSPI0_OTFAD_CLK_MASK;

	/* Enable FLEXSPI module */
	base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;

	/* Wait until FLEXSPI is not busy */
	while (!((base->STS0 & FLEXSPI_STS0_ARBIDLE_MASK) &&
		 (base->STS0 & FLEXSPI_STS0_SEQIDLE_MASK))) {
	}
	/* Disable module during the reset procedure */
	base->MCR0 |= FLEXSPI_MCR0_MDIS_MASK;
}

static void flash_init(FLEXSPI_Type *base)
{
	uint32_t status;
	uint32_t lastStatus;
	uint32_t retry;

	/* If serial root clock is >= 100 MHz, DLLEN set to 1, OVRDEN set to 0,
	 * then SLVDLYTARGET setting of 0x0 is recommended.
	 */
	base->DLLCR[0] = 0x1U;

	/* Enable FLEXSPI module */
	base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;

	base->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (base->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
	}

	/* Need to wait DLL locked if DLL enabled */
	if (0U != (base->DLLCR[0] & FLEXSPI_DLLCR_DLLEN_MASK)) {
		lastStatus = base->STS2;
		retry = FLEXSPI_DLL_LOCK_RETRY;
		/* Wait slave delay line locked and slave reference delay line locked. */
		do {
			status = base->STS2;
			if ((status & (FLEXSPI_STS2_AREFLOCK_MASK | FLEXSPI_STS2_ASLVLOCK_MASK)) ==
			    (FLEXSPI_STS2_AREFLOCK_MASK | FLEXSPI_STS2_ASLVLOCK_MASK)) {
				/* Locked */
				retry = 100;
				break;
			} else if (status == lastStatus) {
				/* Same delay cell number in calibration */
				retry--;
			} else {
				retry = FLEXSPI_DLL_LOCK_RETRY;
				lastStatus = status;
			}
		} while (retry > 0);
		/* According to ERR011377, need to delay at least 100 NOPs to ensure
		 * the DLL is locked.
		 */
		for (; retry > 0U; retry--) {
			__NOP();
		}
	}
}

/*
 * flexspi_set_clock run in RAM used to configure FlexSPI clock source and divider
 * when XIP.
 */
void flexspi_setup_clock(FLEXSPI_Type *base, uint32_t src, uint32_t divider)
{
	if (base == FLEXSPI0) {
		if ((CLKCTL0->FLEXSPI0FCLKSEL != CLKCTL0_FLEXSPI0FCLKSEL_SEL(src)) ||
		   ((CLKCTL0->FLEXSPI0FCLKDIV & CLKCTL0_FLEXSPI0FCLKDIV_DIV_MASK) !=
		    (divider - 1))) {
			/* Always deinit FLEXSPI and init FLEXSPI for the flash to make sure the
			 * flash works correctly after the FLEXSPI root clock changed as the
			 * default FLEXSPI configuration may does not work for the new root
			 * clock frequency.
			 */
			flash_deinit(base);

			/* Disable clock before changing clock source */
			CLKCTL0->PSCCTL0_CLR = CLKCTL0_PSCCTL0_CLR_FLEXSPI0_OTFAD_CLK_MASK;
			/* Update flexspi clock. */
			CLKCTL0->FLEXSPI0FCLKSEL = CLKCTL0_FLEXSPI0FCLKSEL_SEL(src);
			/* Reset the divider counter */
			CLKCTL0->FLEXSPI0FCLKDIV |= CLKCTL0_FLEXSPI0FCLKDIV_RESET_MASK;
			CLKCTL0->FLEXSPI0FCLKDIV = CLKCTL0_FLEXSPI0FCLKDIV_DIV(divider - 1);
			while ((CLKCTL0->FLEXSPI0FCLKDIV) & CLKCTL0_FLEXSPI0FCLKDIV_REQFLAG_MASK) {
			}
			/* Enable FLEXSPI clock again */
			CLKCTL0->PSCCTL0_SET = CLKCTL0_PSCCTL0_SET_FLEXSPI0_OTFAD_CLK_MASK;

			flash_init(base);
		}
	} else if (base == FLEXSPI1) {
		if ((CLKCTL0->FLEXSPI1FCLKSEL != CLKCTL0_FLEXSPI1FCLKSEL_SEL(src)) ||
		    ((CLKCTL0->FLEXSPI1FCLKDIV & CLKCTL0_FLEXSPI1FCLKDIV_DIV_MASK) !=
		     (divider - 1))) {
			/* Always deinit FLEXSPI and init FLEXSPI for the flash to make sure the
			 * flash works correctly after the FLEXSPI root clock changed as the
			 * default FLEXSPI configuration may does not work for the new root
			 * clock frequency.
			 */
			flash_deinit(base);

			/* Disable clock before changing clock source */
			CLKCTL0->PSCCTL0_CLR = CLKCTL0_PSCCTL0_CLR_FLEXSPI1_CLK_MASK;
			/* Update flexspi clock. */
			CLKCTL0->FLEXSPI1FCLKSEL = CLKCTL0_FLEXSPI1FCLKSEL_SEL(src);
			/* Reset the divider counter */
			CLKCTL0->FLEXSPI1FCLKDIV |= CLKCTL0_FLEXSPI1FCLKDIV_RESET_MASK;
			CLKCTL0->FLEXSPI1FCLKDIV = CLKCTL0_FLEXSPI1FCLKDIV_DIV(divider - 1);
			while ((CLKCTL0->FLEXSPI1FCLKDIV) & CLKCTL0_FLEXSPI1FCLKDIV_REQFLAG_MASK) {
			}
			/* Enable FLEXSPI clock again */
			CLKCTL0->PSCCTL0_SET = CLKCTL0_PSCCTL0_SET_FLEXSPI1_CLK_MASK;

			flash_init(base);
		}
	} else {
		return;
	}
}

/* This function is used to change FlexSPI clock to a stable source before clock
 * sources(Such as PLL and Main clock) updating in case XIP (execute code on
 * FLEXSPI memory.)
 */
void flexspi_clock_safe_config(void)
{
	/* Move FLEXSPI clock source from main clock to FRO192M / 2 to avoid instruction/data
	 * fetch issue in XIP when updating PLL and main clock.
	 */
	flexspi_setup_clock(FLEXSPI0, 3U, 2U);
}

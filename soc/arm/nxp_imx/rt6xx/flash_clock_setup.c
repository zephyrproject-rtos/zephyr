/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * To be explicitly clear, the following set of functions
 * cannot execute in place from flash and are instead
 * relocated into internal SRAM.
 */

#include "fsl_power.h"
#include "flash_clock_setup.h"

#define FLEXSPI_DLL_LOCK_RETRY (10)

static void flash_deinit(FLEXSPI_Type *base)
{
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
	uint32_t mask = 0;

	/* Enable FLEXSPI module */
	base->MCR0 &= ~FLEXSPI_MCR0_MDIS_MASK;

	base->MCR0 |= FLEXSPI_MCR0_SWRESET_MASK;
	while (base->MCR0 & FLEXSPI_MCR0_SWRESET_MASK) {
	}

	/* Need to wait DLL locked if DLL enabled */
	if (0U != (base->DLLCR[0] & FLEXSPI_DLLCR_DLLEN_MASK)) {
		lastStatus = base->STS2;
		retry      = FLEXSPI_DLL_LOCK_RETRY;
		/* Flash on port A */
		if (((base->FLSHCR0[0] & FLEXSPI_FLSHCR0_FLSHSZ_MASK) > 0) ||
		    ((base->FLSHCR0[1] & FLEXSPI_FLSHCR0_FLSHSZ_MASK) > 0)) {
			mask |= FLEXSPI_STS2_AREFLOCK_MASK | FLEXSPI_STS2_ASLVLOCK_MASK;
		}
		/* Flash on port B */
		if (((base->FLSHCR0[2] & FLEXSPI_FLSHCR0_FLSHSZ_MASK) > 0) ||
		    ((base->FLSHCR0[3] & FLEXSPI_FLSHCR0_FLSHSZ_MASK) > 0)) {
			mask |= FLEXSPI_STS2_BREFLOCK_MASK | FLEXSPI_STS2_BSLVLOCK_MASK;
		}
		/* Wait slave delay line locked and slave reference delay line locked. */
		do {
			status = base->STS2;
			if ((status & mask) == mask) {
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
		/* According to ERR011377, need to delay at least 100 NOPs
		 * to ensure the DLL is locked.
		 */
		for (; retry > 0U; retry--) {
			__NOP();
		}
	}
}

void flexspi_setup_clock(FLEXSPI_Type *base, uint32_t src, uint32_t divider)
{
	if (base == FLEXSPI) {
		if ((CLKCTL0->FLEXSPIFCLKSEL != CLKCTL0_FLEXSPIFCLKSEL_SEL(src)) ||
		   ((CLKCTL0->FLEXSPIFCLKDIV & CLKCTL0_FLEXSPIFCLKDIV_DIV_MASK) !=
		    (divider - 1))) {
			/* Always deinit FLEXSPI and init FLEXSPI for the flash to make sure the
			 * flash works correctly after the FLEXSPI root clock changed as the
			 * default FLEXSPI configuration may does not work for the new root
			 * clock frequency.
			 */
			flash_deinit(base);

			/* Disable clock before changing clock source */
			CLKCTL0->PSCCTL0_CLR = CLKCTL0_PSCCTL0_CLR_FLEXSPI_OTFAD_CLK_MASK;
			/* Update flexspi clock */
			CLKCTL0->FLEXSPIFCLKSEL = CLKCTL0_FLEXSPIFCLKSEL_SEL(src);
			/* Reset the divider counter */
			CLKCTL0->FLEXSPIFCLKDIV |= CLKCTL0_FLEXSPIFCLKDIV_RESET_MASK;
			CLKCTL0->FLEXSPIFCLKDIV = CLKCTL0_FLEXSPIFCLKDIV_DIV(divider - 1);
			while ((CLKCTL0->FLEXSPIFCLKDIV) & CLKCTL0_FLEXSPIFCLKDIV_REQFLAG_MASK) {
			}
			/* Enable FLEXSPI clock again */
			CLKCTL0->PSCCTL0_SET = CLKCTL0_PSCCTL0_SET_FLEXSPI_OTFAD_CLK_MASK;

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
	flexspi_setup_clock(FLEXSPI, 3U, 1U);
}

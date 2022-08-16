/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMD MCU series initialization code
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

#define SAM0_DFLL_FREQ_HZ (48000000U)
#define SAM0_DPLL_FREQ_MIN_HZ (96000000U)
#define SAM0_DPLL_FREQ_MAX_HZ (200000000U)

// static void gclk_connect(uint8_t gclk, uint8_t src, uint8_t div)
// {
// 	GCLK->GENCTRL[gclk].reg =
// 		GCLK_GENCTRL_SRC(src) | GCLK_GENCTRL_DIV(div) | GCLK_GENCTRL_GENEN;
// }

#if CONFIG_SOC_ATMEL_SAMD5X_XOSC32K_AS_MAIN
static void osc_init(void)
{
	OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_ENABLE | OSC32KCTRL_XOSC32K_XTALEN |
				  OSC32KCTRL_XOSC32K_EN32K | OSC32KCTRL_XOSC32K_RUNSTDBY |
				  OSC32KCTRL_XOSC32K_STARTUP(7);

	while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
	}

	GCLK->GENCTRL[1].reg =
		GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC32K) | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;
}
#elif CONFIG_SOC_ATMEL_SAMD5X_OSCULP32K_AS_MAIN
static void osc_init(void)
{
	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_OSCULP32K) | GCLK_GENCTRL_RUNSTDBY |
			       GCLK_GENCTRL_GENEN;
}

#elif CONFIG_SOC_ATMEL_SAMD5X_XOSC_20MHZ
static void osc_init(void)
{
	/* Configure External Oscillator (XOSC1) */
	OSCCTRL->XOSCCTRL[1].reg = OSCCTRL_XOSCCTRL_STARTUP(8) | OSCCTRL_XOSCCTRL_IMULT(5) |
				   OSCCTRL_XOSCCTRL_IPTAT(3) | OSCCTRL_XOSCCTRL_XTALEN |
				   OSCCTRL_XOSCCTRL_ENABLE;

	while (!OSCCTRL->STATUS.bit.XOSCRDY1) {
		/* Waiting for the XOSC Ready state */
	}

	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_OSCULP32K) | GCLK_GENCTRL_RUNSTDBY |
			       GCLK_GENCTRL_GENEN;
}
#else
#error "No Clock Source selected."
#endif

#if CONFIG_SOC_ATMEL_SAMD5X_DPLL
static void dpll_init(uint8_t n, uint32_t f_cpu)
{
	/* We source the DPLL from 32kHz GCLK1 */
	const uint32_t LDR = ((f_cpu << 5) / SOC_ATMEL_SAM0_OSC32K_FREQ_HZ);

	/* disable the DPLL before changing the configuration */
	OSCCTRL->Dpll[n].DPLLCTRLA.bit.ENABLE = 0;
	while (OSCCTRL->Dpll[n].DPLLSYNCBUSY.reg) {
	}

	/* set DPLL clock source to 32kHz GCLK1 */
	GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + n].reg = GCLK_PCHCTRL_GEN(1) | GCLK_PCHCTRL_CHEN;
	while (!(GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + n].reg & GCLK_PCHCTRL_CHEN)) {
	}

	OSCCTRL->Dpll[n].DPLLRATIO.reg =
		OSCCTRL_DPLLRATIO_LDRFRAC(LDR & 0x1F) | OSCCTRL_DPLLRATIO_LDR((LDR >> 5) - 1);

	/* Without LBYPASS, startup takes very long, see errata section 2.13. */
	OSCCTRL->Dpll[n].DPLLCTRLB.reg =
		OSCCTRL_DPLLCTRLB_REFCLK_GCLK | OSCCTRL_DPLLCTRLB_WUF | OSCCTRL_DPLLCTRLB_LBYPASS;

	OSCCTRL->Dpll[n].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

	while (OSCCTRL->Dpll[n].DPLLSYNCBUSY.reg) {
	}
	while (!(OSCCTRL->Dpll[n].DPLLSTATUS.bit.CLKRDY && OSCCTRL->Dpll[n].DPLLSTATUS.bit.LOCK)) {
	}
}
#endif

static void dfll_init(void)
{
#if CONFIG_SOC_ATMEL_SAMD5X_DFLL
	uint32_t reg = OSCCTRL_DFLLCTRLB_QLDIS
#ifdef OSCCTRL_DFLLCTRLB_WAITLOCK
		       | OSCCTRL_DFLLCTRLB_WAITLOCK
#endif
		;

	OSCCTRL->DFLLCTRLB.reg = reg;
	OSCCTRL->DFLLCTRLA.reg = OSCCTRL_DFLLCTRLA_ENABLE;

	while (!OSCCTRL->STATUS.bit.DFLLRDY) {
	}
#endif
}

static void gclk_reset(void)
{
	GCLK->CTRLA.bit.SWRST = 1;
	while (GCLK->SYNCBUSY.bit.SWRST) {
	}
}

static int atmel_samd_init(const struct device *arg)
{
	uint32_t key;
#if CONFIG_SOC_ATMEL_SAMD5X_DPLL_MAIN_CLK
	uint8_t dfll_div;

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < SAM0_DFLL_FREQ_HZ) {
		dfll_div = 3;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < SAM0_DPLL_FREQ_MIN_HZ) {
		dfll_div = 2;
	} else {
		dfll_div = 1;
	}
#endif

	ARG_UNUSED(arg);

	key = irq_lock();

	/* enable the Cortex M Cache Controller */
	CMCC->CTRL.bit.CEN = 1;

	gclk_reset();
	osc_init();
	dfll_init();
#if CONFIG_SOC_ATMEL_SAMD5X_DPLL
	dpll_init(0, dfll_div * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
#endif

#if CONFIG_SOC_ATMEL_SAMD5X_DPLL_MAIN_CLK
	/* use DPLL for main clock */
	// gclk_connect(0, GCLK_SOURCE_DPLL0, dfll_div);
	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_DPLL0) | GCLK_GENCTRL_DIV(dfll_div) |
			       GCLK_GENCTRL_GENEN;

	/* connect GCLK2 to 48 MHz DFLL for USB */
	// gclk_connect(2, GCLK_SOURCE_DFLL48M, 0);
	GCLK->GENCTRL[2].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_DFLL48M) | GCLK_GENCTRL_GENEN;
#elif CONFIG_SOC_ATMEL_SAMD5X_XOSC_20MHZ
	// gclk_connect(0, GCLK_SOURCE_XOSC1, 1);
	GCLK->GENCTRL[0].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC1) | GCLK_GENCTRL_GENEN;
	/* Set CLK GEN 3 to run off XOSC 1 */
	// gclk_connect(3, GCLK_SOURCE_XOSC1, 4);
	GCLK->GENCTRL[3].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC1) | GCLK_GENCTRL_DIV(4) |
			       GCLK_GENCTRL_GENEN; // | GCLK_GENCTRL_OE;
	GCLK->GENCTRL[5].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC1) | GCLK_GENCTRL_DIV(2) |
			       GCLK_GENCTRL_GENEN | GCLK_GENCTRL_OE;
#else
#error "Need to select main clock configuration"
#endif

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_samd_init, PRE_KERNEL_1, 0);

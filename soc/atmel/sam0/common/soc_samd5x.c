/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 * Copyright (c) 2023-2025 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMD MCU series initialization code
 */

/* The CPU clock will be configured to the DT requested value,
 * and run via DFLL48M.
 *
 * Reference -> GCLK Gen 1 -> DFLL48M -> GCLK Gen 0 -> GCLK_MAIN
 *
 * GCLK Gen 0 -> GCLK_MAIN @ DPLL0
 * GCLK Gen 1 -> DFLL48M @ 32768 Hz
 * GCLK Gen 2 -> USB @ DFLL48M
 * GCLK Gen 3 -> ADC @ reserved
 * GCLK Gen 4 -> RTC @ xtal 32768 Hz
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

/* clang-format off */

#define SAM0_DFLL_FREQ_HZ		(48000000U)
#define SAM0_DPLL_FREQ_MIN_HZ		(96000000U)
#define SAM0_DPLL_FREQ_MAX_HZ		(200000000U)
#define SAM0_XOSC32K_STARTUP_TIME	CONFIG_SOC_ATMEL_SAMD5X_XOSC32K_STARTUP

#if !CONFIG_SOC_ATMEL_SAMD5X_XOSC32K
#define xosc32k_init()
#else
static inline void xosc32k_init(void)
{
	OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_ENABLE
#if CONFIG_SOC_ATMEL_SAMD5X_XOSC32K_CRYSTAL
				| OSC32KCTRL_XOSC32K_XTALEN
#endif
#if CONFIG_SOC_ATMEL_SAMD5X_XOSC32K_GAIN_HS
				| OSC32KCTRL_XOSC32K_CGM_HS
#else
				| OSC32KCTRL_XOSC32K_CGM_XT
#endif
				| OSC32KCTRL_XOSC32K_EN32K
				| OSC32KCTRL_XOSC32K_EN1K
				| OSC32KCTRL_XOSC32K_RUNSTDBY
				| OSC32KCTRL_XOSC32K_STARTUP(SAM0_XOSC32K_STARTUP_TIME);

	while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
	}
}
#endif

#if CONFIG_SOC_ATMEL_SAMD5X_XOSC32K_AS_MAIN
static void osc32k_init(void)
{
	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC32K)
			     | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;

}
#elif CONFIG_SOC_ATMEL_SAMD5X_OSCULP32K_AS_MAIN
static void osc32k_init(void)
{
	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_OSCULP32K)
			     | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_GENEN;
}
#else
#error "No Clock Source selected."
#endif

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

	OSCCTRL->Dpll[n].DPLLRATIO.reg  = OSCCTRL_DPLLRATIO_LDRFRAC(LDR & 0x1F)
					| OSCCTRL_DPLLRATIO_LDR((LDR >> 5) - 1);

	/* Without LBYPASS, startup takes very long, see errata section 2.13. */
	OSCCTRL->Dpll[n].DPLLCTRLB.reg	= OSCCTRL_DPLLCTRLB_REFCLK_GCLK
					| OSCCTRL_DPLLCTRLB_WUF
					| OSCCTRL_DPLLCTRLB_LBYPASS;

	OSCCTRL->Dpll[n].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;

	while (OSCCTRL->Dpll[n].DPLLSYNCBUSY.reg) {
	}
	while (!(OSCCTRL->Dpll[n].DPLLSTATUS.bit.CLKRDY &&
		 OSCCTRL->Dpll[n].DPLLSTATUS.bit.LOCK)) {
	}

}

static void dfll_init(void)
{
	uint32_t reg = OSCCTRL_DFLLCTRLB_QLDIS
#ifdef OSCCTRL_DFLLCTRLB_WAITLOCK
		     | OSCCTRL_DFLLCTRLB_WAITLOCK
#endif
	;

	OSCCTRL->DFLLCTRLB.reg = reg;
	OSCCTRL->DFLLCTRLA.reg = OSCCTRL_DFLLCTRLA_ENABLE;

	while (!OSCCTRL->STATUS.bit.DFLLRDY) {
	}
}

static void gclk_reset(void)
{
	GCLK->CTRLA.bit.SWRST = 1;
	while (GCLK->SYNCBUSY.bit.SWRST) {
	}
}

static void gclk_connect(uint8_t gclk, uint8_t src, uint8_t div)
{
	GCLK->GENCTRL[gclk].reg = GCLK_GENCTRL_SRC(src)
				| GCLK_GENCTRL_DIV(div)
				| GCLK_GENCTRL_GENEN;
}

void soc_reset_hook(void)
{
	uint8_t dfll_div;

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < SAM0_DFLL_FREQ_HZ) {
		dfll_div = 3;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < SAM0_DPLL_FREQ_MIN_HZ) {
		dfll_div = 2;
	} else {
		dfll_div = 1;
	}

	/*
	 * Force Cortex M Cache Controller disabled
	 *
	 * It is not clear if regular Cortex-M instructions can be used to
	 * perform cache maintenance or this is a proprietary cache controller
	 * that require special SoC support.
	 */
	CMCC->CTRL.bit.CEN = 0;

	gclk_reset();
	xosc32k_init();
	osc32k_init();
	dfll_init();
	dpll_init(0, dfll_div * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* use DPLL for main clock */
	gclk_connect(0, GCLK_SOURCE_DPLL0, dfll_div);

	/* connect GCLK2 to 48 MHz DFLL for USB */
	gclk_connect(2, GCLK_SOURCE_DFLL48M, 0);

	/* connect GCLK4 to xosc32k for RTC. The output is 1024 Hz*/
	gclk_connect(4,
#if CONFIG_SOC_ATMEL_SAMD5X_XOSC32K
		     GCLK_SOURCE_XOSC32K,
#else
		     GCLK_SOURCE_OSCULP32K,
#endif
		     CONFIG_SOC_ATMEL_SAMD5X_OSC32K_PRESCALER);
}

/* clang-format on */

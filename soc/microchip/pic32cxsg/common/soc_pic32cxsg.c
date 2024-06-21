/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Micorchip PIC32CXSG MCU series initialization code
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

#define PIC32CXSG_DFLL_FREQ_HZ			(48000000U)
#define PIC32CXSG_DPLL_FREQ_MIN_HZ		(96000000U)
#define PIC32CXSG_DPLL_FREQ_MAX_HZ		(200000000U)

#if CONFIG_SOC_MICROCHIP_PIC32CXSG_XOSC32K_AS_MAIN
static void osc32k_init(void)
{
	OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_ENABLE(1) | OSC32KCTRL_XOSC32K_XTALEN(1)
				| OSC32KCTRL_XOSC32K_EN32K(1) | OSC32KCTRL_XOSC32K_RUNSTDBY(1)
				| OSC32KCTRL_XOSC32K_STARTUP(6); /* Microchip - was 7 which is a reserved value */

	while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
	}

	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC32K)
			     | GCLK_GENCTRL_RUNSTDBY(1) | GCLK_GENCTRL_GENEN(1);

}
#elif CONFIG_SOC_MICROCHIP_PIC32CXSG_OSCULP32K_AS_MAIN
static void osc32k_init(void)
{
	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_SRC(GCLK_SOURCE_OSCULP32K)
			     | GCLK_GENCTRL_RUNSTDBY(1) | GCLK_GENCTRL_GENEN(1);
}
#else
#error "No Clock Source selected."
#endif

static void dpll_init(uint8_t n, uint32_t f_cpu)
{
	/* We source the DPLL from 32kHz GCLK1 */
	const uint32_t LDR = ((f_cpu << 5) / SOC_MICROCHIP_PIC32CXSG_OSC32K_FREQ_HZ);

	/* disable the DPLL before changing the configuration */
	OSCCTRL->Dpll[n].DPLLCTRLA.bit.ENABLE = 0;
	while (OSCCTRL->Dpll[n].DPLLSYNCBUSY.reg) {
	}

	/* set DPLL clock source to 32kHz GCLK1 */
	GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + n].reg = GCLK_PCHCTRL_GEN(1) | GCLK_PCHCTRL_CHEN(1);
	while (!(GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + n].reg & GCLK_PCHCTRL_CHEN(1))) {
	}

	OSCCTRL->Dpll[n].DPLLRATIO.reg  = OSCCTRL_DPLLRATIO_LDRFRAC(LDR & 0x1F)
					| OSCCTRL_DPLLRATIO_LDR((LDR >> 5) - 1);

	/* Without LBYPASS, startup takes very long, see errata section 2.13. */
	OSCCTRL->Dpll[n].DPLLCTRLB.reg	= OSCCTRL_DPLLCTRLB_REFCLK_GCLK
					| OSCCTRL_DPLLCTRLB_WUF(1)
					| OSCCTRL_DPLLCTRLB_LBYPASS(1);

	OSCCTRL->Dpll[n].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE(1);

	while (OSCCTRL->Dpll[n].DPLLSYNCBUSY.reg) {
	}
	while (!(OSCCTRL->Dpll[n].DPLLSTATUS.bit.CLKRDY &&
		 OSCCTRL->Dpll[n].DPLLSTATUS.bit.LOCK)) {
	}

}

static void dfll_init(void)
{
	uint32_t reg = OSCCTRL_DFLLCTRLB_QLDIS(1)
#ifdef OSCCTRL_DFLLCTRLB_WAITLOCK
		     | OSCCTRL_DFLLCTRLB_WAITLOCK(1)
#endif
	;

	OSCCTRL->DFLLCTRLB.reg = reg;
	OSCCTRL->DFLLCTRLA.reg = OSCCTRL_DFLLCTRLA_ENABLE(1);

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
				| GCLK_GENCTRL_GENEN(1);
}

void z_arm_platform_init(void)
{
	uint8_t dfll_div;

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < PIC32CXSG_DFLL_FREQ_HZ) {
		dfll_div = 3;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < PIC32CXSG_DPLL_FREQ_MIN_HZ) {
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
	osc32k_init();
	dfll_init();
	dpll_init(0, dfll_div * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* use DPLL for main clock */
	gclk_connect(0, GCLK_SOURCE_DPLL0, dfll_div);

	/* connect GCLK2 to 48 MHz DFLL for USB */
	gclk_connect(2, GCLK_SOURCE_DFLL48M, 0);
}

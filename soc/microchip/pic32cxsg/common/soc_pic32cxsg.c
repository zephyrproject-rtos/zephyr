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

#define PIC32CXSG_DFLL_FREQ_HZ         (48000000U)
#define PIC32CXSG_DPLL_FREQ_MIN_HZ     (96000000U)
#define PIC32CXSG_DPLL_FREQ_MAX_HZ     (200000000U)
#define PIC32CXSG_XOSC32K_STARTUP_TIME (6) /* Microchip - was 7 which is a reserved value */

#if CONFIG_SOC_MICROCHIP_PIC32CXSG_XOSC32K
static void osc32k_init(void)
{
	gclk_registers_t *gregs = (gclk_registers_t *)DT_REG_ADDR(DT_NODELABEL(gclk));
	osc32kctrl_registers_t *oregs =
		(osc32kctrl_registers_t *)DT_REG_ADDR(DT_NODELABEL(osc32kctrl));

	oregs->OSC32KCTRL_XOSC32K = OSC32KCTRL_XOSC32K_ENABLE(1) | OSC32KCTRL_XOSC32K_XTALEN(1) |
				    OSC32KCTRL_XOSC32K_EN32K(1) | OSC32KCTRL_XOSC32K_RUNSTDBY(1) |
				    OSC32KCTRL_XOSC32K_STARTUP(PIC32CXSG_XOSC32K_STARTUP_TIME);

	while ((oregs->OSC32KCTRL_STATUS & BIT(OSC32KCTRL_STATUS_XOSC32KRDY_Pos)) == 0) {
	}

	gregs->GCLK_GENCTRL[1] = GCLK_GENCTRL_SRC(GCLK_SOURCE_XOSC32K) | GCLK_GENCTRL_RUNSTDBY(1) |
				 GCLK_GENCTRL_GENEN(1);
}
#else
static void osc32k_init(void)
{
	gclk_registers_t *gregs = (gclk_registers_t *)DT_REG_ADDR(DT_NODELABEL(gclk));

	gregs->GCLK_GENCTRL[1] = GCLK_GENCTRL_SRC(GCLK_SOURCE_OSCULP32K) |
				 GCLK_GENCTRL_RUNSTDBY(1) | GCLK_GENCTRL_GENEN(1);
}
#endif

static void dpll_init(uint8_t n, uint32_t f_cpu)
{
	gclk_registers_t *gregs = (gclk_registers_t *)DT_REG_ADDR(DT_NODELABEL(gclk));
	oscctrl_registers_t *oscc_regs = (oscctrl_registers_t *)(OSCCTRL_BASE_ADDRESS);
	/* We source the DPLL from 32kHz GCLK1 */
	const uint32_t ldr = ((f_cpu << 5) / SOC_MICROCHIP_PIC32CXSG_OSC32K_FREQ_HZ);
	uint32_t dpll_rdy_msk =
		BIT(OSCCTRL_DPLLSTATUS_CLKRDY_Pos) | BIT(OSCCTRL_DPLLSTATUS_LOCK_Pos);

	/* disable the DPLL before changing the configuration */
	oscc_regs->DPLL[n].OSCCTRL_DPLLCTRLA &= ~BIT(OSCCTRL_DPLLCTRLA_ENABLE_Pos);

	while (oscc_regs->DPLL[n].OSCCTRL_DPLLSYNCBUSY != 0) {
	}

	/* set DPLL clock source to 32kHz GCLK1 */
	gregs->GCLK_PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + n] =
		(GCLK_PCHCTRL_GEN(1) | BIT(GCLK_PCHCTRL_CHEN_Pos));

	while ((gregs->GCLK_PCHCTRL[OSCCTRL_GCLK_ID_FDPLL0 + n] & BIT(GCLK_PCHCTRL_CHEN_Pos)) ==
	       0) {
	};

	oscc_regs->DPLL[n].OSCCTRL_DPLLRATIO =
		OSCCTRL_DPLLRATIO_LDRFRAC(ldr & 0x1F) | OSCCTRL_DPLLRATIO_LDR((ldr >> 5) - 1);

	/* Without LBYPASS, startup takes very long, see errata section 2.13. */
	oscc_regs->DPLL[n].OSCCTRL_DPLLCTRLB = OSCCTRL_DPLLCTRLB_REFCLK_GCLK |
					       OSCCTRL_DPLLCTRLB_WUF(1) |
					       OSCCTRL_DPLLCTRLB_LBYPASS(1);

	oscc_regs->DPLL[n].OSCCTRL_DPLLCTRLA = OSCCTRL_DPLLCTRLA_ENABLE(1);

	while (oscc_regs->DPLL[n].OSCCTRL_DPLLSYNCBUSY != 0) {
	}

	while ((oscc_regs->DPLL[n].OSCCTRL_DPLLSTATUS & dpll_rdy_msk) != dpll_rdy_msk) {
	}
}

static void dfll_init(void)
{
	oscctrl_registers_t *oscc_regs = (oscctrl_registers_t *)(OSCCTRL_BASE_ADDRESS);
	uint32_t reg = OSCCTRL_DFLLCTRLB_QLDIS(1)
#ifdef OSCCTRL_DFLLCTRLB_WAITLOCK
		       | OSCCTRL_DFLLCTRLB_WAITLOCK(1)
#endif
		;

	oscc_regs->OSCCTRL_DFLLCTRLB = reg;
	oscc_regs->OSCCTRL_DFLLCTRLA = OSCCTRL_DFLLCTRLA_ENABLE(1);

	while ((oscc_regs->OSCCTRL_STATUS & BIT(OSCCTRL_STATUS_DFLLRDY_Pos)) == 0) {
	}
}

static void gclk_reset(void)
{
	gclk_registers_t *gregs = (gclk_registers_t *)DT_REG_ADDR(DT_NODELABEL(gclk));

	gregs->GCLK_CTRLA |= BIT(GCLK_CTRLA_SWRST_Pos);

	while ((gregs->GCLK_SYNCBUSY & BIT(GCLK_SYNCBUSY_SWRST_Pos)) != 0) {
	}
}

static void gclk_connect(uint8_t gclk, uint8_t src, uint8_t div)
{
	gclk_registers_t *gregs = (gclk_registers_t *)DT_REG_ADDR(DT_NODELABEL(gclk));

	gregs->GCLK_GENCTRL[gclk] =
		(GCLK_GENCTRL_SRC(src) | GCLK_GENCTRL_DIV(div) | GCLK_GENCTRL_GENEN(1));
}

void soc_reset_hook(void)
{
	cmcc_registers_t *cmcc_regs = (cmcc_registers_t *)(CMCC_BASE_ADDRESS);
	uint8_t dfll_div = 1;

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
	cmcc_regs->CMCC_CTRL &= ~BIT(CMCC_CTRL_CEN_Pos);

	gclk_reset();
	osc32k_init();
	dfll_init();
	dpll_init(0, dfll_div * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* use DPLL for main clock */
	gclk_connect(0, GCLK_SOURCE_DPLL0, dfll_div);

	/* connect GCLK2 to 48 MHz DFLL for USB */
	gclk_connect(2, GCLK_SOURCE_DFLL48M, 0);
}

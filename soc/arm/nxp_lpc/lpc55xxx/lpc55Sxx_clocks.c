/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <fsl_clock.h>
#include <fsl_power.h>


/* Checks if a clock output is enabled */
#define CLOCK_ENABLED(nodelabel) \
	(DT_NODE_HAS_STATUS(DT_NODELABEL(nodelabel), okay))

/* Intermediate function to get clock mux selection */
#define __CLOCK_MUX_SEL(node, prop, idx) \
	(DT_SAME_NODE(DT_PROP(node, sel), DT_PHANDLE_BY_IDX(node, prop, idx)) ? idx : 0)

/* Extracts 0 based index of clock mux selection */
#define CLOCK_MUX(nodelabel) \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(nodelabel), okay), \
	((DT_FOREACH_PROP_ELEM_SEP(\
	DT_NODELABEL(nodelabel), input_sources, __CLOCK_MUX_SEL, (|)))), \
	(0))

/* Gets clock div */
#define CLOCK_DIV(nodelabel) \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(nodelabel), okay), \
	(DT_PROP(DT_NODELABEL(nodelabel), div)), \
	(1))

#define CLOCK_MULT(nodelabel) \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(nodelabel), okay), \
	(DT_PROP(DT_NODELABEL(nodelabel), mult)), \
	(1))

/* Sets clock div if clock is enabled */
#define CLOCK_SETDIV(clk_enum, nodelabel) \
	if (CLOCK_ENABLED(nodelabel)) { \
		CLOCK_SetClkDiv(clk_enum, CLOCK_DIV(nodelabel), false); \
	}
/* Macro for FRG clock divs, which need div value modified */
#define FRG_CLOCK_SETDIV(clk_enum, nodelabel) \
	if (CLOCK_ENABLED(nodelabel)) { \
		CLOCK_SetClkDiv(clk_enum, CLOCK_DIV(nodelabel) - 256, false); \
	}
/* Sets clock mux if clock is enabled */
#define CLOCK_SETMUX(clk_id, nodelabel) \
	if (CLOCK_ENABLED(nodelabel)) { \
		CLOCK_AttachClk(CLK_ATTACH_ID(clk_id, CLOCK_MUX(nodelabel), 0)); \
	}

int clock_init(void)
{
	status_t res;

	/* Enable FRO12, and use this as a safe main clock source while we
	 * reconfigure the clock tree.
	 */
	res = CLOCK_SetupFROClocking(DT_PROP(DT_NODELABEL(fro_12m), clock_frequency));
	if (res != kStatus_Success) {
		return -EINVAL;
	}
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Enable Clock sources */
	if (CLOCK_ENABLED(xtal32m)) {
		/* Enable external 16MHz crystal clock */
		res = CLOCK_SetupExtClocking(DT_PROP(DT_NODELABEL(xtal32m), clock_frequency));
		if (res != kStatus_Success) {
			return -EINVAL;
		}
	}
	if (CLOCK_ENABLED(clk_in_en)) {
		/* Enable system clock from XTAL */
		ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;
	}
	if (CLOCK_ENABLED(clk_usb_en)) {
		/* Enable usb clock from XTAL */
		ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT_MASK;
	}

	if (CLOCK_ENABLED(fro_1m)) {
		/* Enable 1MHZ FRO output */
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
	}

	if (CLOCK_ENABLED(fro_hf)) {
		/* Setup FROHF (FRO96) */
		res = CLOCK_SetupFROClocking(DT_PROP(DT_NODELABEL(fro_hf), clock_frequency));
		if (res != kStatus_Success) {
			return -EINVAL;
		}
	}

	if (CLOCK_ENABLED(fro_32k)) {
		/* Power up FRO32K */
		POWER_DisablePD(kPDRUNCFG_PD_FRO32K);
	}
	if (CLOCK_ENABLED(xtal32k)) {
		/* Power up XTAL32K */
		POWER_DisablePD(kPDRUNCFG_PD_XTAL32K);
	}
	if (CLOCK_ENABLED(fro_32k) || CLOCK_ENABLED(xtal32k)) {
		/* Enable RTC clock */
		CLOCK_EnableClock(kCLOCK_Rtc);
		RTC->CTRL &= ~RTC_CTRL_SWRESET_MASK;
	}

	if (CLOCK_ENABLED(ostimer32khzclk)) {
		/* Enable 32K clock output to OSTimer */
		CLOCK_EnableOstimer32kClock();
	}
	if (CLOCK_ENABLED(rtc_1hz_clk)) {
		/* Enable 1Hz RTC clock */
		RTC->CTRL |= RTC_CTRL_RTC_EN_MASK;
	}
	if (CLOCK_ENABLED(rtc_1khz_clk)) {
		/* Enable 1KHz RTC clock */
		RTC->CTRL |= RTC_CTRL_RTC1KHZ_EN_MASK;
	}

	if (CLOCK_ENABLED(utickclk)) {
		/* Enable UTICK clock */
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_UTICK_ENA_MASK;
	}
	if (CLOCK_ENABLED(pluglitch1mhzclk) || CLOCK_ENABLED(pluglitch12mhzclk)) {
		/* Enable PLU glitch clock */
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_PLU_DEGLITCH_CLK_ENA_MASK;
	}
	if (CLOCK_ENABLED(plu_clkin)) {
		/* Enable PLU external input clock */
		CLOCK_SetupPLUClkInClocking(DT_PROP(DT_NODELABEL(plu_clkin), clock_frequency));
	}
	if (CLOCK_ENABLED(mclk_in)) {
		/* Enable MCLK external input clock */
		CLOCK_SetupI2SMClkClocking(DT_PROP(DT_NODELABEL(mclk_in), clock_frequency));
	}

	/* Setup pre-pll clock muxes */
	/* 32K oscillator mux */
	CLOCK_SETMUX(CM_RTCOSC32KCLKSEL, rtcosc32ksel);
	/* PLL0 input mux */
	CLOCK_SETMUX(CM_PLL0CLKSEL, pll0clksel);
	if (CLOCK_ENABLED(pll0_directo)) {
		/* PLL0 direct output mux */
		SYSCON->PLL0CTRL |= CLOCK_MUX(pll0_directo) ?
			SYSCON_PLL0CTRL_BYPASSPOSTDIV2_MASK : 0;
	}
	if (CLOCK_ENABLED(pll0_bypass)) {
		/* PLL0 bypass mux */
		CLOCK_SetBypassPLL0(CLOCK_MUX(pll0_bypass));
	}
	/* PLL1 input mux */
	CLOCK_SETMUX(CM_PLL1CLKSEL, pll1clksel);
	if (CLOCK_ENABLED(pll1_directo)) {
		/* PLL1 direct output mux */
		SYSCON->PLL1CTRL |= CLOCK_MUX(pll1_directo) ?
			SYSCON_PLL1CTRL_BYPASSPOSTDIV2_MASK : 0;
	}
	if (CLOCK_ENABLED(pll1_bypass)) {
		/* PLL1 bypass mux */
		CLOCK_SetBypassPLL1(CLOCK_MUX(pll1_bypass));
	}

	/* Setup PLLs */
	if (CLOCK_ENABLED(pll0)) {
		/* Get PLL rate */
		uint32_t pllrate = CLOCK_GetPLL0InClockRate();

		pllrate *= CLOCK_MULT(pll0);
		pllrate /= CLOCK_DIV(pll0);
		if (CLOCK_MUX(pll0_directo) == 0) {
			/* PLL0 is routed through postdiv */
			pllrate /= CLOCK_DIV(pll0_pdec);
		}
		/* Do PLL setup */
		pll_setup_t pll0setup = {
			/* Values for SELI and SELP taken from MCUX SDK clock tool */
			.pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK |
				SYSCON_PLL0CTRL_SELI(15U) |
				SYSCON_PLL0CTRL_SELP(7U),
			.pllndec = SYSCON_PLL0NDEC_NDIV(CLOCK_DIV(pll0)),
			.pllpdec = SYSCON_PLL0PDEC_PDIV(CLOCK_DIV(pll0_pdec) / 2),
			.pllsscg = {0x0,
				(SYSCON_PLL0SSCG1_MDIV_EXT(CLOCK_MULT(pll0)) |
				SYSCON_PLL0SSCG1_SEL_EXT_MASK)},
			.pllRate = pllrate,
			.flags = PLL_SETUPFLAG_WAITLOCK,
		};
		if (CLOCK_SetPLL0Freq(&pll0setup) != kStatus_PLL_Success) {
			return -EINVAL;
		}
	} else if (CLOCK_ENABLED(pll0_bypass) && CLOCK_MUX(pll0_bypass)) {
		/* PLL0 is bypassed, but CLKEN mask still needs to be set */
		SYSCON->PLL0CTRL |= SYSCON_PLL0CTRL_CLKEN_MASK;
	} else {
		/* PLL is fully powered down */
		POWER_EnablePD(kPDRUNCFG_PD_PLL0);
		POWER_EnablePD(kPDRUNCFG_PD_PLL0_SSCG);
	}

	if (CLOCK_ENABLED(pll1)) {
		/* Get PLL rate */
		uint32_t pllrate = CLOCK_GetPLL1InClockRate();

		pllrate *= CLOCK_MULT(pll1);
		pllrate /= CLOCK_DIV(pll1);
		if (CLOCK_MUX(pll1_directo) == 0) {
			/* PLL1 is routed through postdiv */
			pllrate /= CLOCK_DIV(pll1_pdec);
		}
		/* Do PLL setup */
		pll_setup_t pll1setup = {
			/* Values for SELI and SELP taken from MCUX SDK clock tool */
			.pllctrl = SYSCON_PLL1CTRL_CLKEN_MASK |
				SYSCON_PLL1CTRL_SELI(19U) |
				SYSCON_PLL1CTRL_SELP(9U),
			.pllndec = SYSCON_PLL1NDEC_NDIV(CLOCK_DIV(pll1)),
			.pllpdec = SYSCON_PLL1PDEC_PDIV(CLOCK_DIV(pll1_pdec) / 2),
			.pllmdec = SYSCON_PLL1MDEC_MDIV(CLOCK_MULT(pll1)),
			.pllRate = pllrate,
			.flags = PLL_SETUPFLAG_WAITLOCK,
		};
		if (CLOCK_SetPLL1Freq(&pll1setup) != kStatus_PLL_Success) {
			return -EINVAL;
		}
	} else if (CLOCK_ENABLED(pll1_bypass) && CLOCK_MUX(pll1_bypass)) {
		/* PLL1 is bypassed, but CLKEN mask still needs to be set */
		SYSCON->PLL1CTRL |= SYSCON_PLL1CTRL_CLKEN_MASK;
	} else {
		/* PLL is fully powered down */
		POWER_EnablePD(kPDRUNCFG_PD_PLL1);
	}

	/* This is a reimplementation of CLOCK_GetCoreSysClkFreq(), since
	 * the SYSCON MAINCLKSEL register values won't be set at this point,
	 * but we need to calculate the resulting core clock frequency.
	 */
	if (CLOCK_MUX(mainclkselb) == 0) {
		/* Read mainclksela mux */
		if (CLOCK_MUX(mainclksela) == 0) {
			SystemCoreClock = CLOCK_GetFro12MFreq();
		} else if (CLOCK_MUX(mainclksela) == 1) {
			SystemCoreClock = CLOCK_GetExtClkFreq();
		} else if (CLOCK_MUX(mainclksela) == 2) {
			SystemCoreClock = CLOCK_GetFro1MFreq();
		} else if (CLOCK_MUX(mainclksela) == 3) {
			SystemCoreClock = CLOCK_GetFroHfFreq();
		} else {
			return -EINVAL;
		}
	} else if (CLOCK_MUX(mainclkselb) == 1) {
		SystemCoreClock = CLOCK_GetPll0OutFreq();
	} else if (CLOCK_MUX(mainclkselb) == 2) {
		SystemCoreClock = CLOCK_GetPll1OutFreq();
	} else if (CLOCK_MUX(mainclkselb) == 3) {
		SystemCoreClock = CLOCK_GetOsc32KFreq();
	} else {
		return -EINVAL;
	}
	SystemCoreClock /= CLOCK_DIV(ahbclkdiv);

	/* Before we reconfigured to a (potentially faster) core clock,
	 * we must set the flash wait state count and voltage levels
	 * for the new core frequency.
	 */
	POWER_SetVoltageForFreq(SystemCoreClock);
	/* CONFIG_TRUSTED_EXECUTION_NONSECURE is checked
	 * since the non-secure core will not have access to the flash
	 * as this will cause a secure fault to occur
	 */
	if (!IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE)) {
		CLOCK_SetFLASHAccessCyclesForFreq(SystemCoreClock);
	}

	/* Setup clock divs */
	/* AHB clock div */
	CLOCK_SETDIV(kCLOCK_DivAhbClk, ahbclkdiv);
	/* Trace clock div */
	CLOCK_SETDIV(kCLOCK_DivArmTrClkDiv, traceclkdiv);
	/* Systick core 0 clock div */
	CLOCK_SETDIV(kCLOCK_DivSystickClk0, systickclkdiv0);
	/* Systick core 1 clock div */
	CLOCK_SETDIV(kCLOCK_DivSystickClk1, systickclkdiv1);
	/* WDT clock div */
	CLOCK_SETDIV(kCLOCK_DivWdtClk, wdtclkdiv);
	/* ADC clock div */
	CLOCK_SETDIV(kCLOCK_DivAdcAsyncClk, adcclkdiv);
	/* USB0 (FS) clock div */
	CLOCK_SETDIV(kCLOCK_DivUsb0Clk, usb0clkdiv);
	/* MCLK clock div */
	CLOCK_SETDIV(kCLOCK_DivMClk, mclkdiv);
	/* SCT clock div */
	CLOCK_SETDIV(kCLOCK_DivSctClk, sctclkdiv);
	/* CLKOUT clock div */
	CLOCK_SETDIV(kCLOCK_DivClkOut, clkoutdiv);
	/* SDIO clock div */
	CLOCK_SETDIV(kCLOCK_DivSdioClk, sdioclkdiv);
	/* PLL0DIV divider */
	CLOCK_SETDIV(kCLOCK_DivPll0Clk, pll0div);
	/* FROHF divider */
	CLOCK_SETDIV(kCLOCK_DivFrohfClk, frohfdiv);
	/* Flexcomm FRG dividers. Note the input to the FRG is multiplied by 256 */
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg0, frgctrl0_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg1, frgctrl1_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg2, frgctrl2_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg3, frgctrl3_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg4, frgctrl4_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg5, frgctrl5_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg6, frgctrl6_div);
	FRG_CLOCK_SETDIV(kCLOCK_DivFlexFrg7, frgctrl7_div);

	/* Setup post-pll clock muxes */
	/* Main clock select A mux */
	CLOCK_SETMUX(CM_MAINCLKSELA, mainclksela);
	/* Main clock select B mux */
	CLOCK_SETMUX(CM_MAINCLKSELB, mainclkselb);
	/* Trace clock select mux */
	CLOCK_SETMUX(CM_TRACECLKSEL, traceclksel);
	/* Core 0 systick clock */
	CLOCK_SETMUX(CM_SYSTICKCLKSEL0, systickclksel0);
	/* Core 1 systick clock */
	CLOCK_SETMUX(CM_SYSTICKCLKSEL1, systickclksel1);
	/* ADC clock mux */
	CLOCK_SETMUX(CM_ADCASYNCCLKSEL, adcclksel);
	/* USB0 (FS) clock mux */
	CLOCK_SETMUX(CM_USB0CLKSEL, usb0clksel);
	/* MCLK clock mux */
	CLOCK_SETMUX(CM_MCLKCLKSEL, mclkclksel);
	/* SCT clock mux */
	CLOCK_SETMUX(CM_SCTCLKSEL, sctclksel);
	/* CLKOUT clock mux */
	CLOCK_SETMUX(CM_CLKOUTCLKSEL, clkoutsel);
	/* SDIO clock mux */
	CLOCK_SETMUX(CM_SDIOCLKSEL, sdioclksel);
	/* CTIMER clock muxes */
	CLOCK_SETMUX(CM_CTIMERCLKSEL0, ctimerclksel0);
	CLOCK_SETMUX(CM_CTIMERCLKSEL1, ctimerclksel1);
	CLOCK_SETMUX(CM_CTIMERCLKSEL2, ctimerclksel2);
	CLOCK_SETMUX(CM_CTIMERCLKSEL3, ctimerclksel3);
	CLOCK_SETMUX(CM_CTIMERCLKSEL4, ctimerclksel4);
	/* Flexcomm clock muxes */
	CLOCK_SETMUX(CM_FXCOMCLKSEL0, fcclksel0);
	CLOCK_SETMUX(CM_FXCOMCLKSEL1, fcclksel1);
	CLOCK_SETMUX(CM_FXCOMCLKSEL2, fcclksel2);
	CLOCK_SETMUX(CM_FXCOMCLKSEL3, fcclksel3);
	CLOCK_SETMUX(CM_FXCOMCLKSEL4, fcclksel4);
	CLOCK_SETMUX(CM_FXCOMCLKSEL5, fcclksel5);
	CLOCK_SETMUX(CM_FXCOMCLKSEL6, fcclksel6);
	CLOCK_SETMUX(CM_FXCOMCLKSEL7, fcclksel7);
	/* HSLSPI clock mux */
	CLOCK_SETMUX(CM_HSLSPICLKSEL, hslspiclksel);

	if ((!CLOCK_ENABLED(fro_hf)) && (!CLOCK_ENABLED(fro_12m)) && (!CLOCK_ENABLED(fro_1m))) {
		/* No consumers of FRO192M clock. We previously used this as
		 * safe clock source, so power it down.
		 */
		POWER_EnablePD(kPDRUNCFG_PD_FRO192M);
	}

	return 0;
}

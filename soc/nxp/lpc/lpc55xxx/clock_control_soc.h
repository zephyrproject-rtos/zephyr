/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_LPC_55xxx_CLOCK_CONTROL_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_LPC_55xxx_CLOCK_CONTROL_SOC_H_

#include <fsl_clock.h>
#include <fsl_power.h>
#include <zephyr/dt-bindings/clock/nxp_lpc55xxx_clocks.h>

/*
 * Setpoints are implemented as functions, which can be called by passing an
 * integer to the "control_control_configure" function.
 * The function is defined entirely with in a macro so it can be defined for
 * multiple setpoints.
 *
 * This has the following advantages:
 * - significantly less flash space. Since all of the clock setup is known
 *   at compile time, function calls that are not used are optimized out, and
 *   only needed clock setup calls remain
 * - since all clock configuration for a given setpoint is available within
 *   the macro, power gating decisions can be made within the function (IE
 *   powering down a PLL)
 *
 * Of course, the disadvantage is how *ugly* this implementation is.
 * Unfortunately, I'm not aware of another approach that enables conserving
 * flash space as effectively.
 */

#define LPC_CLOCK_ENABLED(node, clk) DT_PROP(node, clk##_enable)
#define LPC_CLOCK_FREQ(node, clk) DT_PROP_OR(node, clk##_freq, 0)
#define LPC_CLOCK_PRESENT(node, clk) DT_NODE_HAS_PROP(node, clk)
#define LPC_CLOCK_MUX(node, prop) DT_PROP_OR(node, prop, 0)
#define LPC_CLOCK_SETMUX(node, clk_id, prop)					\
	if (LPC_CLOCK_PRESENT(node, prop)) {					\
		CLOCK_AttachClk(CLK_ATTACH_ID(clk_id,				\
				LPC_CLOCK_MUX(node, prop), 0));			\
	}
#define LPC_CLOCK_DIV(node, prop) DT_PROP_OR(node, prop, 1)
#define LPC_CLOCK_SETDIV(node, clk_enum, prop)					\
	if (LPC_CLOCK_PRESENT(node, prop)) {					\
		CLOCK_SetClkDiv(clk_enum, LPC_CLOCK_DIV(node, prop), false);	\
	}
/* Macro for FRG clock divs, which need div value modified */
#define FRG_LPC_CLOCK_SETDIV(node, clk_enum, prop) \
	if (LPC_CLOCK_PRESENT(node, prop)) { \
		CLOCK_SetClkDiv(clk_enum, LPC_CLOCK_DIV(node, prop) - 256, false); \
	}
#define LPC_PLL_CFG(node, pll)							\
	COND_CODE_1(DT_PROP_HAS_IDX(node, pll, 1),				\
		    ((((uint64_t)DT_PROP_BY_IDX(node, pll, 0)) << 32) |		\
		    DT_PROP_BY_IDX(node, pll, 1)), (0))

#define CLOCK_CONTROL_SETPOINT_GET(node)					\
	UTIL_CAT(nxp_lpc55xxx_setpoint_, DT_NODE_CHILD_IDX(node))

#define CLOCK_CONTROL_SETPOINT_DEFINE(node)					\
int UTIL_CAT(nxp_lpc55xxx_setpoint_, DT_NODE_CHILD_IDX(node))(void) {		\
	status_t res;								\
	/* Enable FRO12, and use this as a safe main clock source */		\
	res = CLOCK_SetupFROClocking(MHZ(12));					\
	if (res != kStatus_Success) {						\
		return -EINVAL;							\
	}									\
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);					\
										\
	if (LPC_CLOCK_ENABLED(node, pluglitch12mhzclk) ||			\
	    LPC_CLOCK_ENABLED(node, pluglitch1mhzclk)) {			\
		/* Enable PLU glitch clock */					\
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_PLU_DEGLITCH_CLK_ENA_MASK; \
	}									\
	if (LPC_CLOCK_FREQ(node, xtal32m)) {					\
		/* Enable external 32mhz clock */				\
		res = CLOCK_SetupExtClocking(LPC_CLOCK_FREQ(node, xtal32m));	\
		if (res != kStatus_Success) {					\
			return -EINVAL;						\
		}								\
	}									\
	if (LPC_CLOCK_ENABLED(node, clk_in_en)) {				\
		/* Enable system clock from XTAL */				\
		ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK; \
	}									\
	if (LPC_CLOCK_ENABLED(node, clk_usb_en)) {				\
		/* Enable usb clock from XTAL */				\
		ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT_MASK; \
	}									\
	if (LPC_CLOCK_ENABLED(node, fro_1m)) {					\
		/* Enable 1MHZ FRO output */					\
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;	\
	}									\
	if (LPC_CLOCK_ENABLED(node, fro_hf)) {					\
		/* Setup FROHF (FRO96) */					\
		res = CLOCK_SetupFROClocking(MHZ(96));				\
		if (res != kStatus_Success) {					\
			return -EINVAL;						\
		}								\
	}									\
	if (LPC_CLOCK_ENABLED(node, fro_1m)) {					\
		/* Enable 1MHZ FRO output */					\
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;	\
	}									\
										\
	if (LPC_CLOCK_ENABLED(node, utickclk)) {				\
		/* Enable UTICK clock */					\
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_UTICK_ENA_MASK;	\
	}									\
	if (LPC_CLOCK_ENABLED(node, fro_32k)) {					\
		/* Power up FRO32K */						\
		POWER_DisablePD(kPDRUNCFG_PD_FRO32K);				\
	}									\
	if (LPC_CLOCK_FREQ(node, xtal32k)) {					\
		/* Power up XTAL32K */						\
		POWER_DisablePD(kPDRUNCFG_PD_XTAL32K);				\
	}									\
	if (LPC_CLOCK_ENABLED(node, fro_32k) || LPC_CLOCK_FREQ(node, xtal32k)) { \
		/* Enable RTC clock */						\
		CLOCK_EnableClock(kCLOCK_Rtc);					\
		RTC->CTRL &= ~RTC_CTRL_SWRESET_MASK;				\
	}									\
	if (LPC_CLOCK_FREQ(node, mclk_in)) {					\
		/* Enable MCLK external input clock */				\
		CLOCK_SetupI2SMClkClocking(LPC_CLOCK_FREQ(node, mclk_in));	\
	}									\
	if (LPC_CLOCK_FREQ(node, plu_clkin)) {					\
		/* Enable PLU external input clock */				\
		CLOCK_SetupPLUClkInClocking(LPC_CLOCK_FREQ(node, plu_clkin));	\
	}									\
	if (LPC_CLOCK_ENABLED(node, ostimer32khzclk)) {				\
		/* Enable 32K clock output to OSTimer */			\
		CLOCK_EnableOstimer32kClock();					\
	}									\
	if (LPC_CLOCK_ENABLED(node, rtc_1hz_clk)) {				\
		/* Enable 1Hz RTC clock */					\
		RTC->CTRL |= RTC_CTRL_RTC_EN_MASK;				\
	}									\
	if (LPC_CLOCK_ENABLED(node, rtc_1khz_clk)) {				\
		/* Enable 1KHz RTC clock */					\
		RTC->CTRL |= RTC_CTRL_RTC1KHZ_EN_MASK;				\
	}									\
	/* Setup pre-pll clock muxes */						\
	/* 32K oscillator mux */						\
	LPC_CLOCK_SETMUX(node, CM_RTCOSC32KCLKSEL, rtcosc32ksel);		\
	/* PLL0 input mux */							\
	LPC_CLOCK_SETMUX(node, CM_PLL0CLKSEL, pll0clksel);			\
	if (LPC_CLOCK_PRESENT(node, pll0_directo)) {				\
		/* Set PLL0 direct output mux */				\
		SYSCON->PLL0CTRL |= LPC_CLOCK_MUX(node, pll0_directo) ?		\
			SYSCON_PLL0CTRL_BYPASSPOSTDIV2_MASK : 0;		\
	}									\
	if (LPC_CLOCK_PRESENT(node, pll0_bypass)) {				\
		/* PLL0 bypass mux */						\
		CLOCK_SetBypassPLL0(LPC_CLOCK_MUX(node, pll0_bypass));		\
	}									\
	/* PLL1 input mux */							\
	LPC_CLOCK_SETMUX(node, CM_PLL1CLKSEL, pll1clksel);			\
	if (LPC_CLOCK_PRESENT(node, pll1_directo)) {				\
		/* PLL1 direct output mux */					\
		SYSCON->PLL1CTRL |= LPC_CLOCK_MUX(node, pll1_directo) ?		\
			SYSCON_PLL1CTRL_BYPASSPOSTDIV2_MASK : 0;		\
	}									\
	if (LPC_CLOCK_PRESENT(node, pll1_bypass)) {				\
		/* PLL1 bypass mux */						\
		CLOCK_SetBypassPLL1(LPC_CLOCK_MUX(node, pll1_bypass));		\
	}									\
	/* Setup PLLs */							\
	if (LPC_CLOCK_PRESENT(node, pll0)) {					\
		/* Get PLL rate */						\
		uint32_t pllrate = CLOCK_GetPLL0InClockRate();			\
		uint64_t pll_cfg = LPC_PLL_CFG(node, pll0);			\
										\
		pllrate *= LPC_CLOCK_PLL_MULT(pll_cfg);				\
		pllrate /= LPC_CLOCK_PLL_DIV(pll_cfg);				\
		if (LPC_CLOCK_MUX(node, pll0_directo) == 0) {			\
			/* PLL0 is routed through postdiv */			\
			pllrate /= LPC_CLOCK_PLL_PDEC(pll_cfg);			\
		}								\
		/* Do PLL setup */						\
		pll_setup_t pll0setup = {					\
			.pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK |			\
				SYSCON_PLL0CTRL_SELI(LPC_CLOCK_PLL_SELI(pll_cfg)) | \
				SYSCON_PLL0CTRL_SELP(LPC_CLOCK_PLL_SELP(pll_cfg)) | \
				SYSCON_PLL0CTRL_SELR(LPC_CLOCK_PLL_SELR(pll_cfg)), \
			.pllndec = SYSCON_PLL0NDEC_NDIV(LPC_CLOCK_PLL_DIV(pll_cfg)), \
			.pllpdec = SYSCON_PLL0PDEC_PDIV(LPC_CLOCK_PLL_PDEC(pll_cfg) / 2), \
			.pllsscg = {0x0,					\
				(SYSCON_PLL0SSCG1_MDIV_EXT(LPC_CLOCK_PLL_MULT(pll_cfg)) | \
				SYSCON_PLL0SSCG1_SEL_EXT_MASK)},		\
			.pllRate = pllrate,					\
			.flags = PLL_SETUPFLAG_WAITLOCK,			\
		};								\
		if (CLOCK_SetPLL0Freq(&pll0setup) != kStatus_PLL_Success) {	\
			return -EINVAL;						\
		}								\
	} else if (LPC_CLOCK_PRESENT(node, pll0_bypass) &&			\
		   LPC_CLOCK_MUX(node, pll0_bypass)) {				\
		/* PLL0 is bypassed, but CLKEN mask still needs to be set */	\
		SYSCON->PLL0CTRL |= SYSCON_PLL0CTRL_CLKEN_MASK;			\
	} else {								\
		/* PLL is fully powered down */					\
		POWER_EnablePD(kPDRUNCFG_PD_PLL0);				\
		POWER_EnablePD(kPDRUNCFG_PD_PLL0_SSCG);				\
	}									\
	if (LPC_CLOCK_PRESENT(node, pll1)) {					\
		/* Get PLL rate */						\
		uint32_t pllrate = CLOCK_GetPLL1InClockRate();			\
		uint64_t pll_cfg = LPC_PLL_CFG(node, pll1);			\
										\
		pllrate *= LPC_CLOCK_PLL_MULT(pll_cfg);				\
		pllrate /= LPC_CLOCK_PLL_DIV(pll_cfg);				\
		if (LPC_CLOCK_MUX(node, pll1_directo) == 0) {			\
			/* PLL1 is routed through postdiv */			\
			pllrate /= LPC_CLOCK_PLL_PDEC(pll_cfg);			\
		}								\
		/* Do PLL setup */						\
		pll_setup_t pll1setup = {					\
			.pllctrl = SYSCON_PLL1CTRL_CLKEN_MASK |			\
				SYSCON_PLL1CTRL_SELI(LPC_CLOCK_PLL_SELI(pll_cfg)) | \
				SYSCON_PLL1CTRL_SELP(LPC_CLOCK_PLL_SELP(pll_cfg)) | \
				SYSCON_PLL1CTRL_SELR(LPC_CLOCK_PLL_SELR(pll_cfg)), \
			.pllndec = SYSCON_PLL1NDEC_NDIV(LPC_CLOCK_PLL_DIV(pll_cfg)), \
			.pllpdec = SYSCON_PLL1PDEC_PDIV(LPC_CLOCK_PLL_PDEC(pll_cfg) / 2), \
			.pllRate = pllrate,					\
			.flags = PLL_SETUPFLAG_WAITLOCK,			\
		};								\
		if (CLOCK_SetPLL1Freq(&pll1setup) != kStatus_PLL_Success) {	\
			return -EINVAL;						\
		}								\
	} else if (LPC_CLOCK_PRESENT(node, pll1_bypass) &&			\
		   LPC_CLOCK_MUX(node, pll1_bypass)) {				\
		/* PLL1 is bypassed, but CLKEN mask still needs to be set */	\
		SYSCON->PLL1CTRL |= SYSCON_PLL1CTRL_CLKEN_MASK;			\
	} else {								\
		/* PLL is fully powered down */					\
		POWER_EnablePD(kPDRUNCFG_PD_PLL1);				\
	}									\
	/* This is a reimplementation of CLOCK_GetCoreSysClkFreq(), since */	\
	/* the SYSCON MAINCLKSEL register values won't be set at this point. */	\
	/* but we need to calculate the resulting core clock frequency. */	\
	if (LPC_CLOCK_PRESENT(node, mainclkselb)) {				      \
		if (LPC_CLOCK_MUX(node, mainclkselb) == 0) {			\
			/* Read mainclksela mux */				\
			if (LPC_CLOCK_MUX(node, mainclksela) == 0) {		\
				SystemCoreClock = CLOCK_GetFro12MFreq();	\
			} else if (LPC_CLOCK_MUX(node, mainclksela) == 1) {	\
				SystemCoreClock = CLOCK_GetExtClkFreq();	\
			} else if (LPC_CLOCK_MUX(node, mainclksela) == 2) {	\
				SystemCoreClock = CLOCK_GetFro1MFreq();		\
			} else if (LPC_CLOCK_MUX(node, mainclksela) == 3) {	\
				SystemCoreClock = CLOCK_GetFroHfFreq();		\
			} else {						\
				return -EINVAL;					\
			}							\
		} else if (LPC_CLOCK_MUX(node, mainclkselb) == 1) {		\
			SystemCoreClock = CLOCK_GetPll0OutFreq();		\
		} else if (LPC_CLOCK_MUX(node, mainclkselb) == 2) {		\
			SystemCoreClock = CLOCK_GetPll1OutFreq();		\
		} else if (LPC_CLOCK_MUX(node, mainclkselb) == 3) {		\
			SystemCoreClock = CLOCK_GetOsc32KFreq();		\
		} else {							\
			return -EINVAL;						\
		}								\
		SystemCoreClock /= LPC_CLOCK_DIV(node, ahbclkdiv);		\
										\
		/* Before we reconfigured to a (potentially faster) core */	\
		/* clock we must set the flash wait state count and voltage */	\
		/* level for the new core frequency. */				\
		POWER_SetVoltageForFreq(SystemCoreClock);			\
		/* CONFIG_TRUSTED_EXECUTION_NONSECURE is checked */		\
		/* since the non-secure core will not have access to the */	\
		/* flash, and cannot configure this parameter */		\
		if (!IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE)) {		\
			CLOCK_SetFLASHAccessCyclesForFreq(SystemCoreClock);	\
		}								\
	}									\
	/* Setup clock divs */							\
	/* AHB clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivAhbClk, ahbclkdiv);			\
	/* Trace clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivArmTrClkDiv, traceclkdiv);		\
	/* Systick core 0 clock div */						\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivSystickClk0, systickclkdiv0);		\
	/* Systick core 1 clock div */						\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivSystickClk1, systickclkdiv1);		\
	/* WDT clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivWdtClk, wdtclkdiv);			\
	/* ADC clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivAdcAsyncClk, adcclkdiv);		\
	/* USB0 (FS) clock div */						\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivUsb0Clk, usb0clkdiv);			\
	/* MCLK clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivMClk, mclkdiv);			\
	/* SCT clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivSctClk, sctclkdiv);			\
	/* CLKOUT clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivClkOut, clkoutdiv);			\
	/* SDIO clock div */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivSdioClk, sdioclkdiv);			\
	/* PLL0DIV divider */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivPll0Clk, pll0div);			\
	/* FROHF divider */							\
	LPC_CLOCK_SETDIV(node, kCLOCK_DivFrohfClk, frohfdiv);			\
	/* Flexcomm FRG divs. Note the input to the FRG is multiplied by 256 */ \
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg0, frgctrl0_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg1, frgctrl1_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg2, frgctrl2_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg3, frgctrl3_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg4, frgctrl4_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg5, frgctrl5_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg6, frgctrl6_div);		\
	FRG_LPC_CLOCK_SETDIV(node, kCLOCK_DivFlexFrg7, frgctrl7_div);		\
										\
	/* Setup post-pll clock muxes */					\
	/* Main clock select A mux */						\
	LPC_CLOCK_SETMUX(node, CM_MAINCLKSELA, mainclksela);			\
	/* Main clock select B mux */						\
	LPC_CLOCK_SETMUX(node, CM_MAINCLKSELB, mainclkselb);			\
	/* Trace clock select mux */						\
	LPC_CLOCK_SETMUX(node, CM_TRACECLKSEL, traceclksel);			\
	/* Core 0 systick clock */						\
	LPC_CLOCK_SETMUX(node, CM_SYSTICKCLKSEL0, systickclksel0);		\
	/* Core 1 systick clock */						\
	LPC_CLOCK_SETMUX(node, CM_SYSTICKCLKSEL1, systickclksel1);		\
	/* ADC clock mux */							\
	LPC_CLOCK_SETMUX(node, CM_ADCASYNCCLKSEL, adcclksel);			\
	/* USB0 (FS) clock mux */						\
	LPC_CLOCK_SETMUX(node, CM_USB0CLKSEL, usb0clksel);			\
	/* MCLK clock mux */							\
	LPC_CLOCK_SETMUX(node, CM_MCLKCLKSEL, mclkclksel);			\
	/* SCT clock mux */							\
	LPC_CLOCK_SETMUX(node, CM_SCTCLKSEL, sctclksel);			\
	/* CLKOUT clock mux */							\
	LPC_CLOCK_SETMUX(node, CM_CLKOUTCLKSEL, clkoutsel);			\
	/* SDIO clock mux */							\
	LPC_CLOCK_SETMUX(node, CM_SDIOCLKSEL, sdioclksel);			\
	/* CTIMER clock muxes */						\
	LPC_CLOCK_SETMUX(node, CM_CTIMERCLKSEL0, ctimerclksel0);		\
	LPC_CLOCK_SETMUX(node, CM_CTIMERCLKSEL1, ctimerclksel1);		\
	LPC_CLOCK_SETMUX(node, CM_CTIMERCLKSEL2, ctimerclksel2);		\
	LPC_CLOCK_SETMUX(node, CM_CTIMERCLKSEL3, ctimerclksel3);		\
	LPC_CLOCK_SETMUX(node, CM_CTIMERCLKSEL4, ctimerclksel4);		\
	/* Flexcomm clock muxes */						\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL0, fcclksel0);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL1, fcclksel1);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL2, fcclksel2);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL3, fcclksel3);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL4, fcclksel4);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL5, fcclksel5);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL6, fcclksel6);			\
	LPC_CLOCK_SETMUX(node, CM_FXCOMCLKSEL7, fcclksel7);			\
	/* HSLSPI clock mux */							\
	LPC_CLOCK_SETMUX(node, CM_HSLSPICLKSEL, hslspiclksel);			\
	if ((!LPC_CLOCK_ENABLED(node, fro_hf)) && (!LPC_CLOCK_ENABLED(node, fro_12m)) && \
	    (!LPC_CLOCK_ENABLED(node, fro_1m))) {				\
		/* No consumers of FRO192M clock. We previously used this as */ \
		/* safe clock source, so power it down. */			\
		POWER_EnablePD(kPDRUNCFG_PD_FRO192M);				\
	}									\
	return 0;								\
}

#endif /* ZEPHYR_SOC_ARM_NXP_LPC_55xxx_CLOCK_CONTROL_SOC_H_ */

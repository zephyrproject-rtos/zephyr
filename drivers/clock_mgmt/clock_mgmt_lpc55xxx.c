/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef int (*clock_mgmt_soc_subsys_t)(void);
typedef int (*clock_mgmt_soc_state_t)(void);

#include <fsl_clock.h>
#include <fsl_power.h>
#include "lpc55sxx_clocks.h"
#include <zephyr/drivers/clock_mgmt.h>

/* Helper macros, used to apply clock configuration */
#define LPC_CLOCK_SET_MUX_CB(node, pha, idx, clock_id, mux_id, output_id) \
	CLOCK_AttachClk(MUX_A(mux_id, DT_PHA_BY_IDX(node, pha, idx, selector))); \
	CLOCK_MGMT_FIRE_CALLBACK(output_id, CLOCK_MGMT_RATE_CHANGED);

#define LPC_CLOCK_SET_MUX(node, pha, idx, clock_id, mux_id) \
	CLOCK_AttachClk(MUX_A(mux_id, DT_PHA_BY_IDX(node, pha, idx, selector))); \

#define LPC_CLOCK_MUX(node, state, clock_id) \
	DT_CLOCK_STATE_ID_READ_CELL_OR(node, clock_id, selector, state, 0)

#define LPC_CLOCK_DIV(node, state, clock_id) \
	DT_CLOCK_STATE_ID_READ_CELL_OR(node, clock_id, divider, state, 1)

#define LPC_CLOCK_SET_DIV_CB(node, pha, idx, clock_id, div_id, output_id) \
	CLOCK_SetClkDiv(div_id, DT_PHA_BY_IDX(node, pha, idx, divider), false); \
	CLOCK_MGMT_FIRE_CALLBACK(output_id, CLOCK_MGMT_RATE_CHANGED);

#define LPC_CLOCK_SET_DIV(node, pha, idx, clock_id, div_id) \
	CLOCK_SetClkDiv(div_id, DT_PHA_BY_IDX(node, pha, idx, divider), false)

#define FRG_LPC_CLOCK_SET_DIV_CB(node, pha, idx, clock_id, div_id, output_id) \
	CLOCK_SetClkDiv(div_id, DT_PHA_BY_IDX(node, pha, idx, divider) - 256, false); \
	CLOCK_MGMT_FIRE_CALLBACK(output_id, CLOCK_MGMT_RATE_CHANGED);

#define LPC_CLOCK_PLL0_SSCG(node, state)			\
	.pllsscg = {0x0,					\
		(SYSCON_PLL0SSCG1_MDIV_EXT(			\
		DT_CLOCK_STATE_ID_READ_CELL_OR(node, PLL0,	\
		multiplier, state, 0)) |			\
		SYSCON_PLL0SSCG1_SEL_EXT_MASK)},		\

#define LPC_CLOCK_PLL1_SSCG(node, state)

#define LPC_CLOCK_PLL0_SSCG_POWER POWER_EnablePD(kPDRUNCFG_PD_PLL0_SSCG);
#define LPC_CLOCK_PLL1_SSCG_POWER

#define LPC_CLOCK_SETUP_PLL(node, state, pll)					\
	if (DT_CLOCK_STATE_HAS_ID(node, state, pll)) {				\
		/* Get PLL rate */						\
		uint32_t pllrate = CLOCK_Get##pll##InClockRate();		\
										\
		notify_all_consumers = true;					\
										\
		pllrate *= DT_CLOCK_STATE_ID_READ_CELL_OR(node,			\
			pll, multiplier, state, 0);				\
		pllrate /= LPC_CLOCK_DIV(node, state, pll);			\
		if (DT_CLOCK_STATE_ID_READ_CELL_OR(node,			\
			pll##_DIRECT0, selector, state, 0)) {			\
			/* PLL is routed through postdiv */			\
			pllrate /= DT_CLOCK_STATE_ID_READ_CELL_OR(node,		\
					pll, pdec, state, 1);			\
		}								\
		/* Do PLL setup */						\
		pll_setup_t pllsetup = {					\
			.pllctrl = SYSCON_##pll##CTRL_CLKEN_MASK |			\
				SYSCON_##pll##CTRL_SELI(DT_CLOCK_STATE_ID_READ_CELL_OR( \
						node, pll, seli, state, 0)) |	\
				SYSCON_##pll##CTRL_SELP(DT_CLOCK_STATE_ID_READ_CELL_OR( \
						node, pll, selp, state, 0)) |	\
				SYSCON_##pll##CTRL_SELR(DT_CLOCK_STATE_ID_READ_CELL_OR( \
						node, pll, selr, state, 0)),	\
			.pllndec = SYSCON_##pll##NDEC_NDIV(LPC_CLOCK_DIV(node, state, pll)), \
			.pllpdec = SYSCON_##pll##NDEC_NDIV(DT_CLOCK_STATE_ID_READ_CELL_OR(node, \
							pll, pdec, state, 1)), \
			LPC_CLOCK_##pll##_SSCG(node, state)			\
			.pllRate = pllrate,					\
			.flags = PLL_SETUPFLAG_WAITLOCK,			\
		};								\
		if (CLOCK_Set##pll##Freq(&pllsetup) != kStatus_PLL_Success) {	\
			return -EINVAL;						\
		}								\
	} else if (DT_CLOCK_STATE_HAS_ID(node, state, pll##_BYPASS) &&		\
		   LPC_CLOCK_MUX(node, state, pll##_BYPASS)) { \
		/* PLL is bypassed, but CLKEN mask still needs to be set */	\
		notify_all_consumers = true;					\
		SYSCON->pll##CTRL |= SYSCON_##pll##CTRL_CLKEN_MASK;		\
	} else if (LPC_CLOCK_MUX(node, state, pll##CLKSEL) > 3) {	\
		/* PLL input selector is not clocked, power down PLL */		\
		POWER_EnablePD(kPDRUNCFG_PD_##pll);				\
		LPC_CLOCK_##pll##_SSCG_POWER;					\
	}									\

/*
 * This template declares a clock subsystem rate handler. The parameters
 * passed to this template are as follows:
 * @param node: node ID for device with clocks property
 * @param prop: clocks property of node
 * @param idx: index of the clock subsystem within the clocks property
 */
Z_CLOCK_MGMT_SUBSYS_TEMPL(node, prop, idx)
{
	switch (CONCAT(NXP_CLOCK_, DT_STRING_TOKEN(
		DT_PHANDLE_BY_IDX(node, clocks, idx), clock_id))) {
	case NXP_CLOCK_FXCOM0_CLOCK:
		return CLOCK_GetFlexCommClkFreq(0);
	case NXP_CLOCK_FXCOM1_CLOCK:
		return CLOCK_GetFlexCommClkFreq(1);
	case NXP_CLOCK_FXCOM2_CLOCK:
		return CLOCK_GetFlexCommClkFreq(2);
	case NXP_CLOCK_FXCOM3_CLOCK:
		return CLOCK_GetFlexCommClkFreq(2);
	case NXP_CLOCK_FXCOM4_CLOCK:
		return CLOCK_GetFlexCommClkFreq(4);
	case NXP_CLOCK_FXCOM5_CLOCK:
		return CLOCK_GetFlexCommClkFreq(5);
	case NXP_CLOCK_FXCOM6_CLOCK:
		return CLOCK_GetFlexCommClkFreq(6);
	case NXP_CLOCK_FXCOM7_CLOCK:
		return CLOCK_GetFlexCommClkFreq(7);
	default:
		return -ENOTSUP;
	}
}

/*
 * This template declares a clock management setpoint. The parameters
 * passed to this template are as follows:
 * @param node: node ID for device with clock-control-state-<n> property
 * @param state: identifier for current state
 */
Z_CLOCK_MGMT_SETPOINT_TEMPL(node, state)
{
	status_t res = kStatus_Success;
	bool notify_all_consumers = false;

	if (DT_CLOCK_STATE_HAS_ID(node, state, PLUGLITCH12MHZCLK)) {
		/* Enable PLU Glitch clock */
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_PLU_DEGLITCH_CLK_ENA_MASK;
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, XTAL32M)) {
		/* Enable external 32Mhz clock */
		res = CLOCK_SetupExtClocking(DT_CLOCK_STATE_ID_READ_CELL_OR(
					node, XTAL32M, freq, state, 0));
		if (res != kStatus_Success) {
			return res;
		}
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, CLK_IN_EN)) {
		/* Enable system clock from XTAL */
		ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;
		CLOCK_MGMT_FIRE_CALLBACK(CLK_IN_EN, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, CLK_USB_EN)) {
		/* Enable USB clock from XTAL */
		ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_PLL_USB_OUT_MASK;
		CLOCK_MGMT_FIRE_CALLBACK(CLK_USB_EN, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, FRO_1M)) {
		/* Enable 1MHZ FRO output */
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_CLK_ENA_MASK;
		CLOCK_MGMT_FIRE_CALLBACK(FRO_1M, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, FRO_HF)) {
		/* Setup FROHF (FRO96) */
		res = CLOCK_SetupFROClocking(MHZ(96));
		if (res != kStatus_Success) {
			return -EINVAL;
		}
		CLOCK_MGMT_FIRE_CALLBACK(FRO_HF, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, UTICKCLK)) {
		/* Enable UTICK clock */
		SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_FRO1MHZ_UTICK_ENA_MASK;
		CLOCK_MGMT_FIRE_CALLBACK(UTICKCLK, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, FRO_32K)) {
		if (DT_CLOCK_STATE_ID_READ_CELL_OR(node, FRO_32K,
		    freq, state, 0) != 32000) {
			return -ENOTSUP;
		}
		/* Power up FRO32K */
		POWER_DisablePD(kPDRUNCFG_PD_FRO32K);
		CLOCK_MGMT_FIRE_CALLBACK(FRO_32K, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, XTAL32K)) {
		if (DT_CLOCK_STATE_ID_READ_CELL_OR(node, XTAL32K,
		    freq, state, 0) != 32000) {
			return -ENOTSUP;
		}
		/* Power up XTAL32K */
		POWER_DisablePD(kPDRUNCFG_PD_XTAL32K);
		CLOCK_MGMT_FIRE_CALLBACK(XTAL32K, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, FRO_32K) ||
	    DT_CLOCK_STATE_HAS_ID(node, state, XTAL32K)) {
		/* Enable RTC clock */
		CLOCK_EnableClock(kCLOCK_Rtc);
		RTC->CTRL &= ~RTC_CTRL_SWRESET_MASK;
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, MCLK_IN)) {
		/* Enable MCLK external input clock */
		CLOCK_SetupI2SMClkClocking(DT_CLOCK_STATE_ID_READ_CELL_OR(node,
					   MCLK_IN, freq, state, 0));
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLU_CLKIN)) {
		/* Enable PLU external input clock */
		CLOCK_SetupPLUClkInClocking(DT_CLOCK_STATE_ID_READ_CELL_OR(node,
					   PLU_CLKIN, freq, state, 0));
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, OSTIMER32KHZCLK)) {
		/* Enable 32K clock output to OSTimer */
		CLOCK_EnableOstimer32kClock();
		CLOCK_MGMT_FIRE_CALLBACK(OSTIMER32KHZCLK, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, RTC_1HZ_CLK)) {
		/* Enable 1Hz RTC clock */
		RTC->CTRL |= RTC_CTRL_RTC_EN_MASK;
		CLOCK_MGMT_FIRE_CALLBACK(RTC_1HZ_CLK, CLOCK_MGMT_STARTED);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, RTC_1KHZ_CLK)) {
		/* Enable 1KHz RTC clock */
		RTC->CTRL |= RTC_CTRL_RTC1KHZ_EN_MASK;
		CLOCK_MGMT_FIRE_CALLBACK(RTC_1KHZ_CLK, CLOCK_MGMT_STARTED);
	}
	/* Setup pre-pll clock muxes */
	/* 32K oscillator mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, RTCOSC32KSEL, LPC_CLOCK_SET_MUX,
				      state, CM_RTCOSC32KSEL);
	/* PLL0 input mux */
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLL0CLKSEL)) {
		notify_all_consumers = true;
		DT_CLOCK_STATE_APPLY_ID_VARGS(node, PLL0CLKSEL,
					      LPC_CLOCK_SET_MUX,
					      state, CM_PLL0CLKSEL);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLL0_DIRECTO)) {
		notify_all_consumers = true;
		/* Set PLL0 direct output mux */
		SYSCON->PLL0CTRL |= LPC_CLOCK_MUX(node, state, PLL0_DIRECT0) ?
			SYSCON_PLL0CTRL_BYPASSPOSTDIV2_MASK : 0;
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLL0_BYPASS)) {
		notify_all_consumers = true;
		/* PLL0 bypass mux */
		CLOCK_SetBypassPLL0(LPC_CLOCK_MUX(node, state, PLL0_BYPASS));
	}
	/* PLL1 input mux */
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLL1CLKSEL)) {
		notify_all_consumers = true;
		DT_CLOCK_STATE_APPLY_ID_VARGS(node, PLL1CLKSEL,
					      LPC_CLOCK_SET_MUX,
					      state, CM_PLL1CLKSEL);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLL1_BYPASS)) {
		notify_all_consumers = true;
		/* PLL1 bypass mux */
		CLOCK_SetBypassPLL1(LPC_CLOCK_MUX(node, state, PLL1_BYPASS));
	}
	/* Setup PLLs */
	LPC_CLOCK_SETUP_PLL(node, state, PLL0);
	LPC_CLOCK_SETUP_PLL(node, state, PLL1);
	/* This is a reimplementation of CLOCK_GetCoreSysClkFreq(), since
	 * the SYSCON MAINCLKSEL register values won't be set at this point.
	 * but we need to calculate the resulting core clock frequency.
	 */
	if (DT_CLOCK_STATE_HAS_ID(node, state, MAINCLKSELB)) {
		notify_all_consumers = true;
		if (LPC_CLOCK_MUX(node, state, MAINCLKSELB) == 0) {
			/* Read MAINCLKSELA mux */
			if (LPC_CLOCK_MUX(node, state, MAINCLKSELA) == 0) {
				SystemCoreClock = CLOCK_GetFro12MFreq();
			} else if (LPC_CLOCK_MUX(node, state, MAINCLKSELA) == 1) {
				SystemCoreClock = CLOCK_GetExtClkFreq();
			} else if (LPC_CLOCK_MUX(node, state, MAINCLKSELA) == 2) {
				SystemCoreClock = CLOCK_GetFro1MFreq();
			} else if (LPC_CLOCK_MUX(node, state, MAINCLKSELA) == 3) {
				SystemCoreClock = CLOCK_GetFroHfFreq();
			} else {
				return -EINVAL;
			}
		} else if (LPC_CLOCK_MUX(node, state, MAINCLKSELB) == 1) {
			SystemCoreClock = CLOCK_GetPll0OutFreq();
		} else if (LPC_CLOCK_MUX(node, state, MAINCLKSELB) == 2) {
			SystemCoreClock = CLOCK_GetPll1OutFreq();
		} else if (LPC_CLOCK_MUX(node, state, MAINCLKSELB) == 3) {
			SystemCoreClock = CLOCK_GetOsc32KFreq();
		} else {
			return -EINVAL;
		}
		SystemCoreClock /= LPC_CLOCK_DIV(node, state, AHBCLKDIV);

		/* Before we reconfigured to a (potentially faster) core
		 * clock we must set the flash wait state count and voltage
		 * level for the new core frequency.
		 */
		POWER_SetVoltageForFreq(SystemCoreClock);
		/* CONFIG_TRUSTED_EXECUTION_NONSECURE is checked
		 * since the non-secure core will not have access to the
		 * flash, and cannot configure this parameter
		 */
		if (!IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE)) {
			CLOCK_SetFLASHAccessCyclesForFreq(SystemCoreClock);
		}
	}
	/* Setup clock divs */
	/* AHB clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, AHBCLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivAhbClk, SYSTEM_CLOCK);
	/* Trace clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, TRACECLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivArmTrClkDiv, TRACE_CLOCK);
	/* Systick core 0 clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SYSTICKCLKDIV0, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivSystickClk0, SYSTICK0_CLOCK);
	/* Systick core 1 clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SYSTICKCLKDIV1, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivSystickClk1, SYSTICK1_CLOCK);
	/* WDT clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, WDTCLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivWdtClk, WDT_CLOCK);
	/* ADC clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, ADCCLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivAdcAsyncClk, ASYNCADC_CLOCK);
	/* USB0 (FS) clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, USB0CLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivUsb0Clk, USB0_CLOCK);
	/* MCLK clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, MCLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivMClk, MCLK_CLOCK);
	/* SCT clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SCTCLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivSctClk, SCT_CLOCK);
	/* CLKOUT clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CLKOUTDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivClkOut, CLKOUT_CLOCK);
	/* SDIO clock div */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SDIOCLKDIV, LPC_CLOCK_SET_DIV_CB,
				      state, kCLOCK_DivSdioClk, SDIO_CLOCK);
	if (DT_CLOCK_STATE_HAS_ID(node, state, PLL0DIV)) {
		/* PLL0DIV divider */
		notify_all_consumers = true;
		CLOCK_SetClkDiv(kCLOCK_DivPll0Clk,
				LPC_CLOCK_DIV(node, state, PLL0DIV), false);
	}
	if (DT_CLOCK_STATE_HAS_ID(node, state, FROHFDIV)) {
		/* FROHF divider */
		notify_all_consumers = true;
		CLOCK_SetClkDiv(kCLOCK_DivFrohfClk,
				LPC_CLOCK_DIV(node, state, FROHFDIV), false);
	}
	/* Flexcomm FRG divs. Note the input to the FRG is multiplied by 256 */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL0_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg0, FXCOM0_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL1_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg1, FXCOM1_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL2_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg2, FXCOM2_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL3_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg3, FXCOM3_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL4_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg4, FXCOM4_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL5_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg5, FXCOM5_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL6_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg6, FXCOM6_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FRGCTRL7_DIV,
				      FRG_LPC_CLOCK_SET_DIV_CB, state,
				      kCLOCK_DivFlexFrg7, FXCOM7_CLOCK);

	/* Setup post-pll clock muxes */
	/* Main clock select A mux */
	if (DT_CLOCK_STATE_HAS_ID(node, state, MAINCLKSELA)) {
		notify_all_consumers = true;
		DT_CLOCK_STATE_APPLY_ID_VARGS(node, MAINCLKSELA, LPC_CLOCK_SET_MUX,
					      state, CM_MAINCLKSELA);
	}
	/* Main clock select B mux */
	if (DT_CLOCK_STATE_HAS_ID(node, state, MAINCLKSELB)) {
		DT_CLOCK_STATE_APPLY_ID_VARGS(node, MAINCLKSELB, LPC_CLOCK_SET_MUX,
				      state, CM_MAINCLKSELB);
	}
	/* Trace clock select mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, TRACECLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_TRACECLKSEL, TRACE_CLOCK);
	/* Core 0 systick clock */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SYSTICKCLKSEL0, LPC_CLOCK_SET_MUX_CB,
				      state, CM_SYSTICKCLKSEL0, SYSTICK0_CLOCK);
	/* Core 1 systick clock */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SYSTICKCLKSEL1, LPC_CLOCK_SET_MUX_CB,
				      state, CM_SYSTICKCLKSEL1, SYSTICK1_CLOCK);
	/* ADC clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, ADCCLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_ADCASYNCCLKSEL, ASYNCADC_CLOCK);
	/* USB0 (FS) clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, USB0CLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_USB0CLKSEL, USB0_CLOCK);
	/* MCLK clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, MCLKCLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_MCLKCLKSEL, MCLK_CLOCK);
	/* SCT clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SCTCLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_SCTCLKSEL, SCT_CLOCK);
	/* CLKOUT clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CLKOUTSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_CLKOUTCLKSEL, CLKOUT_CLOCK);
	/* SDIO clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, SDIOCLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_SDIOCLKSEL, SDIO_CLOCK);
	/* CTIMER clock muxes */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CTIMERCLKSEL0, LPC_CLOCK_SET_MUX_CB,
				      state, CM_CTIMERCLKSEL0, CTIMER0_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CTIMERCLKSEL1, LPC_CLOCK_SET_MUX_CB,
				      state, CM_CTIMERCLKSEL1, CTIMER1_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CTIMERCLKSEL2, LPC_CLOCK_SET_MUX_CB,
				      state, CM_CTIMERCLKSEL2, CTIMER2_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CTIMERCLKSEL3, LPC_CLOCK_SET_MUX_CB,
				      state, CM_CTIMERCLKSEL3, CTIMER3_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, CTIMERCLKSEL4, LPC_CLOCK_SET_MUX_CB,
				      state, CM_CTIMERCLKSEL4, CTIMER4_CLOCK);
	/* Flexcomm clock muxes */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL0, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL0, FXCOM0_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL1, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL1, FXCOM1_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL2, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL2, FXCOM2_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL3, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL3, FXCOM3_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL4, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL4, FXCOM4_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL5, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL5, FXCOM5_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL6, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL6, FXCOM6_CLOCK);
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, FCCLKSEL7, LPC_CLOCK_SET_MUX_CB,
				      state, CM_FXCOMCLKSEL7, FXCOM7_CLOCK);
	/* HSLSPI clock mux */
	DT_CLOCK_STATE_APPLY_ID_VARGS(node, HSLSPICLKSEL, LPC_CLOCK_SET_MUX_CB,
				      state, CM_HSLSPICLKSEL, HSLSPI_CLOCK);
	if (notify_all_consumers) {
		CLOCK_MGMT_FIRE_ALL_CALLBACKS(CLOCK_MGMT_RATE_CHANGED);
	}
	return res;
}

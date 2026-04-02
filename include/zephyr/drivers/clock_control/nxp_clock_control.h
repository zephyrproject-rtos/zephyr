/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
#include <zephyr/dt-bindings/clock/nxp_mc_cgm.h>
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(firc), nxp_firc, okay)
#define NXP_FIRC_DIV DT_ENUM_IDX(DT_NODELABEL(firc), firc_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
#define NXP_FXOSC_FREQ DT_PROP(DT_NODELABEL(fxosc), freq)
#define NXP_FXOSC_WORKMODE                                                                         \
	(DT_ENUM_IDX(DT_NODELABEL(fxosc), workmode) == 0 ? kFXOSC_ModeCrystal : kFXOSC_ModeBypass)
#define NXP_FXOSC_DELAY     DT_PROP(DT_NODELABEL(fxosc), delay)
#define NXP_FXOSC_OVERDRIVE DT_PROP(DT_NODELABEL(fxosc), overdrive)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
#define NXP_PLL_WORKMODE       DT_ENUM_IDX(DT_NODELABEL(pll), workmode)
#define NXP_PLL_PREDIV         DT_PROP(DT_NODELABEL(pll), prediv)
#define NXP_PLL_POSTDIV        DT_PROP(DT_NODELABEL(pll), postdiv)
#define NXP_PLL_MULTIPLIER     DT_PROP(DT_NODELABEL(pll), multiplier)
#define NXP_PLL_FRACLOOPDIV    DT_PROP(DT_NODELABEL(pll), fracloopdiv)
#define NXP_PLL_STEPSIZE       DT_PROP(DT_NODELABEL(pll), stepsize)
#define NXP_PLL_STEPNUM        DT_PROP(DT_NODELABEL(pll), stepnum)
#define NXP_PLL_ACCURACY       DT_ENUM_IDX(DT_NODELABEL(pll), accuracy)
#define NXP_PLL_OUTDIV_POINTER DT_PROP(DT_NODELABEL(pll), outdiv)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
#define NXP_PLL_MAXIDOCHANGE   DT_PROP(DT_NODELABEL(mc_cgm), max_ido_change)
#define NXP_PLL_STEPDURATION   DT_PROP(DT_NODELABEL(mc_cgm), step_duration)
#define NXP_PLL_CLKSRCFREQ     DT_PROP(DT_NODELABEL(mc_cgm), clk_src_freq)
#define NXP_PLL_MUX_0_DC_0_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_0_div)
#define NXP_PLL_MUX_0_DC_1_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_1_div)
#define NXP_PLL_MUX_0_DC_2_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_2_div)
#define NXP_PLL_MUX_0_DC_3_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_3_div)
#define NXP_PLL_MUX_0_DC_4_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_4_div)
#define NXP_PLL_MUX_0_DC_5_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_5_div)
#define NXP_PLL_MUX_0_DC_6_DIV DT_PROP(DT_NODELABEL(mc_cgm), mux_0_dc_6_div)
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_CLOCK_CONTROL_H_ */

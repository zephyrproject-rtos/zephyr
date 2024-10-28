/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SL_CLOCK_MANAGER_OSCILLATOR_CONFIG_H
#define SL_CLOCK_MANAGER_OSCILLATOR_CONFIG_H

#include <zephyr/devicetree.h>

#include <em_device.h>

/* HFXO */
#define SL_CLOCK_MANAGER_HFXO_EN                 DT_NODE_HAS_STATUS(DT_NODELABEL(hfxo), okay)
#define SL_CLOCK_MANAGER_HFXO_MODE               DT_ENUM_IDX(DT_NODELABEL(hfxo), mode)
#define SL_CLOCK_MANAGER_HFXO_FREQ               DT_PROP(DT_NODELABEL(hfxo), clock_frequency)
#define SL_CLOCK_MANAGER_HFXO_CTUNE              DT_PROP(DT_NODELABEL(hfxo), ctune)
#define SL_CLOCK_MANAGER_HFXO_PRECISION          DT_PROP(DT_NODELABEL(hfxo), precision)
#define SL_CLOCK_MANAGER_HFXO_CRYSTAL_SHARING_EN 0

/* LFXO */
#define SL_CLOCK_MANAGER_LFXO_EN   DT_NODE_HAS_STATUS(DT_NODELABEL(lfxo), okay)
#define SL_CLOCK_MANAGER_LFXO_MODE (DT_ENUM_IDX(DT_NODELABEL(lfxo), mode) << _LFXO_CFG_MODE_SHIFT)

#define SL_CLOCK_MANAGER_LFXO_CTUNE     DT_PROP(DT_NODELABEL(lfxo), ctune)
#define SL_CLOCK_MANAGER_LFXO_PRECISION DT_PROP(DT_NODELABEL(lfxo), precision)
#define SL_CLOCK_MANAGER_LFXO_TIMEOUT                                                              \
	(DT_ENUM_IDX(DT_NODELABEL(lfxo), timeout) << _LFXO_CFG_TIMEOUT_SHIFT)

/* HFRCODPLL */
#define SL_CLOCK_MANAGER_HFRCO_BAND                                                                \
	(DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 1500000    ? 1000000U                 \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 3000000  ? 2000000U                 \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 5500000  ? 4000000U                 \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 10000000 ? 7000000U                 \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 14500000 ? 13000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 17500000 ? 16000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 23000000 ? 19000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 29000000 ? 26000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 35000000 ? 32000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 44000000 ? 38000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 52000000 ? 48000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 60000000 ? 56000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 72000000 ? 64000000U                \
	 : DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency) < 90000000 ? 80000000U                \
									: 100000000U)
#define SL_CLOCK_MANAGER_HFRCO_DPLL_EN DT_NUM_CLOCKS(DT_NODELABEL(hfrcodpll))
#define SL_CLOCK_MANAGER_DPLL_FREQ     DT_PROP(DT_NODELABEL(hfrcodpll), clock_frequency)
#define SL_CLOCK_MANAGER_DPLL_N        DT_PROP(DT_NODELABEL(hfrcodpll), dpll_n)
#define SL_CLOCK_MANAGER_DPLL_M        DT_PROP(DT_NODELABEL(hfrcodpll), dpll_m)
#define SL_CLOCK_MANAGER_DPLL_REFCLK                                                               \
	(DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(hfrcodpll)), DT_NODELABEL(hfxo))                 \
		 ? CMU_DPLLREFCLKCTRL_CLKSEL_HFXO                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(hfrcodpll)), DT_NODELABEL(lfxo))               \
		 ? CMU_DPLLREFCLKCTRL_CLKSEL_LFXO                                                  \
	 : DT_SAME_NODE(DT_CLOCKS_CTLR(DT_NODELABEL(hfrcodpll)), DT_NODELABEL(clkin0))             \
		 ? CMU_DPLLREFCLKCTRL_CLKSEL_CLKIN0                                                \
		 : CMU_DPLLREFCLKCTRL_CLKSEL_DISABLED)
#define SL_CLOCK_MANAGER_DPLL_EDGE        DT_ENUM_IDX(DT_NODELABEL(hfrcodpll), dpll_edge)
#define SL_CLOCK_MANAGER_DPLL_LOCKMODE    DT_ENUM_IDX(DT_NODELABEL(hfrcodpll), dpll_lock)
#define SL_CLOCK_MANAGER_DPLL_AUTORECOVER DT_PROP(DT_NODELABEL(hfrcodpll), dpll_autorecover)
#define SL_CLOCK_MANAGER_DPLL_DITHER      DT_PROP(DT_NODELABEL(hfrcodpll), dpll_dither)

/* HFRCOEM23 */
#if DT_NODE_EXISTS(DT_NODELABEL(hfrcoem23))
#define SL_CLOCK_MANAGER_HFRCOEM23_BAND DT_PROP(DT_NODELABEL(hfrcoem23), clock_frequency)
#endif

/* LFRCO */
#if DT_NODE_EXISTS(DT_NODELABEL(lfrco))
#define SL_CLOCK_MANAGER_LFRCO_PRECISION DT_PROP(DT_NODELABEL(lfrco), precision_mode)
#endif

/* CLKIN0 */
#define SL_CLOCK_MANAGER_CLKIN0_FREQ DT_PROP(DT_NODELABEL(clkin0), clock_frequency)

#endif /* SL_CLOCK_MANAGER_OSCILLATOR_CONFIG_H */

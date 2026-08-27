/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock source configuration helpers for NXP devices.
 * @ingroup clock_control_nxp_sources
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_CLOCK_CONTROLLER_SOURCES_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_CLOCK_CONTROLLER_SOURCES_H_

#include <zephyr/drivers/clock_control.h>

/**
 * @defgroup clock_control_nxp_sources NXP clock sources
 * @ingroup clock_control_nxp
 * @{
 */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(firc), nxp_firc, okay)
/** @brief FIRC output divider selection index. */
#define NXP_FIRC_DIV DT_ENUM_IDX(DT_NODELABEL(firc), firc_div)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
/** @brief FXOSC frequency, in Hz. */
#define NXP_FXOSC_FREQ DT_PROP(DT_NODELABEL(fxosc), freq)
/** @brief FXOSC work mode (crystal or bypass). */
#define NXP_FXOSC_WORKMODE                                                                         \
	(DT_ENUM_IDX(DT_NODELABEL(fxosc), workmode) == 0 ? kFXOSC_ModeCrystal : kFXOSC_ModeBypass)
/** @brief FXOSC startup delay. */
#define NXP_FXOSC_DELAY     DT_PROP(DT_NODELABEL(fxosc), delay)
/** @brief FXOSC overdrive setting. */
#define NXP_FXOSC_OVERDRIVE DT_PROP(DT_NODELABEL(fxosc), overdrive)
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
/** @brief PLL work mode selection index. */
#define NXP_PLL_WORKMODE       DT_ENUM_IDX(DT_NODELABEL(pll), workmode)
/** @brief PLL pre-divider. */
#define NXP_PLL_PREDIV         DT_PROP(DT_NODELABEL(pll), prediv)
/** @brief PLL post-divider. */
#define NXP_PLL_POSTDIV        DT_PROP(DT_NODELABEL(pll), postdiv)
/** @brief PLL loop multiplier. */
#define NXP_PLL_MULTIPLIER     DT_PROP(DT_NODELABEL(pll), multiplier)
/** @brief PLL fractional loop divider. */
#define NXP_PLL_FRACLOOPDIV    DT_PROP(DT_NODELABEL(pll), fracloopdiv)
/** @brief PLL modulation step size. */
#define NXP_PLL_STEPSIZE       DT_PROP(DT_NODELABEL(pll), stepsize)
/** @brief PLL modulation step count. */
#define NXP_PLL_STEPNUM        DT_PROP(DT_NODELABEL(pll), stepnum)
/** @brief PLL accuracy selection index. */
#define NXP_PLL_ACCURACY       DT_ENUM_IDX(DT_NODELABEL(pll), accuracy)
/** @brief PLL output divider table pointer. */
#define NXP_PLL_OUTDIV_POINTER DT_PROP(DT_NODELABEL(pll), outdiv)
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_CLOCK_CONTROLLER_SOURCES_H_ */

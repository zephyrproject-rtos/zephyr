/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX Clock Generator Circuit (CGC) definitions for Zephyr.
 * This header provides macro constants for clock source selections
 * and multipliers/dividers for Renesas RX SoCs. These values are used by the
 * DeviceTree clock control subsystem to describe clock configurations.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_

/**
 * @name Renesas RX clock source selection definitions
 * @{
 */

#define RX_CLOCKS_SOURCE_CLOCK_LOCO     0    /**< LOCO clock source */
#define RX_CLOCKS_SOURCE_CLOCK_HOCO     1    /**< HOCO clock source */
#define RX_CLOCKS_SOURCE_CLOCK_MAIN_OSC 2    /**< Main oscillator */
#define RX_CLOCKS_SOURCE_CLOCK_SUBCLOCK 3    /**< Sub-clock oscillator */
#define RX_CLOCKS_SOURCE_PLL            4    /**< PLL clock source */
#define RX_CLOCKS_SOURCE_CLOCK_DISABLE  0xff /**< Clock source disabled. */

/** @} */

/**
 * @name Renesas RX clock source definitions for some peripherals selections
 * @brief Encoded clock source values used by certain peripherals.
 * @{
 */
#define RX_IF_CLOCKS_SOURCE_CLOCK_HOCO 0 /**< HOCO clock source */
#define RX_IF_CLOCKS_SOURCE_CLOCK_LOCO 2 /**< LOCO clock source */
#define RX_IF_CLOCKS_SOURCE_PLL        5 /**< PLL clock source */
#define RX_IF_CLOCKS_SOURCE_PLL2       6 /**< PLL2 clock source */

/** @} */

/**
 * @name Renesas RX clock source selection for Low-power-timer (LPT) definitions
 * @{
 */
#define RX_LPT_CLOCKS_SOURCE_CLOCK_SUBCLOCK       0 /**< Sub-clock oscillator. */
#define RX_LPT_CLOCKS_SOURCE_CLOCK_IWDT_LOW_SPEED 1 /**< IWDT low spped. */
#define RX_LPT_CLOCKS_NON_USE                     2 /**< Not use. */
#define RX_LPT_CLOCKS_SOURCE_CLOCK_LOCO           3 /**< LOCO clock source. */

/** @} */

/**
 * @name Renesas RX PLL clock source selection (RX26T series only)
 * @{
 */
#ifdef CONFIG_SOC_SERIES_RX26T
#define RX_PLL_CLOCKS_SOURCE_CLOCK_MAIN_OSC 0 /**< Main oscillator. */
#define RX_PLL_CLOCKS_SOURCE_CLOCK_HOCO     1 /**< HOCO clock source. */
#endif /* CONFIG_SOC_SERIES_RX26T */

/** @} */

/**
 * @name Renesas RX PLL multiplier definitions
 * @{
 */
#define RX_PLL_MUL_4   7  /**< PLL multiplier ×4 */
#define RX_PLL_MUL_4_5 8  /**< PLL multiplier ×4.5 */
#define RX_PLL_MUL_5   9  /**< PLL multiplier ×5 */
#define RX_PLL_MUL_5_5 10 /**< PLL multiplier ×5.5 */
#define RX_PLL_MUL_6   11 /**< PLL multiplier ×6 */
#define RX_PLL_MUL_6_5 12 /**< PLL multiplier ×6.5 */
#define RX_PLL_MUL_7   13 /**< PLL multiplier ×7 */
#define RX_PLL_MUL_7_5 14 /**< PLL multiplier ×7.5 */
#define RX_PLL_MUL_8   15 /**< PLL multiplier ×7 */

#define RX_PLL_MUL_10   19 /**< PLL multiplier ×10 */
#define RX_PLL_MUL_10_5 20 /**< PLL multiplier ×10.5 */
#define RX_PLL_MUL_11   21 /**< PLL multiplier ×11 */
#define RX_PLL_MUL_11_5 22 /**< PLL multiplier ×11.5 */
#define RX_PLL_MUL_12   23 /**< PLL multiplier ×12 */
#define RX_PLL_MUL_12_5 24 /**< PLL multiplier ×12.5 */
#define RX_PLL_MUL_13   25 /**< PLL multiplier ×13 */
#define RX_PLL_MUL_13_5 26 /**< PLL multiplier ×13.5 */
#define RX_PLL_MUL_14   27 /**< PLL multiplier ×14 */
#define RX_PLL_MUL_14_5 28 /**< PLL multiplier ×14.5 */
#define RX_PLL_MUL_15   29 /**< PLL multiplier ×15 */
#define RX_PLL_MUL_15_5 30 /**< PLL multiplier ×15.5 */
#define RX_PLL_MUL_16   31 /**< PLL multiplier ×16 */
#define RX_PLL_MUL_16_5 32 /**< PLL multiplier ×16.5 */
#define RX_PLL_MUL_17   33 /**< PLL multiplier ×17 */
#define RX_PLL_MUL_17_5 34 /**< PLL multiplier ×17.5 */
#define RX_PLL_MUL_18   35 /**< PLL multiplier ×18 */
#define RX_PLL_MUL_18_5 36 /**< PLL multiplier ×18.5 */
#define RX_PLL_MUL_19   37 /**< PLL multiplier ×19 */
#define RX_PLL_MUL_19_5 38 /**< PLL multiplier ×19.5 */
#define RX_PLL_MUL_20   39 /**< PLL multiplier ×20 */
#define RX_PLL_MUL_20_5 40 /**< PLL multiplier ×20.5 */
#define RX_PLL_MUL_21   41 /**< PLL multiplier ×21 */
#define RX_PLL_MUL_21_5 42 /**< PLL multiplier ×21.5 */
#define RX_PLL_MUL_22   43 /**< PLL multiplier ×22 */
#define RX_PLL_MUL_22_5 44 /**< PLL multiplier ×22.5 */
#define RX_PLL_MUL_23   45 /**< PLL multiplier ×23 */
#define RX_PLL_MUL_23_5 46 /**< PLL multiplier ×23.5 */
#define RX_PLL_MUL_24   47 /**< PLL multiplier ×24 */
#define RX_PLL_MUL_24_5 48 /**< PLL multiplier ×24.5 */
#define RX_PLL_MUL_25   49 /**< PLL multiplier ×25 */
#define RX_PLL_MUL_25_5 50 /**< PLL multiplier ×25.5 */
#define RX_PLL_MUL_26   51 /**< PLL multiplier ×26 */
#define RX_PLL_MUL_26_5 52 /**< PLL multiplier ×26.5 */
#define RX_PLL_MUL_27   53 /**< PLL multiplier ×27 */
#define RX_PLL_MUL_27_5 54 /**< PLL multiplier ×27.5 */
#define RX_PLL_MUL_28   55 /**< PLL multiplier ×28 */
#define RX_PLL_MUL_28_5 56 /**< PLL multiplier ×28.5 */
#define RX_PLL_MUL_29   57 /**< PLL multiplier ×29 */
#define RX_PLL_MUL_29_5 58 /**< PLL multiplier ×29.5 */
#define RX_PLL_MUL_30   59 /**< PLL multiplier ×30 */

/** @} */

/**
 * @name Renesas RX Module Stop Control Register (MSTP) definitions
 * @{
 */
#define MSTPA 0 /**< Module stop control register A */
#define MSTPB 1 /**< Module stop control register B */
#define MSTPC 2 /**< Module stop control register C */
#define MSTPD 3 /**< Module stop control register D */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_ */

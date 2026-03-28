/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IFX_CLOCK_SOURCE_PSOC4XX_H_
#define IFX_CLOCK_SOURCE_PSOC4XX_H_

/*
 * Note : use IFX_PATH_PSOC4_IMO for internal main oscillator as src
 * use IFX_PATH_PSOC4_EXT for External clock as src
 *
 * Usage : input src for clk_hf
 */
#define IFX_PATH_PSOC4_IMO 0
#define IFX_PATH_PSOC4_EXT 1

/*
 * Note : use IFX_PATH_PSOC4_PUMP_GND for No clock, connect to gnd
 * use IFX_PATH_PSOC4_PUMP_IMO for main IMO pump output
 * use IFX_PATH_PSOC4_PUMP_HFCLK for clk_hf pump
 *
 * Usage : input src for clk_pump
 */
#define IFX_PATH_PSOC4_PUMP_GND   0
#define IFX_PATH_PSOC4_PUMP_IMO   1
#define IFX_PATH_PSOC4_PUMP_HFCLK 2

/*
 * @brief Device Tree compatible clock source to Infineon PDL clock source
 *
 * This macro converts Device Tree clock source identifiers (defined in
 * ifx_clock_source_common.h) to the corresponding Infineon PDL API values.
 *
 * Usage : in clock_control_infineon_fixed_factor_clock.c, use this macro
 * when passing clock source values from Device Tree to Infineon PDL APIs.
 * This allows users to use the generic ifx_clock_source_common.h macros
 * directly in Device Tree files while ensuring compatibility with the
 * underlying PDL implementation.
 */
#define IFX_PSOC4_HFCLK_DIV(div)                                                                   \
	((div) == 0   ? CY_SYSCLK_NO_DIV                                                           \
	 : (div) == 1 ? CY_SYSCLK_DIV_2                                                            \
	 : (div) == 3 ? CY_SYSCLK_DIV_4                                                            \
	 : (div) == 7 ? CY_SYSCLK_DIV_8                                                            \
		      : CY_SYSCLK_NO_DIV)

#endif

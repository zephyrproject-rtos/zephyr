/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IFX_CLOCK_SOURCE_PSOC4XX_H_
#define IFX_CLOCK_SOURCE_PSOC4XX_H_

#define IFX_PATH_PSOC4_IMO        0
#define IFX_PATH_PSOC4_EXT        1
#define IFX_PATH_PSOC4_PUMP_GND   0
#define IFX_PATH_PSOC4_PUMP_IMO   1
#define IFX_PATH_PSOC4_PUMP_HFCLK 2

#define IFX_PSOC4_HFCLK_DIV(div)                                                                   \
	((div) == 1   ? CY_SYSCLK_NO_DIV                                                           \
	 : (div) == 2 ? CY_SYSCLK_DIV_2                                                            \
	 : (div) == 4 ? CY_SYSCLK_DIV_4                                                            \
	 : (div) == 8 ? CY_SYSCLK_DIV_8                                                            \
		      : CY_SYSCLK_NO_DIV)

#endif

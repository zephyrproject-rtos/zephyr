/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon clock control driver public API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IFX_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IFX_H_

#include <stdbool.h>
#include <stdint.h>

/** PLL clock type */
#define IFX_CLK_PLL 0

/** High-frequency clock type */
#define IFX_CLK_HF  1

/** Low-frequency clock type */
#define IFX_CLK_LF  2

/** Infineon clock specifier */
struct ifx_clk {
	/** Clock type */
	uint8_t clk;
	/** Clock ID */
	uint8_t clk_id;
};

/** Infineon peripheral clock specifier (peripheral names only its PCLK id) */
struct ifx_clk_peri {
	/** PCLK output id within the peripheral clock group */
	uint16_t pclk_id;
	bool integer_only;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_IFX_H_ */

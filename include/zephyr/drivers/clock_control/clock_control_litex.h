/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control helpers for LiteX MMCM clock generators.
 * @ingroup clock_control_litex
 */

#ifndef CLK_CTRL_LITEX_H
#define CLK_CTRL_LITEX_H

/**
 * @defgroup clock_control_litex LiteX
 * @ingroup clock_control_interface_ext
 * @{
 */

#include <zephyr/types.h>

/** @brief Devicetree node identifier of the LiteX MMCM clock generator. */
#define MMCM			DT_NODELABEL(clock0)
/** @brief Number of clock outputs provided by the MMCM. */
#define NCLKOUT			DT_PROP_LEN(MMCM, clock_output_names)

/**
 * @brief Configuration for a single MMCM clock output.
 */
struct litex_clk_setup {
	uint8_t clkout_nr; /**< Index of the clock output to configure. */
	uint32_t rate;     /**< Frequency to set, in Hz. */
	uint16_t phase;    /**< Phase offset, in degrees. */
	uint8_t duty;      /**< Duty cycle of the clock signal, in percent. */
};

/**
 * @}
 */

#endif /* CLK_CTRL_LITEX_H */

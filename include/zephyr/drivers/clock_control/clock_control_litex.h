/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LiteX Clock Control driver interface
 */

#ifndef CLK_CTRL_LITEX_H
#define CLK_CTRL_LITEX_H

/**
 * @brief LiteX Clock Control driver interface
 * @defgroup clock_control_litex_interface LiteX Clock Control driver interface
 * @brief LiteX Clock Control driver interface
 * @ingroup clock_control_interface
 * @{
 */

#include <zephyr/types.h>

#define MMCM			DT_NODELABEL(clock0)
#define NCLKOUT			DT_PROP_LEN(MMCM, clock_output_names)

/**
 * @brief Structure for interfacing with clock control API
 *
 * @param clkout_nr Number of clock output to be changed
 * @param rate Frequency to set given in Hz
 * @param phase Phase offset in degrees
 * @param duty Duty cycle of clock signal in percent
 *
 */
struct litex_clk_setup {
	uint8_t clkout_nr;
	uint32_t rate;
	uint16_t phase;
	uint8_t duty;
};

/**
 * @}
 */

#endif /* CLK_CTRL_LITEX_H */

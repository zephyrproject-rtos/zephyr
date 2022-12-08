/*
 * Copyright (c) 2022 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Raspberry Pi Pico Clock Control driver interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RPI_PICO_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RPI_PICO_CLOCK_CONTROL_H_

/**
 * @brief Raspberry Pi Pico Clock Control driver interface
 * @defgroup clock_control_rpi_pico_interface Raspberry Pi Pico Clock Control driver interface
 * @brief Raspberry Pi Pico Clock Control driver interface
 * @ingroup clock_control_interface
 * @{
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rpi_pico_clock.h>

/**
 * @brief Structure for interfacing with clock control API
 *
 * @param source Clock source glitchlessly
 * @param aux_source Auxiliary clock source
 * @param source_rate Frequency of the source given in Hz
 * @param rate Frequency to set given in Hz
 *
 */
struct rpi_pico_clk_setup {
	uint32_t source;
	uint32_t aux_source;
	uint32_t source_rate;
	uint32_t rate;
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RPI_PICO_CLOCK_CONTROL_H_ */

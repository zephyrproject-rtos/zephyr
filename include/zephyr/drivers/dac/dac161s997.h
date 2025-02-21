/*
 * Copyright 2025 Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_DAC161S997_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_DAC161S997_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

union dac161s997_status {
	uint8_t raw;
	struct {
		/**
		 * True if the DAC161S997 is unable to maintain the output current.
		 */
		bool current_loop_status: 1;
		/**
		 * Identical to current_loop_status except this bit is sticky.
		 */
		bool loop_status: 1;
		/**
		 * True if a SPI command has not been received within SPI timeout period (default
		 * 100 ms). If this error occurs, it is cleared with a properly formatted write
		 * command to a valid address.
		 */
		bool spi_timeout_error: 1;
		/**
		 * A frame error is caused by an incorrect number of clocks during a register write.
		 * A register write without an integer multiple of 24 clock cycles will cause a
		 * Frame error.
		 */
		bool frame_status: 1;
		/**
		 * Returns the state of the ERR_LVL pin.
		 */
		bool error_level_pin_state: 1;
		/**
		 * DAC resolution register. Always returns 0x7.
		 */
		uint8_t dac_resolution: 3;
	} __packed;
};

/**
 * @typedef dac161s997_error_callback_t
 * @brief Callback to invoke when an error is triggered
 *
 * @param dev Pointer to the device
 * @param status NULL if read was not possible otherwise pointer to status.
 */
typedef void (*dac161s997_error_callback_t)(const struct device *dev,
					    const union dac161s997_status *status);

/**
 * @brief Set callback to invoke when an error is triggered
 *
 * The callback runs in a work queue context and the device is locked while it runs.
 *
 * @param dev Pointer to the device
 * @param cb Callback to invoke when an error is triggered
 *
 * @returns 0 on success and -errno otherwise.
 */
int dac161s997_set_error_callback(const struct device *dev, dac161s997_error_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DAC_DAC161S997_H_ */

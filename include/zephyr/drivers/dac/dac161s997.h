/*
 * Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended DAC API of DAC161S997
 * @ingroup dac161s997_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_DAC161S997_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_DAC161S997_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TI DAC161S997 16-bit 1 channel SPI DAC for 4-20 mA loops
 * @defgroup dac161s997_interface DAC161S997
 * @ingroup dac_interface_ext
 * @{
 */

/**
 * @brief DAC161S997 Status Register.
 *
 * This union represents the contents of the DAC161S997's STATUS register.
 * It provides access to individual status bits as well as the raw register value.
 */
union dac161s997_status {
	/**
	 * Raw 8-bit value of the status register.
	 */
	uint8_t raw;
	/**
	 * Bit-field access to the status register.
	 */
	struct {
		/**
		 * Loop error status (real-time).
		 *
		 * This flag is set to true when the loop supply voltage is too low to support the
		 * programmed output current.
		 */
		bool current_loop_status: 1;
		/**
		 * Loop error status (latched).
		 *
		 * Similar to @ref current_loop_status, except that this flag stays set until the
		 * status register is read or the device is reset.
		 */
		bool loop_status: 1;
		/**
		 * SPI timeout error.
		 *
		 * The default timeout is 100 ms. If this error occurs, it is cleared with a
		 * properly formatted write command to a valid address.
		 */
		bool spi_timeout_error: 1;
		/**
		 * Frame error.
		 *
		 * A frame error is caused by an incorrect number of clocks during a register write.
		 * A register write without an integer multiple of 24 clock cycles will cause a
		 * frame error.
		 */
		bool frame_status: 1;
		/**
		 * ERR_LVL pin state.
		 */
		bool error_level_pin_state: 1;
		/**
		 * DAC resolution. Always returns 0x7.
		 */
		uint8_t dac_resolution: 3;
	} __packed;
};

/**
 * @typedef dac161s997_error_callback_t
 * @brief Callback to invoke when an error is triggered
 *
 * @param dev Pointer to the device
 * @param status @c NULL if read was not possible, otherwise pointer to status.
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
 * @retval 0 success
 * @retval -errno negative error code on failure.
 */
int dac161s997_set_error_callback(const struct device *dev, dac161s997_error_callback_t cb);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DAC_DAC161S997_H_ */

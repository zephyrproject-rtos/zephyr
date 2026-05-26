/*
 * Copyright (c) 2026 Shi Weizhi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPS30_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPS30_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for SPS30
 */
enum sensor_attribute_sps30 {
	/**
	 * Trigger fan cleaning
	 *
	 * Set-only attribute. Writing any value starts the fan
	 * cleaning procedure (accelerates fan to max speed for
	 * 10 seconds). Sensor must be in measurement mode.
	 */
	SENSOR_ATTR_SPS30_FAN_CLEANING = SENSOR_ATTR_PRIV_START,

	/**
	 * Auto cleaning interval in seconds
	 *
	 * Read/write attribute. Default is 604800 seconds (1 week).
	 * Set to 0 to disable automatic cleaning.
	 * Stored permanently in non-volatile memory.
	 */
	SENSOR_ATTR_SPS30_CLEANING_INTERVAL,

	/**
	 * Device status register (read-only)
	 *
	 * 32-bit register. Key bits:
	 * - Bit 4: Fan failure (fan blocked or broken)
	 * - Bit 5: Laser failure (current out of range)
	 * - Bit 21: Fan speed out of range
	 */
	SENSOR_ATTR_SPS30_DEVICE_STATUS,

	/**
	 * Device reset (set-only)
	 *
	 * Writing any value triggers a soft reset. The sensor
	 * returns to the same state as after a power reset.
	 */
	SENSOR_ATTR_SPS30_DEVICE_RESET,

	/**
	 * Clear device status register (set-only)
	 *
	 * Writing any value clears all bits in the Device Status
	 * Register.
	 */
	SENSOR_ATTR_SPS30_CLEAR_STATUS,
};

struct sps30_info {
	const char *product_type;
	const char *serial_number;
	uint8_t fw_major;
	uint8_t fw_minor;
};

/**
 * @brief Read SPS30 device information
 *
 * Populates the info struct with product type, serial number,
 * and firmware version. The string pointers remain valid for
 * the lifetime of the device.
 *
 * @param dev  SPS30 sensor device
 * @param info Pointer to info struct to populate
 * @return 0 on success, negative errno on failure
 */
int sps30_device_info_get(const struct device *dev, struct sps30_info *info);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SPS30_H_ */

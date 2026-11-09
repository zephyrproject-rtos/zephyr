/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of ADT7604 sensor
 * @ingroup adt7604_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADT7604_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADT7604_H_

/**
 * @brief Analog Devices ADT7604 Temperature Sensor
 * @defgroup adt7604_interface ADT7604
 * @ingroup sensor_interface_ext_adi
 * @{
 */

#include <zephyr/drivers/sensor.h>

struct adt7604_custom_sensor_data {
	union {
		struct {
			uint32_t resistance_mohm; /* milliohms, 24-bit */
			uint32_t temp_kelvin;     /* Kelvin,    24-bit */
		} copper_trace;

		struct {
			uint32_t resistance_ohm; /* ohms,               24-bit */
			uint32_t coverage;       /* percentage + 273.15, 24-bit */
		} leak;

		struct {
			float coeff; /* Steinhart-Hart A..F, 32-bit IEEE 754 */
		} thermistor;
	};
};

/**
 * @brief Custom sensor table payload for adt7604_write_custom_sensor_data().
 *
 * Set @p type to match the channel's configured sensor type, then fill the
 * corresponding union arm before calling the write function.
 */
struct adt7604_custom_sensor_data_table {
	struct adt7604_custom_sensor_data *data;
	uint8_t num_rows;
	uint8_t custom_addr_ptr; /* Starting byte offset from 0x250 (user-managed) */
};

/**
 * @brief Write custom sensor data into the ADT7604 custom sensor table.
 *
 * Writes the custom sensor data to the specified byte offset in the custom table
 * (0x250 + custom_addr_ptr) and updates the channel configuration with the calculated
 * address and length fields.
 *
 * User is responsible for:
 * - Managing custom_addr_ptr to avoid overlaps between channels
 * - Ensuring custom_addr_ptr + data length <= 0x17F (table end)
 *
 * @param dev      ADT7604 device pointer
 * @param chan_num Hardware channel number (matches DT `reg`, 1-based)
 * @param table    Payload containing custom sensor data and starting byte offset
 * @return 0 on success, -ENOTSUP if the channel type does not use the custom
 *         table, -EINVAL if address range invalid, -ENODEV if chan_num is not found,
 *         negative errno on SPI error
 */
int adt7604_write_custom_sensor_data_table(const struct device *dev, uint8_t chan_num,
					   const struct adt7604_custom_sensor_data_table *table);

/**
 * @brief ADT7604-specific sensor attributes
 */
enum sensor_attribute_adt7604 {
	/**
	 * @brief Temperature unit selection
	 *
	 * Set to 0 for Celsius (°C), 1 for Fahrenheit (°F)
	 * Read returns current temperature unit setting
	 */
	SENSOR_ATTR_ADT7604_TEMP_UNIT = SENSOR_ATTR_PRIV_START,

	/**
	 * @brief Digital filter notch frequency
	 *
	 * Set to:
	 *   0 = 50Hz and 60Hz rejection (simultaneous, default)
	 *   1 = 60Hz rejection only
	 *   2 = 50Hz rejection only
	 * Read returns current notch frequency setting
	 */
	SENSOR_ATTR_ADT7604_FILTER_NOTCH,

	/**
	 * @brief MUX configuration delay
	 *
	 * Set delay in microseconds (0-25500, multiple of 100µs)
	 * The value is multiplied by 100µs internally
	 * Max extra MUX delay is 25.5ms (255 × 100µs)
	 * Read returns current delay in microseconds
	 */
	SENSOR_ATTR_ADT7604_MUX_DELAY_US,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADT7604_H_ */

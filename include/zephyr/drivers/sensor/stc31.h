/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of STC31 sensor
 * @ingroup stc31_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_STC31_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_STC31_H_

/**
 * @brief Sensirion STC31 CO<sub>2</sub> sensor
 *
 * @c SENSOR_CHAN_CO2 is ppm. Datasheet section 3.5.1 defines vol% as
 * (raw - 16384) * 100 / 32768, and 1 vol% is 10000 ppm.
 * @c SENSOR_CHAN_DIE_TEMP is the bulk-silicon temperature in deg C,
 * raw / 200 (section 3.5.2).
 *
 * sensor_sample_fetch() and stc31_forced_recalibration() return -EAGAIN
 * when the previous measurement command is still inside the section 2.4
 * maximum period: 100 ms in low-noise mode (10 Hz) and 143 ms in
 * low-cross-sensitivity mode (7 Hz). The driver does not sleep to extend
 * that period. sensor_channel_get() returns -ENODATA until a sample has
 * been fetched.
 *
 * @defgroup stc31_interface STC31
 * @ingroup sensor_interface_ext_sensirion
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for STC31
 */
enum sensor_attribute_stc31 {
	/**
	 * Relative humidity compensation input.
	 *
	 * Unit: %RH. Datasheet section 3.5.3 encodes it as an unsigned 16-bit
	 * value, 65535 for 100 %RH. Values outside 0 to 100 %RH are clamped.
	 * When never set after power-up/reset, the sensor assumes 0 %RH.
	 */
	SENSOR_ATTR_STC31_REL_HUMIDITY = SENSOR_ATTR_PRIV_START,
	/**
	 * External temperature compensation input.
	 *
	 * Unit: deg C. Datasheet section 3.5.2 encodes it as a signed 16-bit
	 * value, 200 ticks per deg C. Values outside that encoding are clamped.
	 * Prefer a more accurate external sensor (e.g. SHT4x). When never set,
	 * the sensor uses its internal temperature signal.
	 */
	SENSOR_ATTR_STC31_TEMPERATURE,
	/**
	 * Ambient pressure compensation input.
	 *
	 * Unit: mbar, rounded to an integer (datasheet section 3.5.4).
	 * Compensation is valid from 600 to 1500 mbar (section 3.3.5); values
	 * outside that range are rejected. When never set after power-up/reset,
	 * 1013 mbar is assumed.
	 */
	SENSOR_ATTR_STC31_PRESSURE,
	/**
	 * Binary gas / measurement mode argument for command 0x3615.
	 *
	 * See :c:macro:`STC31_DT_BINARY_GAS_*` in
	 * :file:`zephyr/dt-bindings/sensor/stc31.h`.
	 */
	SENSOR_ATTR_STC31_BINARY_GAS,
};

/**
 * @brief Perform a forced recalibration (FRC).
 *
 * @param dev Pointer to the sensor device
 * @param reference_ppm Reference CO2 concentration in ppm (0-1000000)
 *        (converted internally to the sensor vol% encoding)
 *
 * @return 0 if successful, negative errno code if failure.
 */
int stc31_forced_recalibration(const struct device *dev, uint32_t reference_ppm);

/**
 * @brief Soft-reset the STC31 via I2C general call.
 *
 * Uses the I2C general call address and therefore resets every Sensirion
 * device on the same bus that responds to it. Blocks for the sensor soft-reset
 * time, then restores the last configured binary gas selection. RH, temperature,
 * and pressure compensation inputs are not restored.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int stc31_soft_reset(const struct device *dev);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_STC31_H_ */

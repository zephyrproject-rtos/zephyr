/*
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for CCS811 Indoor Air Quality Sensor
 *
 * Some capabilities and operational requirements for this sensor
 * cannot be expressed within the sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_CCS811_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_CCS811_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <drivers/sensor.h>

/**
 * @brief Fetch the current value of the BASELINE register.
 *
 * The BASELINE register encodes data used to correct sensor readings
 * based on individual device configuration and variation over time.
 *
 * For proper management of the BASELINE register see AN000370
 * "Baseline Save and Restore on CCS811".
 *
 * @param dev Pointer to the sensor device
 *
 * @return a non-negative 16-bit register value, or a negative errno
 * code on failure.
 */
int ccs811_baseline_fetch(struct device *dev);

/**
 * @brief Update the BASELINE register.
 *
 * For proper management of the BASELINE register see AN000370
 * "Baseline Save and Restore on CCS811".
 *
 * @param dev Pointer to the sensor device
 *
 * @param baseline the value to be stored in the BASELINE register.
 *
 * @return 0 if successful, negative errno code if failure.
 */
int ccs811_baseline_update(struct device *dev, u16_t baseline);

/**
 * @brief Update the ENV_DATA register.
 *
 * Accurate calculation of gas levels requires accurate environment
 * data.  Measurements are only accurate to 0.5 Cel and 0.5 %RH.
 *
 * @param dev Pointer to the sensor device
 *
 * @param temperature the current temperature at the sensor
 *
 * @param humidity the current humidity at the sensor
 *
 * @return 0 if successful, negative errno code if failure.
 */
int ccs811_envdata_update(struct device *dev,
			  const struct sensor_value *temperature,
			  const struct sensor_value *humidity);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_CCS811_H_ */

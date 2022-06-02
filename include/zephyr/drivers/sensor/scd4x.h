/*
 * Copyright (c) 2022, Stephen Oliver
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief API for Sensirion SCD4X CO2/T/RH sensors
 *
 * Only provides access to the sensor's pressure level setting, used for increasing CO2 accuracy.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD4X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD4X_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>


/**
 * @brief Updates the sensor ambient pressure value used for increasing CO2 accuracy. Overrides the altitude
 *        set in the device tree. Can be set at any time.
 *
 * @param dev Pointer to the sensor device
 * 
 * @param pressure Ambient pressure, unit is Pascal
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd4x_set_ambient_pressure(const struct device *dev, uint16_t pressure);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD4X_H_ */

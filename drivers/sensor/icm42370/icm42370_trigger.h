/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42370_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42370_TRIGGER_H_

#include <zephyr/device.h>

/** implement the trigger_set sensor api function */
int icm42370_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/**
 * @brief initialize the icm42370 trigger system
 *
 * @param dev icm42370 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm42370_trigger_init(const struct device *dev);

/**
 * @brief enable the trigger gpio interrupt
 *
 * @param dev icm42370 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm42370_trigger_enable_interrupt(const struct device *dev);

/**
 * @brief lock access to the icm42370 device driver
 *
 * @param dev icm42370 device pointer
 */
void icm42370_lock(const struct device *dev);

/**
 * @brief lock access to the icm42370 device driver
 *
 * @param dev icm42370 device pointer
 */
void icm42370_unlock(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42370_TRIGGER_H_ */

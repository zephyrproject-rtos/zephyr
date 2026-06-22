/*
 * Copyright (c) 2022 Esco Medical ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42X70_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42X70_TRIGGER_H_

#include <zephyr/device.h>

/** implement the trigger_set sensor api function */
int icm42x70_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/**
 * @brief initialize the icm42x70 trigger system
 *
 * @param dev icm42x70 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm42x70_trigger_init(const struct device *dev);

/**
 * @brief enable the trigger gpio interrupt
 *
 * @param dev icm42x70 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm42x70_trigger_enable_interrupt(const struct device *dev);

/**
 * @brief lock access to the icm42x70 device driver
 *
 * @param dev icm42x70 device pointer
 */
void icm42x70_lock(const struct device *dev);

/**
 * @brief lock access to the icm42x70 device driver
 *
 * @param dev icm42x70 device pointer
 */
void icm42x70_unlock(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42X70_TRIGGER_H_ */

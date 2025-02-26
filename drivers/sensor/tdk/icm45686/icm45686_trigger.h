/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_TRIGGER_H_

#include <zephyr/device.h>

/** implement the trigger_set sensor api function */
int icm45686_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/**
@brief initialize the icm45686 trigger system
 *
 * @param dev icm45686 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_trigger_init(const struct device *dev);

/**
@brief enable the trigger gpio interrupt
 *
 * @param dev icm45686 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_trigger_enable_interrupt(const struct device *dev);

/**
@brief lock access to the icm45686 device driver
 *
 * @param dev icm45686 device pointer
 */
void icm45686_lock(const struct device *dev);

/**
@brief lock access to the icm45686 device driver
 *
 * @param dev icm45686 device pointer
 */
void icm45686_unlock(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_TRIGGER_H_ */

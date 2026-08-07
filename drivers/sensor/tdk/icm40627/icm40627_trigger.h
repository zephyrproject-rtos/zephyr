/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM40627_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM40627_TRIGGER_H_

#include <zephyr/device.h>

/** implement the trigger_set sensor api function */
int icm40627_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/**
 * @brief initialize the icm40627 trigger system
 *
 * @param dev icm40627 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm40627_trigger_init(const struct device *dev);

/**
 * @brief enable the trigger gpio interrupt
 *
 * @param dev icm40627 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm40627_trigger_enable_interrupt(const struct device *dev);

/**
 * @brief lock access to the icm40627 device driver
 *
 * @param dev icm40627 device pointer
 */
void icm40627_lock(const struct device *dev);

/**
 * @brief lock access to the icm40627 device driver
 *
 * @param dev icm40627 device pointer
 */
void icm40627_unlock(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM40627_TRIGGER_H_ */

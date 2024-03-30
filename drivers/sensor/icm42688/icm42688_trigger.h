/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_TRIGGER_H_

#include <zephyr/device.h>

/** implement the trigger_set sensor api function */
int icm42688_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/**
 * @brief initialize the icm42688 trigger system
 *
 * @param dev icm42688 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int icm42688_trigger_init(const struct device *dev);

/**
 * @brief enable the trigger gpio interrupt
 *
 * @param dev icm42688 device pointer
 * @param new_cfg New configuration to use for the device
 * @return int 0 on success, negative error code otherwise
 */
int icm42688_trigger_enable_interrupt(const struct device *dev, struct icm42688_cfg *new_cfg);

/**
 * @brief lock access to the icm42688 device driver
 *
 * @param dev icm42688 device pointer
 */
void icm42688_lock(const struct device *dev);

/**
 * @brief lock access to the icm42688 device driver
 *
 * @param dev icm42688 device pointer
 */
void icm42688_unlock(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_TRIGGER_H_ */

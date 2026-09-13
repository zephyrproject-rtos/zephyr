/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA400_INTERRUPT_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA400_INTERRUPT_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#define BMA400_INT_MODE (GPIO_INT_EDGE_TO_ACTIVE)

/**
 * @brief Initialize the bma400 interrupt system
 *
 * @param dev bma400 device pointer
 * @return int 0 on success, negative error code otherwise
 */
int bma400_init_interrupt(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA400_INTERRUPT_H_ */

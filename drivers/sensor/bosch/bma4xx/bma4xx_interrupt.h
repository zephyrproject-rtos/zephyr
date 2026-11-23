/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA4XX_INTERRUPT_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA4XX_INTERRUPT_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @brief Initialize the bma4xx interrupt system
 *
 * @param dev bma4xx device pointer
 * @return int 0 on success, negative error code otherwise
 */
int bma4xx_init_interrupt(const struct device *dev);

/**
 * @brief Enable the trigger gpio interrupt1
 *
 * @param dev bma4xx device pointer
 * @param new_cfg New configuration to use for the device
 * @return int 0 on success, negative error code otherwise
 */
int bma4xx_enable_interrupt1(const struct device *dev, struct bma4xx_runtime_config *new_cfg);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_INTERRUPT_H_ */

/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for PCA_SERIES GPIO driver
 * @ingroup gpio_pca_series_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_PCA_SERIES_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_PCA_SERIES_H_

/**
 * @defgroup gpio_pca_series_interface NXP PCA SERIES
 * @ingroup gpio_interface_ext
 * @brief NXP PCA SERIES I2C-based I/O expander
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset function of pca_series
 *
 * This function pulls reset pin to reset a pca_series
 * device if reset_pin is present. Otherwise it write
 * reset value to device registers.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
int gpio_pca_series_reset(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_PCA_SERIES_H_ */

/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NXP PCAL64XXA GPIO driver
 * @ingroup gpio_pcal64xxa_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_PCAL64XXA_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_PCAL64XXA_H_

/**
 * @defgroup gpio_pcal64xxa_interface NXP PCAL64XXA
 * @ingroup gpio_interface_ext
 * @brief NXP PCAL64XXA I2C-based I/O expander
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manually reset a PCAL64XXA
 *
 * Resetting a PCAL64XXA manually is only necessary if the by default
 * enabled automatic reset has been disabled.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
int pcal64xxa_reset(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_PCAL64XXA_H_ */

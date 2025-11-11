/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NCT38XX GPIO driver
 * @ingroup gpio_nct38xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NCT38XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NCT38XX_H_

/**
 * @defgroup gpio_nct38xx_interface NCT38XX
 * @ingroup gpio_interface_ext
 * @brief Nuvoton NCT38XX series I2C-based GPIO expander
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dispatch all GPIO ports ISR in the NCT38XX device.
 *
 * @param dev NCT38XX device.
 */
void nct38xx_gpio_alert_handler(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NCT38XX_H_ */

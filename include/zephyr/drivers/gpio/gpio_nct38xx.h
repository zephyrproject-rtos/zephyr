/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NCT38XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NCT38XX_H_

/**
 * @brief Dispatch all GPIO ports ISR in the NCT38XX device.
 *
 * @param dev NCT38XX device.
 */
void nct38xx_gpio_alert_handler(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NCT38XX_H_ */

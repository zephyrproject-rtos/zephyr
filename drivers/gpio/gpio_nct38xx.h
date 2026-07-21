/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_NCT38XX_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_NCT38XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

/* NCT38XX controller register */
#define NCT38XX_REG_ALERT      0x10
#define NCT38XX_REG_ALERT_MASK 0x12

#define NCT38XX_REG_GPIO_DATA_IN(n)     (0xC0 + ((n) * 8))
#define NCT38XX_REG_GPIO_DATA_OUT(n)    (0xC1 + ((n) * 8))
#define NCT38XX_REG_GPIO_DIR(n)         (0xC2 + ((n) * 8))
#define NCT38XX_REG_GPIO_OD_SEL(n)      (0xC3 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_RISE(n)  (0xC4 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_FALL(n)  (0xC5 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_LEVEL(n) (0xC6 + ((n) * 8))
#define NCT38XX_REG_GPIO_ALERT_MASK(n)  (0xC7 + ((n) * 8))
#define NCT38XX_REG_MUX_CONTROL         0xD0
#define NCT38XX_REG_GPIO_ALERT_STAT(n)  (0xD4 + (n))

/* NCT38XX controller register field */
#define NCT38XX_REG_ALERT_VENDOR_DEFINDED_ALERT      15
#define NCT38XX_REG_ALERT_MASK_VENDOR_DEFINDED_ALERT 15

/**
 * @brief Dispatch GPIO port ISR
 *
 * @param dev GPIO port device
 * @return 0 if successful, otherwise failed.
 */
int gpio_nct38xx_dispatch_port_isr(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_NCT38XX_H_*/

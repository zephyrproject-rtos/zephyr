/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>

#define VDD_5V0_PWR_CTRL_GPIO_PIN  21   /* ENABLE_5V0_BOOST  --> speed sensor */

static int pwr_ctrl_init(struct device *dev)
{
    struct device *gpio;
    int    err = -ENODEV;

    /* Get handle of the GPIO device. */
    gpio = device_get_binding(DT_GPIO_P0_DEV_NAME);

    /* Valid handle? */
    if (gpio != NULL) {

        /* Configure this pin as output. */
        if ((err = gpio_pin_configure(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN, GPIO_OUTPUT_ACTIVE)) == 0) {

            /* Write "1" to this pin. */
        	err = gpio_pin_set(gpio, VDD_5V0_PWR_CTRL_GPIO_PIN, 1);
        }

        /* Wait for the rail to come up and stabilize. */
        k_sleep(1);
    }

    /* Operation status? */
    return (err);
}

DEVICE_INIT(vdd_5v0_pwr_ctrl_init, "", pwr_ctrl_init, NULL, NULL,
            POST_KERNEL, 70);

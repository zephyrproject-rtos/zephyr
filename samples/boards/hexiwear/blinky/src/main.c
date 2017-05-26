/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <board.h>
#include <device.h>
#include <gpio.h>

#include <misc/printk.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME 	1000

#ifndef CONFIG_BOARD_HEXIWEAR_K64
#error this is a board specific demo
#endif

void main(void)
{
        struct device *rdev;
        struct device *gdev;
        struct device *bdev;

        rdev = device_get_binding(RED_GPIO_NAME);
        gdev = device_get_binding(GREEN_GPIO_NAME);
        bdev = device_get_binding(BLUE_GPIO_NAME);

        if (rdev == 0 || gdev == 0 || bdev == 0) {
                printk("unable to find devices\n");
                return;
        }

        while (1) {
                gpio_pin_write(rdev, RED_GPIO_PIN, 0);
                k_sleep(SLEEP_TIME);
                gpio_pin_write(rdev, RED_GPIO_PIN, 1);

                gpio_pin_write(gdev, GREEN_GPIO_PIN, 0);
                k_sleep(SLEEP_TIME);
                gpio_pin_write(gdev, GREEN_GPIO_PIN, 1);

                gpio_pin_write(bdev, BLUE_GPIO_PIN, 0);
                k_sleep(SLEEP_TIME);

                gpio_pin_write(rdev, RED_GPIO_PIN, 0);
                gpio_pin_write(gdev, GREEN_GPIO_PIN, 0);
                k_sleep(SLEEP_TIME);

                gpio_pin_write(rdev, RED_GPIO_PIN, 1);
                gpio_pin_write(gdev, GREEN_GPIO_PIN, 1);
                gpio_pin_write(bdev, BLUE_GPIO_PIN, 1);
                k_sleep(SLEEP_TIME);
        }
}

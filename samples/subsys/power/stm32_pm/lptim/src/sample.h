/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2019 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <string.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

/* GPIO for the User push button */
#define PORT		DT_ALIAS_SW0_GPIOS_CONTROLLER
#define SW		DT_ALIAS_SW0_GPIOS_PIN
/* GPIO for the Led */
#define PORT0		DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED0		DT_ALIAS_LED0_GPIOS_PIN

#define LOW		0
#define HIGH		1
#define LED_ON		HIGH
#define LED_OFF		LOW

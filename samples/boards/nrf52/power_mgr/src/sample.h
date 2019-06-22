/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power/power.h>
#include <string.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

#define GPIO_CFG_SENSE_LOW (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos)

/* change this to use another GPIO port */
#define PORT		DT_ALIAS_SW0_GPIOS_CONTROLLER
#define BUTTON_1	DT_ALIAS_SW0_GPIOS_PIN
#define BUTTON_2	DT_ALIAS_SW1_GPIOS_PIN
#define LED_1		DT_ALIAS_LED0_GPIOS_PIN
#define LED_2		DT_ALIAS_LED1_GPIOS_PIN

#define LOW		0
#define HIGH		1
#define LED_ON		LOW
#define LED_OFF		HIGH

extern struct device *gpio_port;

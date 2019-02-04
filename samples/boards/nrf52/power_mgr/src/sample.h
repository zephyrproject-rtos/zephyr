/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power.h>
#include <string.h>
#include <soc.h>
#include <device.h>
#include <gpio.h>
#include <misc/printk.h>

#define GPIO_CFG_SENSE_LOW (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos)

/* change this to use another GPIO port */
#define PORT		SW0_GPIO_CONTROLLER
#define BUTTON_1	SW0_GPIO_PIN
#define BUTTON_2	SW1_GPIO_PIN
#define LED_1		LED0_GPIO_PIN
#define LED_2		LED1_GPIO_PIN

#define LOW		0
#define HIGH		1
#define LED_ON		LOW
#define LED_OFF		HIGH

extern struct device *gpio_port;

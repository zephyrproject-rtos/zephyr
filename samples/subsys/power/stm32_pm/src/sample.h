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

/* The devicetree node identifier for the "sw0" alias. */
#define SW0_NODE DT_ALIAS(sw0)
/* GPIO for the User push button */
#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW	DT_GPIO_LABEL(SW0_NODE, gpios)
#define PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#if DT_PHA_HAS_CELL(SW0_NODE, gpios, flags)
#define SW0_FLAGS DT_GPIO_FLAGS(SW0_NODE, gpios)
#else
#define SW0_FLAGS 0
#endif
#else
/* A build error here means your board isn't set up to input a button. */
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW	""
#define PIN	0
#endif

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
/* GPIO for the Led */
#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN0	DT_GPIO_PIN(LED0_NODE, gpios)
#if DT_PHA_HAS_CELL(LED0_NODE, gpios, flags)
#define LED0_FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
#define LED0_FLAGS 0
#endif
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN0	0
#endif

#define LOW		0
#define HIGH		1
#define LED_ON		HIGH
#define LED_OFF		LOW

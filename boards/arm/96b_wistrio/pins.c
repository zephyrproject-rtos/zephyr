/*
 * Copyright (c) 2019 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/gpio.h>
#include <init.h>
#include <kernel.h>
#include <sys/sys_io.h>

static int pins_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);
	const struct device *gpioa, *gpiob, *gpioh;

	gpioa = device_get_binding(DT_LABEL(DT_NODELABEL(gpioa)));
	if (!gpioa) {
		return -ENODEV;
	}

	gpiob = device_get_binding(DT_LABEL(DT_NODELABEL(gpiob)));
	if (!gpiob) {
		return -ENODEV;
	}

	gpioh = device_get_binding(DT_LABEL(DT_NODELABEL(gpioh)));
	if (!gpioh) {
		return -ENODEV;
	}

	/* RF_CTX_PA */
	gpio_pin_configure(gpioa, 4, GPIO_OUTPUT | GPIO_PULL_UP);
	gpio_pin_set(gpioa, 4, 1);

	/* RF_CRX_RX */
	gpio_pin_configure(gpiob, 6, GPIO_OUTPUT | GPIO_PULL_UP);
	gpio_pin_set(gpiob, 6, 1);

	/* RF_CBT_HF */
	gpio_pin_configure(gpiob, 7, GPIO_OUTPUT | GPIO_PULL_UP);
	gpio_pin_set(gpiob, 7, 0);

	gpio_pin_configure(gpioh, 1, GPIO_OUTPUT);
	gpio_pin_set(gpioh, 1, 1);

	return 0;
}

/* Need to be initialised after GPIO driver */
SYS_INIT(pins_stm32_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

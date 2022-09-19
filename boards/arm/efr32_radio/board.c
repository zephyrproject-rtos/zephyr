/*
 * Copyright (c) 2018 Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* This pin is used to enable the serial port using the board controller */
#ifdef CONFIG_BOARD_EFR32_RADIO_BRD4180A
#define VCOM_ENABLE_GPIO_NODE  DT_NODELABEL(gpiod)
#define VCOM_ENABLE_GPIO_PIN   4
#else
#define VCOM_ENABLE_GPIO_NODE  DT_NODELABEL(gpioa)
#define VCOM_ENABLE_GPIO_PIN   5
#endif /* CONFIG_BOARD_EFR32_RADIO_BRD4180A */

static int efr32_radio_init(const struct device *dev)
{
	const struct device *vce_dev; /* Virtual COM Port Enable GPIO Device */

	ARG_UNUSED(dev);

	/* Enable the board controller to be able to use the serial port */
	vce_dev = DEVICE_DT_GET(VCOM_ENABLE_GPIO_NODE);
	if (!device_is_ready(vce_dev)) {
		printk("Virtual COM Port Enable device is not ready!\n");
		return -ENODEV;
	}

	gpio_pin_configure(vce_dev, VCOM_ENABLE_GPIO_PIN, GPIO_OUTPUT_HIGH);

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(efr32_radio_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

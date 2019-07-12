/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to utilize APA102C LED.
 *
 * The APA102/C requires 5V data and clock signals, so logic
 * level shifter (preferred) or pull-up resistors are needed.
 * Make sure the pins are 5V tolerant if using pull-up
 * resistors.
 *
 * WARNING: the APA102C are very bright even at low settings.
 * Protect your eyes and do not look directly into those LEDs.
 */

#include <zephyr.h>

#include <sys/printk.h>

#include <device.h>
#include <drivers/gpio.h>
/* in millisecond */
#define SLEEPTIME	250

#define GPIO_DATA_PIN	16
#define GPIO_CLK_PIN	19
#define GPIO_NAME	"GPIO_"

#define GPIO_DRV_NAME	DT_ALIAS_GPIO_0_LABEL

#define APA102C_START_FRAME	0x00000000
#define APA102C_END_FRAME	0xFFFFFFFF

/* The LED is very bright. So to protect the eyes,
 * brightness is set very low, and RGB values are
 * set low too.
 */
u32_t rgb[] = {
	0xE1000010,
	0xE1001000,
	0xE1100000,
	0xE1101010,
};
#define NUM_RGB		4

/* Number of LEDS linked together */
#define NUM_LEDS	1

void send_rgb(struct device *gpio_dev, u32_t rgb)
{
	int i;

	for (i = 0; i < 32; i++) {
		/* MSB goes in first */
		gpio_pin_write(gpio_dev, GPIO_DATA_PIN, !!(rgb & 0x80000000));

		/* Latch data into LED */
		gpio_pin_write(gpio_dev, GPIO_CLK_PIN, 1);
		gpio_pin_write(gpio_dev, GPIO_CLK_PIN, 0);

		rgb <<= 1;
	}
}

void main(void)
{
	struct device *gpio_dev;
	int ret;
	int idx = 0;
	int leds = 0;

	gpio_dev = device_get_binding(GPIO_DRV_NAME);
	if (!gpio_dev) {
		printk("Cannot find %s!\n", GPIO_DRV_NAME);
		return;
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_dev, GPIO_DATA_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_DATA_PIN);
	}

	ret = gpio_pin_configure(gpio_dev, GPIO_CLK_PIN, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_CLK_PIN);
	}

	while (1) {
		send_rgb(gpio_dev, APA102C_START_FRAME);

		for (leds = 0; leds < NUM_LEDS; leds++) {
			send_rgb(gpio_dev, rgb[(idx + leds) % NUM_RGB]);
		}

		/* If there are more LEDs linked together,
		 * then what NUM_LEDS is, the NUM_LEDS+1
		 * LED is going to be full bright.
		 */
		send_rgb(gpio_dev, APA102C_END_FRAME);

		idx++;
		if (idx >= NUM_RGB) {
			idx = 0;
		}

		k_sleep(SLEEPTIME);
	}
}

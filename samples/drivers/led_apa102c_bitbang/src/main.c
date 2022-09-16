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

#include <zephyr/zephyr.h>

#include <zephyr/sys/printk.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
/* in millisecond */
#define SLEEPTIME	K_MSEC(250)

#define GPIO_DATA_PIN	16
#define GPIO_CLK_PIN	19
#define GPIO_NAME	"GPIO_"

#define APA102C_START_FRAME	0x00000000
#define APA102C_END_FRAME	0xFFFFFFFF

/* The LED is very bright. So to protect the eyes,
 * brightness is set very low, and RGB values are
 * set low too.
 */
uint32_t rgb[] = {
	0xE1000010,
	0xE1001000,
	0xE1100000,
	0xE1101010,
};
#define NUM_RGB		4

/* Number of LEDS linked together */
#define NUM_LEDS	1

void send_rgb(const struct device *gpio_dev, uint32_t rgb)
{
	int i;

	for (i = 0; i < 32; i++) {
		/* MSB goes in first */
		gpio_pin_set_raw(gpio_dev, GPIO_DATA_PIN, (rgb & BIT(31)) != 0);

		/* Latch data into LED */
		gpio_pin_set_raw(gpio_dev, GPIO_CLK_PIN, 1);
		gpio_pin_set_raw(gpio_dev, GPIO_CLK_PIN, 0);

		rgb <<= 1;
	}
}

void main(void)
{
	const struct device *gpio_dev;
	int ret;
	int idx = 0;
	int leds = 0;

	gpio_dev = DEVICE_DT_GET(DT_ALIAS(gpio_0));
	if (!device_is_ready(gpio_dev)) {
		printk("GPIO device %s is not ready!\n", gpio_dev->name);
		return;
	}

	/* Setup GPIO output */
	ret = gpio_pin_configure(gpio_dev, GPIO_DATA_PIN, GPIO_OUTPUT);
	if (ret) {
		printk("Error configuring " GPIO_NAME "%d!\n", GPIO_DATA_PIN);
	}

	ret = gpio_pin_configure(gpio_dev, GPIO_CLK_PIN, GPIO_OUTPUT);
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

/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>

/**
 * the demo assumes use of nucleo_f103rb board, adjust defines below
 * to fit your board
 */

/* we're going to use PB8 and PB5 */
#define PORT "GPIOB"
/* PB5 */
#define LED1 5
/* PB8 */
#define LED2 8

#define SLEEP_TIME	500

void main(void)
{
	int cnt = 0;
	struct device *gpiob;

	gpiob = device_get_binding(PORT);

	gpio_pin_configure(gpiob, LED1, GPIO_DIR_OUT);
	gpio_pin_configure(gpiob, LED2, GPIO_DIR_OUT);

	while (1) {
		gpio_pin_write(gpiob, LED1, cnt % 2);
		gpio_pin_write(gpiob, LED2, (cnt + 1) % 2);
		k_sleep(SLEEP_TIME);
		cnt++;
	}
}

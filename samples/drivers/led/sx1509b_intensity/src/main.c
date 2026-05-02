/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_sx1509b.h>

#define NUMBER_OF_LEDS 3
#define GREEN_LED DT_GPIO_PIN(DT_NODELABEL(led0), gpios)
#define BLUE_LED DT_GPIO_PIN(DT_NODELABEL(led1), gpios)
#define RED_LED DT_GPIO_PIN(DT_NODELABEL(led2), gpios)

enum sx1509b_color { sx1509b_red = 0, sx1509b_green, sx1509b_blue };

const struct device *sx1509b_dev;
static struct k_work_delayable rgb_work;
static uint8_t which_color = sx1509b_red;
static uint8_t iterator;

static const gpio_pin_t rgb_pins[] = {
	RED_LED,
	GREEN_LED,
	BLUE_LED,
};

static void rgb_work_handler(struct k_work *work)
{
	if (which_color == sx1509b_red) {
		sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, iterator);

		iterator++;

		if (iterator >= 255) {
			sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, 0);
			which_color = sx1509b_green;
			iterator = 0;
		}
	} else if (which_color == sx1509b_green) {
		sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, iterator);

		iterator++;

		if (iterator >= 255) {
			sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED,
						      0);
			which_color = sx1509b_blue;
			iterator = 0;
		}
	} else if (which_color == sx1509b_blue) {
		sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, iterator);

		iterator++;

		if (iterator >= 255) {
			sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, 0);
			which_color = sx1509b_red;
			iterator = 0;
		}
	}

	k_work_schedule(k_work_delayable_from_work(work), K_MSEC(10));
}

int main(void)
{
	int err;

	printk("SX1509B intensity sample\n");

	sx1509b_dev = DEVICE_DT_GET(DT_NODELABEL(sx1509b));

	if (!device_is_ready(sx1509b_dev)) {
		printk("sx1509b: device not ready.\n");
		return 0;
	}

	for (int i = 0; i < NUMBER_OF_LEDS; i++) {
		err = sx1509b_led_intensity_pin_configure(sx1509b_dev,
							  rgb_pins[i]);

		if (err) {
			printk("Error configuring pin for LED intensity\n");
		}
	}

	k_work_init_delayable(&rgb_work, rgb_work_handler);
	k_work_schedule(&rgb_work, K_NO_WAIT);
	return 0;
}

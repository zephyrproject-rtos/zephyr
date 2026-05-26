/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define BTN_NODE DT_ALIAS(sw0)

/* LED state tracking. */
static bool led_is_on = false;

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define HAS_LED0 1

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#else
#define HAS_LED0 0
#endif

#if DT_NODE_HAS_STATUS(BTN_NODE, okay)
#define HAS_BTN 1
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BTN_NODE, gpios);
static struct gpio_callback btn_cb_data;
#else
#define HAS_BTN 0
#endif

#if HAS_LED0 && HAS_BTN
static void button_pressed(const struct device *dev, struct gpio_callback *cb,
		             uint32_t pins)
{
	//ARG_UNUSED(dev);
	//ARG_UNUSED(cb);
	//ARG_UNUSED(pins);

	led_is_on = !led_is_on;
	(void)gpio_pin_set_dt(&led, led_is_on ? 1 : 0);
	printf("SW0 pressed: LED %s\n", led_is_on ? "ON" : "OFF");
}
#endif

int main(void)
{
	if (!HAS_LED0) {
		printf("No led0 alias found in devicetree.\n");
		while (1) {
			k_msleep(SLEEP_TIME_MS);
		}
	}

	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return 0;
	}

	if (!HAS_BTN) {
		printf("Missing sw0 alias for button control.\n");
		while (1) {
			k_msleep(SLEEP_TIME_MS);
		}
	}

	if (!gpio_is_ready_dt(&button)) {
		printf("Button GPIO device not ready.\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	gpio_init_callback(&btn_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &btn_cb_data);

	printf("Press SW0 to toggle LED ON/OFF\n");

	while (1) {
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}

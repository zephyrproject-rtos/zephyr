/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_mchp_touch_api.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* LED on (or) off value */
#define LED_ON  1
#define LED_OFF 0

#define SCROLLER_POSITION_1 0u
#define SCROLLER_POSITION_2 1u
#define SCROLLER_POSITION_3 3u
#define SCROLLER_POSITION_4 4u
#define SCROLLER_POSITION_5 6u

#define TOUCH_NODE DT_NODELABEL(ptc)

/* The devicetree node identifier for the "led0" alias. */
#define LED_B0_NODE         DT_NODELABEL(qt7_led_b0)
#define LED_B1_NODE         DT_NODELABEL(qt7_led_b1)
#define LED_SCROLLDER0_NODE DT_NODELABEL(qt7_scroller_led0)
#define LED_SCROLLDER1_NODE DT_NODELABEL(qt7_scroller_led1)
#define LED_SCROLLDER2_NODE DT_NODELABEL(qt7_scroller_led2)
#define LED_SCROLLDER3_NODE DT_NODELABEL(qt7_scroller_led3)
#define LED_SCROLLDER4_NODE DT_NODELABEL(qt7_scroller_led4)
#define LED_SCROLLDER5_NODE DT_NODELABEL(qt7_scroller_led5)

static const struct gpio_dt_spec b0_led = GPIO_DT_SPEC_GET(LED_B0_NODE, gpios);
static const struct gpio_dt_spec b1_led = GPIO_DT_SPEC_GET(LED_B1_NODE, gpios);
static const struct gpio_dt_spec scroller0_led = GPIO_DT_SPEC_GET(LED_SCROLLDER0_NODE, gpios);
static const struct gpio_dt_spec scroller1_led = GPIO_DT_SPEC_GET(LED_SCROLLDER1_NODE, gpios);
static const struct gpio_dt_spec scroller2_led = GPIO_DT_SPEC_GET(LED_SCROLLDER2_NODE, gpios);
static const struct gpio_dt_spec scroller3_led = GPIO_DT_SPEC_GET(LED_SCROLLDER3_NODE, gpios);
static const struct gpio_dt_spec scroller4_led = GPIO_DT_SPEC_GET(LED_SCROLLDER4_NODE, gpios);
static const struct gpio_dt_spec scroller5_led = GPIO_DT_SPEC_GET(LED_SCROLLDER5_NODE, gpios);

static void touch_callback(struct input_event *evt, void *user_data)
{
	uint16_t scroller_position = 0u;

	if (evt != NULL) {
		switch (evt->type) {
		case INPUT_EV_KEY: {
			switch (evt->code) {
			case INPUT_KEY_0: {
				gpio_pin_set_dt(&b0_led, evt->value);
				break;
			}
			case INPUT_KEY_1: {
				gpio_pin_set_dt(&b1_led, evt->value);
				break;
			}
			/* The events INPUT_KEY_2, INPUT_KEY_3, INPUT_KEY_4
			 * will be available when CONFIG_INPUT_MCHP_SCROLLER_TOUCH_EVENT
			 * is enabled.
			 */
			case INPUT_KEY_2:
			case INPUT_KEY_3:
			case INPUT_KEY_4:
				if (evt->value == 0u) {
					gpio_pin_set_dt(&scroller0_led, LED_OFF);
					gpio_pin_set_dt(&scroller1_led, LED_OFF);
					gpio_pin_set_dt(&scroller2_led, LED_OFF);
					gpio_pin_set_dt(&scroller3_led, LED_OFF);
					gpio_pin_set_dt(&scroller4_led, LED_OFF);
					gpio_pin_set_dt(&scroller5_led, LED_OFF);
				}
				break;
			default: {
				break;
			}
			}
			break;
		}
		case INPUT_EV_ABS: {
			if (evt->code == INPUT_ABS_WHEEL) {
				scroller_position = evt->value;
				scroller_position = scroller_position >> 5u;

				gpio_pin_set_dt(&scroller0_led, LED_OFF);
				gpio_pin_set_dt(&scroller1_led, LED_OFF);
				gpio_pin_set_dt(&scroller2_led, LED_OFF);
				gpio_pin_set_dt(&scroller3_led, LED_OFF);
				gpio_pin_set_dt(&scroller4_led, LED_OFF);
				gpio_pin_set_dt(&scroller5_led, LED_OFF);

				gpio_pin_set_dt(&scroller0_led, LED_ON);
				if (scroller_position > SCROLLER_POSITION_1) {
					gpio_pin_set_dt(&scroller1_led, LED_ON);
				}
				if (scroller_position > SCROLLER_POSITION_2) {
					gpio_pin_set_dt(&scroller2_led, LED_ON);
				}
				if (scroller_position > SCROLLER_POSITION_3) {
					gpio_pin_set_dt(&scroller3_led, LED_ON);
				}
				if (scroller_position > SCROLLER_POSITION_4) {
					gpio_pin_set_dt(&scroller4_led, LED_ON);
				}
				if (scroller_position > SCROLLER_POSITION_5) {
					gpio_pin_set_dt(&scroller5_led, LED_ON);
				}
			}
			break;
		}
		default:
			break;
		}
	}
}

INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(TOUCH_NODE), touch_callback, NULL, button);

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&b0_led)) {
		printk("Button0 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&b1_led)) {
		printk("Button1 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&scroller0_led)) {
		printk("Scroller0 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&scroller1_led)) {
		printk("Scroller1 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&scroller2_led)) {
		printk("Scroller2 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&scroller3_led)) {
		printk("Scroller3 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&scroller4_led)) {
		printk("Scroller4 GPIO controller device is not ready\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&scroller5_led)) {
		printk("Scroller5 GPIO controller device is not ready\n");
		return 0;
	}

	/* turn off the led by default */
	ret = gpio_pin_configure_dt(&b0_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Button0 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&b1_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Button1 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&scroller0_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Scroller0 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&scroller1_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Scroller1 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&scroller2_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Scroller2 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&scroller3_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Scroller3 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&scroller4_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Scroller4 Led device. ret code = %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&scroller5_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Failed to configure Scroller5 Led device. ret code = %d\n", ret);
		return 0;
	}

	while (1) {
		/*user code */
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

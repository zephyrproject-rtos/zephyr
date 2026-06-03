/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_mchp_touch_api.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* Button (or) Channel Number */
#define BUTTON0 0u
#define BUTTON1 1u

/* Scroller (or) Channel Number */
#define SCROLLER 0u

/* LED on (or) off value */
#define LED_ON  1
#define LED_OFF 0

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

static void process_button_status(struct input_event *evt)
{
	if (get_sensor_state(evt->dev, BUTTON0) == QTM_KEY_STATE_DETECT) {
		gpio_pin_set_dt(&b0_led, LED_ON);
	} else {
		gpio_pin_set_dt(&b0_led, LED_OFF);
	}

	if (get_sensor_state(evt->dev, BUTTON1) == QTM_KEY_STATE_DETECT) {
		gpio_pin_set_dt(&b1_led, LED_ON);
	} else {
		gpio_pin_set_dt(&b1_led, LED_OFF);
	}
}

static void process_scroller_status(struct input_event *evt)
{
	uint8_t scroller_status = 0u;
	uint16_t scroller_position = 0u;

	scroller_status = get_scroller_state(evt->dev, SCROLLER);
	scroller_position = get_scroller_position(evt->dev, SCROLLER);
	/* Example: 8 bit scroller resolution. Modify as per requirement. */
	scroller_position = scroller_position >> 5u;

	gpio_pin_set_dt(&scroller0_led, LED_OFF);
	gpio_pin_set_dt(&scroller1_led, LED_OFF);
	gpio_pin_set_dt(&scroller2_led, LED_OFF);
	gpio_pin_set_dt(&scroller3_led, LED_OFF);
	gpio_pin_set_dt(&scroller4_led, LED_OFF);
	gpio_pin_set_dt(&scroller5_led, LED_OFF);

	if (0u != scroller_status) {
		gpio_pin_set_dt(&scroller0_led, LED_ON);

		if (scroller_position > 0u) {
			gpio_pin_set_dt(&scroller1_led, LED_ON);
		}
		if (scroller_position > 1u) {
			gpio_pin_set_dt(&scroller2_led, LED_ON);
		}
		if (scroller_position > 3u) {
			gpio_pin_set_dt(&scroller3_led, LED_ON);
		}
		if (scroller_position > 4u) {
			gpio_pin_set_dt(&scroller4_led, LED_ON);
		}
		if (scroller_position > 6u) {
			gpio_pin_set_dt(&scroller5_led, LED_ON);
		}
	}
}

static void touch_callback(struct input_event *evt, void *user_data)
{
	if (evt != NULL) {
		if (evt->code == INPUT_BTN_TOUCH) {
			process_button_status(evt);
			process_scroller_status(evt);
		}
	}
}

INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET(TOUCH_NODE), touch_callback, NULL, button);

int main(void)
{
	gpio_is_ready_dt(&b0_led);
	gpio_is_ready_dt(&b1_led);
	gpio_is_ready_dt(&scroller0_led);
	gpio_is_ready_dt(&scroller1_led);
	gpio_is_ready_dt(&scroller2_led);
	gpio_is_ready_dt(&scroller3_led);
	gpio_is_ready_dt(&scroller4_led);
	gpio_is_ready_dt(&scroller5_led);

	/* turn off the led by default */
	gpio_pin_configure_dt(&b0_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&b1_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&scroller0_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&scroller1_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&scroller2_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&scroller3_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&scroller4_led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&scroller5_led, GPIO_OUTPUT_INACTIVE);

	while (1) {
		/*user code */
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}

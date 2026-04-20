/*
 * Copyright (c) 2025 Realtek Semiconductor Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Button-controlled Blinky Sample
 *
 * This sample demonstrates using a push button to control LED behavior:
 * - Short press: Toggle LED on/off
 * - Long press (>1s): Cycle through blink speeds (slow -> medium -> fast -> off -> slow)
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* LED node from devicetree */
#define LED0_NODE DT_ALIAS(led0)

/* Button node from devicetree */
#define SW0_NODE DT_ALIAS(sw0)

#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

static struct gpio_callback button_cb_data;

/* Blink modes: off, slow (1s), medium (500ms), fast (100ms) */
enum blink_mode {
	BLINK_OFF = 0,
	BLINK_SLOW,
	BLINK_MEDIUM,
	BLINK_FAST,
	BLINK_MODE_COUNT
};

static const int blink_periods_ms[] = {
	0,      /* OFF */
	1000,   /* SLOW: 1 second */
	500,    /* MEDIUM: 500ms */
	100,    /* FAST: 100ms */
};

static const char *const blink_mode_names[] = {
	"OFF",
	"SLOW (1s)",
	"MEDIUM (500ms)",
	"FAST (100ms)",
};

static volatile enum blink_mode current_mode = BLINK_SLOW;
static volatile bool led_state;
static volatile bool button_pressed;
static volatile int64_t press_start_time;

#define LONG_PRESS_THRESHOLD_MS 1000

static void button_pressed_handler(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	int val = gpio_pin_get_dt(&button);

	if (val == 1) {
		/* Button pressed (active low, so pressed = 1 after inversion) */
		button_pressed = true;
		press_start_time = k_uptime_get();
	} else {
		/* Button released */
		if (button_pressed) {
			int64_t press_duration = k_uptime_get() - press_start_time;

			if (press_duration >= LONG_PRESS_THRESHOLD_MS) {
				/* Long press: cycle blink mode */
				current_mode = (current_mode + 1) % BLINK_MODE_COUNT;
				printf("Blink mode changed to: %s\n",
						blink_mode_names[current_mode]);
			} else {
				/* Short press: toggle LED (only in OFF mode) */
				if (current_mode == BLINK_OFF) {
					led_state = !led_state;
					gpio_pin_set_dt(&led, led_state);
					printf("LED toggled %s\n", led_state ? "ON" : "OFF");
				} else {
					printf("Short press (ignored in blink mode)\n");
				}
			}
			button_pressed = false;
		}
	}
}

int main(void)
{
	int ret;

	printf("Button-controlled Blinky Sample\n");
	printf("================================\n");
	printf("Short press: Toggle LED (when in OFF mode)\n");
	printf("Long press (>1s): Cycle blink modes\n\n");

	/* Configure LED */
	if (!gpio_is_ready_dt(&led)) {
		printf("Error: LED device %s is not ready\n", led.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printf("Error: Failed to configure LED pin\n");
		return -1;
	}

	/* Configure button */
	if (!gpio_is_ready_dt(&button)) {
		printf("Error: Button device %s is not ready\n", button.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		printf("Error: Failed to configure button pin\n");
		return -1;
	}

	/* Configure button interrupt */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		printf("Error: Failed to configure button interrupt\n");
		return -1;
	}

	gpio_init_callback(&button_cb_data, button_pressed_handler, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	printf("Starting in %s mode\n", blink_mode_names[current_mode]);

	/* Main loop - handle blinking */
	while (1) {
		if (current_mode == BLINK_OFF) {
			/* In OFF mode, LED is controlled by button press */
			k_msleep(100);
		} else {
			/* Blink mode */
			led_state = !led_state;
			gpio_pin_set_dt(&led, led_state);
			k_msleep(blink_periods_ms[current_mode] / 2);
		}
	}

	return 0;
}

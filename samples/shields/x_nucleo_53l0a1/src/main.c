/*
 * Copyright (c) 2021 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include "display_7seg.h"

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE        DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define N_MODES ARRAY_SIZE(modes)
#define N_SENSORS ARRAY_SIZE(sensors)
#define LEFT   sensors[0]
#define CENTER sensors[1]
#define RIGHT  sensors[2]

typedef void (*fsm_state)(void);

static int64_t last_mode_change;

static const struct device *const sensors[] = {
	DEVICE_DT_GET(DT_NODELABEL(vl53l0x_l_x_nucleo_53l0a1)),
	DEVICE_DT_GET(DT_NODELABEL(vl53l0x_c_x_nucleo_53l0a1)),
	DEVICE_DT_GET(DT_NODELABEL(vl53l0x_r_x_nucleo_53l0a1)),
};

static void mode_show_distance(void)
{
	int ret;
	double distance;
	struct sensor_value value;

	ret = sensor_sample_fetch(CENTER);
	if (ret) {
		printk("sensor_sample_fetch(%s) failed -> %d\n", CENTER->name, ret);
		display_chars(TEXT_Err);
		return;
	}

	ret = sensor_channel_get(CENTER, SENSOR_CHAN_DISTANCE, &value);
	if (ret) {
		printk("sensor_channel_get(%s) failed -> %d\n", CENTER->name, ret);
		display_chars(TEXT_Err);
		return;
	}
	distance = sensor_value_to_double(&value);
	if (distance < 1.25) {
		display_number(100 * distance, 10);
	} else {
		display_chars(DISPLAY_OFF);
	}
}

static void mode_show_presence(void)
{
	int i, ret;
	struct sensor_value value;
	uint8_t text[4] = { CHAR_OFF, CHAR_OFF, CHAR_OFF, CHAR_OFF };

	for (i = 0; i < N_SENSORS; i++) {
		ret = sensor_sample_fetch(sensors[i]);
		if (ret) {
			printk("sensor_sample_fetch(%s) failed -> %d\n",
			       sensors[i]->name, ret);
			display_chars(TEXT_Err);
			return;
		}
		ret = sensor_channel_get(sensors[i],
					 SENSOR_CHAN_PROX,
					 &value);
		if (ret) {
			printk("sensor_channel_get(%s) failed -> %d\n",
			       sensors[i]->name, ret);
			display_chars(TEXT_Err);
			return;
		}

		if (value.val1) {
			text[i] = CHAR_UNDERSCORE | CHAR_DASH | CHAR_OVERLINE;
		}
	}
	display_chars(text);
}

static size_t current_mode;
static fsm_state modes[] = {
	mode_show_distance,
	mode_show_presence,
};

static void change_mode(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	(void) dev;
	(void) cb;
	(void) pins;

	int64_t now = k_uptime_get();

	if (now - last_mode_change > 300) {
		current_mode = (current_mode + 1) % N_MODES;
		last_mode_change = now;
	}
}


int main(void)
{
	struct gpio_callback button_cb_data;
	const uint8_t Hello[4] = { CHAR_H, CHAR_E, CHAR_PIPE | CHAR_1, CHAR_0 };
	const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

	display_chars(Hello);
	k_sleep(K_MSEC(1000));

	if (!device_is_ready(button.port)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_RISING);
	gpio_init_callback(&button_cb_data, change_mode, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	while (1) {
		modes[current_mode]();
	}
	return 0;
}

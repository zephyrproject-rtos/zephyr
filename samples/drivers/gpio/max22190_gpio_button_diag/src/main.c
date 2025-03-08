/*
 * Copyright (c) 2025 Analog Devices Inc.
 * Copyright (c) 2025 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio/gpio_diag.h> /* for diagnostic flags */

#define SLEEP_TIME_MS	10

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

struct device_info {
	struct k_work work;
	struct gpio_dt_spec button;
} max22190_dev;

void print_error(struct k_work *item)
{
	struct device_info *work_dev = CONTAINER_OF(item, struct device_info, work);

	gpio_port_diag_fetch_dt(&work_dev->button);

	uint8_t val;

	gpio_port_diag_get_dt(&work_dev->button, GPIO_DIAG_WIRE_BREAK, &val);
	printk("Wire Break generic: [%d]\n", val);

	gpio_port_diag_get_dt(&work_dev->button, GPIO_DIAG_SUPPLY_ERR, &val);
	printk("Power supply issue: [%d]\n", val);
}

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	k_work_submit(&max22190_dev.work);
}

int main(void)
{
	int ret;

	max22190_dev.button = button;

	if (!gpio_is_ready_dt(&max22190_dev.button)) {
		printk("Error: button device %s is not ready\n",
		       max22190_dev.button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&max22190_dev.button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, max22190_dev.button.port->name, max22190_dev.button.pin);
		return 0;
	}

	gpio_init_diag_callback(&button_cb_data, button_pressed);
	ret = gpio_add_diag_callback(max22190_dev.button.port, &button_cb_data);
	if (ret != 0) {
		printk("Error [%d] [gpio_add_diag_callback]\n", ret);
	}

	printk("Set up button at %s pin %d\n", max22190_dev.button.port->name,
	       max22190_dev.button.pin);

	if (led.port && !gpio_is_ready_dt(&led)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n",
		       ret, led.port->name);
		led.port = NULL;
	}
	if (led.port) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n",
			       ret, led.port->name, led.pin);
			led.port = NULL;
		} else {
			printk("Set up LED at %s pin %d\n", led.port->name, led.pin);
		}
	}

	k_work_init(&max22190_dev.work, print_error);

	printk("Press the button\n");
	if (led.port) {

		while (1) {
			/* If we have an LED, match its state to the button's. */
			int val = gpio_pin_get_dt(&max22190_dev.button);

			if (val >= 0) {
				gpio_pin_set_dt(&led, val);
			}

			k_msleep(SLEEP_TIME_MS);
		}
	}
	return 0;
}

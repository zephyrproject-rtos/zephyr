/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_DECLARE(ead_central_sample, CONFIG_BT_EAD_LOG_LEVEL);

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

extern int run_central_sample(int get_passkey_confirmation(struct bt_conn *conn),
			      uint8_t *received_data, size_t received_data_size,
			      struct key_material *keymat);

static struct k_poll_signal button_pressed_signal;

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("Button pressed...");

	k_poll_signal_raise(&button_pressed_signal, 0);
	k_sleep(K_SECONDS(1));
	k_poll_signal_reset(&button_pressed_signal);
}

static int get_passkey_confirmation(struct bt_conn *conn)
{
	int err;

	printk("Confirm passkey by pressing button at %s pin %d...\n", button.port->name,
	       button.pin);

	await_signal(&button_pressed_signal);

	err = bt_conn_auth_passkey_confirm(conn);
	if (err) {
		LOG_DBG("Failed to confirm passkey.");
		return -1;
	}

	printk("Passkey confirmed.\n");

	return 0;
}

static int setup_btn(void)
{
	int ret;

	k_poll_signal_init(&button_pressed_signal);

	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Error: button device %s is not ready", button.port->name);
		return -1;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, button.port->name,
			button.pin);
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret,
			button.port->name, button.pin);
		return -1;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	LOG_DBG("Set up button at %s pin %d", button.port->name, button.pin);

	return 0;
}

int main(void)
{
	int err;

	err = setup_btn();
	if (err) {
		return 0;
	}

	LOG_DBG("Starting central sample...");

	(void)run_central_sample(get_passkey_confirmation, NULL, 0, NULL);

	return 0;
}

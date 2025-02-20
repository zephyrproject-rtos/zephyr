/*
 * Copyright 2024 Ian Morris
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_da1453x);

#define DT_DRV_COMPAT renesas_bt_hci_da1453x

int bt_hci_transport_setup(const struct device *h4)
{
	int err = 0;

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	char c;
	struct gpio_dt_spec bt_reset = GPIO_DT_SPEC_GET(DT_DRV_INST(0), reset_gpios);

	if (!gpio_is_ready_dt(&bt_reset)) {
		LOG_ERR("Error: failed to configure bt_reset %s pin %d", bt_reset.port->name,
			bt_reset.pin);
		return -EIO;
	}

	/* Set bt_reset as output and activate DA1453x reset */
	err = gpio_pin_configure_dt(&bt_reset, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("Error %d: failed to configure bt_reset %s pin %d", err,
			bt_reset.port->name, bt_reset.pin);
		return err;
	}

	k_sleep(K_MSEC(DT_INST_PROP_OR(0, reset_assert_duration_ms, 0)));

	/* Release the DA1453x from reset */
	err = gpio_pin_configure_dt(&bt_reset, GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}

	/* Wait for the DA1453x to boot */
	k_sleep(K_MSEC(DT_INST_PROP(0, boot_duration_ms)));

	/* Drain bytes */
	while (h4 && uart_fifo_read(h4, &c, 1)) {
		continue;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, reset_gpios) */

	return err;
}

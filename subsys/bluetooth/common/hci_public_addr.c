/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>

void bt_hci_set_public_addr(const struct device *dev, const bt_addr_t *addr)
{
	struct bt_hci_driver_data *data = dev->data;

	bt_addr_copy(&data->public_addr, addr);
	data->public_addr_set = !bt_addr_eq(addr, BT_ADDR_NONE);
}

const bt_addr_t *bt_hci_get_public_addr(const struct device *dev)
{
	const struct bt_hci_driver_data *data = dev->data;

	if (!data->public_addr_set) {
		return BT_ADDR_NONE;
	}

	return &data->public_addr;
}

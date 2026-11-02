/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>

int bt_hci_state_cb_set(const struct device *dev, bt_hci_state_cb_t cb)
{
	struct bt_hci_driver_data *data = dev->data;

	if (cb != NULL && data->state_cb != NULL && data->state_cb != cb) {
		return -EALREADY;
	}

	data->state_cb = cb;

	return 0;
}

static void report(const struct device *dev, bool available)
{
	const struct bt_hci_driver_data *data = dev->data;

	if (data->state_cb != NULL) {
		data->state_cb(dev, available);
	}
}

void bt_hci_report_unavailable(const struct device *dev)
{
	report(dev, false);
}

void bt_hci_report_available(const struct device *dev)
{
	report(dev, true);
}

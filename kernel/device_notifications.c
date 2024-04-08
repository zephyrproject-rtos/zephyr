/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

ZBUS_CHAN_DEFINE(device_failed_chan,
		struct device_notification,
		NULL,
		NULL,
		ZBUS_OBSERVERS_EMPTY,
		ZBUS_MSG_INIT(0));

static int device_failed_notify(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	for (const struct device *dev = devs; dev < devs + devc; dev++) {
		if (dev->state->status == DEVICE_INIT_STATUS_FAILED) {
			struct device_notification msg = {
				.dev = dev,
				.type = DEVICE_NOTIFICATION_FAILURE,
			};
			zbus_chan_pub(&device_failed_chan, &msg,
				      K_NO_WAIT);
		}
	}

	return 0;
}
SYS_INIT(device_failed_notify, APPLICATION, CONFIG_DEVICE_NOTIFICATIONS_PRIORITY);

void device_notification_send(const struct device *dev,
			      enum device_notification_type type)
{
	struct device_notification msg = {
		.dev = dev,
		.type = type,
	};

	/* Also notify failures on common channel */
	if (type == DEVICE_NOTIFICATION_FAILURE) {
		zbus_chan_pub(&device_failed_chan, &msg, K_NO_WAIT);
	}
	zbus_chan_pub(dev->channel, &msg, K_NO_WAIT);
}

/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rc_loopback

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/sys/atomic.h>

struct rc_loopback_data {
	struct rc_dev_data rc;
	atomic_t rx_enabled;
};

static int rc_loopback_get_caps(const struct device *dev, struct rc_caps *caps)
{
	ARG_UNUSED(dev);

	*caps = (struct rc_caps){
		.roles = RC_ROLE_RAW_RX,
		.rx_resolution_ns = 1000U,
	};

	return 0;
}

static int rc_loopback_rx_enable(const struct device *dev, bool enable)
{
	struct rc_loopback_data *data = dev->data;

	atomic_set(&data->rx_enabled, enable ? 1 : 0);

	return 0;
}

bool rc_loopback_rx_is_enabled(const struct device *dev)
{
	struct rc_loopback_data *data = dev->data;

	return atomic_get(&data->rx_enabled) != 0;
}

static DEVICE_API(rc, rc_loopback_api) = {
	.get_caps = rc_loopback_get_caps,
	.rx_enable = rc_loopback_rx_enable,
};

#define RC_LOOPBACK_DEFINE(inst)                                                                   \
	static struct rc_loopback_data rc_loopback_data_##inst;                                    \
	RC_DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &rc_loopback_data_##inst, NULL, POST_KERNEL,    \
				 CONFIG_RC_INIT_PRIORITY, &rc_loopback_api);

DT_INST_FOREACH_STATUS_OKAY(RC_LOOPBACK_DEFINE)

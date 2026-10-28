/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rc_loopback

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/sys/atomic.h>

#define RC_LOOPBACK_TIMEOUT_MIN_US 1000U
#define RC_LOOPBACK_TIMEOUT_MAX_US 1000000U

struct rc_loopback_config {
	bool hw_rx_timeout;
};

struct rc_loopback_data {
	struct rc_dev_data rc;
	atomic_t rx_enabled;
	atomic_t rx_timeout_us;
};

static int rc_loopback_get_caps(const struct device *dev, struct rc_caps *caps)
{
	const struct rc_loopback_config *config = dev->config;

	*caps = (struct rc_caps){
		.roles = RC_ROLE_RAW_RX,
		.rx_resolution_ns = 1000U,
	};

	if (config->hw_rx_timeout) {
		caps->rx_timeout_min_us = RC_LOOPBACK_TIMEOUT_MIN_US;
		caps->rx_timeout_max_us = RC_LOOPBACK_TIMEOUT_MAX_US;
	}

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

static int rc_loopback_set_rx_timeout(const struct device *dev, uint32_t us)
{
	struct rc_loopback_data *data = dev->data;

	atomic_set(&data->rx_timeout_us, (atomic_val_t)us);

	return 0;
}

uint32_t rc_loopback_rx_timeout_us(const struct device *dev)
{
	struct rc_loopback_data *data = dev->data;

	return (uint32_t)atomic_get(&data->rx_timeout_us);
}

int rc_loopback_inject(const struct device *dev, const uint32_t *words, size_t count)
{
	if (!rc_loopback_rx_is_enabled(dev)) {
		return -EAGAIN;
	}

	for (size_t i = 0; i < count; i++) {
		int ret = rc_raw_store(dev, words[i]);

		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(rc, rc_loopback_api) = {
	.get_caps = rc_loopback_get_caps,
	.rx_enable = rc_loopback_rx_enable,
};

static DEVICE_API(rc, rc_loopback_hw_timeout_api) = {
	.get_caps = rc_loopback_get_caps,
	.rx_enable = rc_loopback_rx_enable,
	.set_rx_timeout = rc_loopback_set_rx_timeout,
};

#define RC_LOOPBACK_API(inst)                                                                      \
	COND_CODE_1(DT_INST_PROP(inst, hw_rx_timeout), (&rc_loopback_hw_timeout_api),              \
		    (&rc_loopback_api))

#define RC_LOOPBACK_DEFINE(inst)                                                                   \
	static const struct rc_loopback_config rc_loopback_config_##inst = {                       \
		.hw_rx_timeout = DT_INST_PROP(inst, hw_rx_timeout),                                \
	};                                                                                         \
	static struct rc_loopback_data rc_loopback_data_##inst;                                    \
	RC_DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &rc_loopback_data_##inst,                       \
				 &rc_loopback_config_##inst, POST_KERNEL, CONFIG_RC_INIT_PRIORITY, \
				 RC_LOOPBACK_API(inst));

DT_INST_FOREACH_STATUS_OKAY(RC_LOOPBACK_DEFINE)

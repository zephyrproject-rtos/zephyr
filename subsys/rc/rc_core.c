/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/rc.h>
#include <zephyr/logging/log.h>
#include <zephyr/rc/rc.h>

LOG_MODULE_REGISTER(rc, CONFIG_RC_LOG_LEVEL);

static struct rc_dev_data *rc_data(const struct device *dev)
{
	return dev->data;
}

int z_rc_device_init(const struct device *dev, int (*init_fn)(const struct device *dev))
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);
	int ret;

	if (init_fn != NULL) {
		ret = init_fn(dev);
		if (ret != 0) {
			return ret;
		}
	}

	ret = api->get_caps(dev, &rc_data(dev)->caps);
	if (ret != 0) {
		LOG_ERR("%s: get_caps failed (%d)", dev->name, ret);
		return ret;
	}

	return 0;
}

int rc_get_caps(const struct device *dev, struct rc_caps *caps)
{
	*caps = rc_data(dev)->caps;

	return 0;
}

int rc_rx_enable(const struct device *dev, bool enable)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);

	if (api->rx_enable == NULL) {
		return -ENOSYS;
	}

	return api->rx_enable(dev, enable);
}

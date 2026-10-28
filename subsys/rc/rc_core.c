/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/rc.h>
#include <zephyr/logging/log.h>
#include <zephyr/rc/rc.h>

#include "rc_internal.h"

LOG_MODULE_REGISTER(rc, CONFIG_RC_LOG_LEVEL);

int z_rc_device_init(const struct device *dev, int (*init_fn)(const struct device *dev),
		     const struct z_rc_init_cfg *cfg)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);
	struct rc_dev_data *data = rc_data(dev);
	int ret;

	data->dev = dev;
	data->ring = cfg->ring;
	data->decoder_state = cfg->decoder_state;

	if (init_fn != NULL) {
		ret = init_fn(dev);
		if (ret != 0) {
			return ret;
		}
	}

	ret = api->get_caps(dev, &data->caps);
	if (ret != 0) {
		LOG_ERR("%s: get_caps failed (%d)", dev->name, ret);
		return ret;
	}

	ret = rc_protocol_init(dev, cfg->protocols, cfg->num_protocols);
	if (ret != 0) {
		return ret;
	}

	rc_raw_init(dev);

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

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

#define RC_RX_TIMEOUT_DEFAULT_US 125000U
#define RC_RX_TIMEOUT_MARGIN_US  10000U

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

	rc_raw_init(dev, api->set_rx_timeout == NULL);

	if (cfg->rx_timeout_us == 0U) {
		atomic_set_bit(&data->flags, RC_FLAG_TIMEOUT_AUTO);
		return rc_rx_timeout_update(dev);
	}

	ret = rc_set_rx_timeout(dev, cfg->rx_timeout_us);
	if (ret != 0) {
		LOG_ERR("%s: rx timeout %u us failed (%d)", dev->name, cfg->rx_timeout_us, ret);
	}

	return ret;
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

static int rc_rx_timeout_apply(const struct device *dev, uint32_t us)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);
	struct rc_dev_data *data = rc_data(dev);
	int ret;

	if (api->set_rx_timeout != NULL) {
		if ((us < data->caps.rx_timeout_min_us) || (us > data->caps.rx_timeout_max_us)) {
			return -EINVAL;
		}

		ret = api->set_rx_timeout(dev, us);
		if (ret != 0) {
			return ret;
		}
	}

	atomic_set(&data->rx_timeout_us, (atomic_val_t)us);

	return 0;
}

int rc_set_rx_timeout(const struct device *dev, uint32_t us)
{
	int ret;

	if ((us == 0U) || (us > RC_RAW_DURATION_MASK)) {
		return -EINVAL;
	}

	ret = rc_rx_timeout_apply(dev, us);
	if (ret == 0) {
		atomic_clear_bit(&rc_data(dev)->flags, RC_FLAG_TIMEOUT_AUTO);
	}

	return ret;
}

int rc_rx_timeout_update(const struct device *dev)
{
	const struct rc_driver_api *api = DEVICE_API_GET(rc, dev);
	struct rc_dev_data *data = rc_data(dev);
	uint32_t us = rc_protocol_min_timeout_us(dev);
	int ret;

	if (!atomic_test_bit(&data->flags, RC_FLAG_TIMEOUT_AUTO)) {
		return 0;
	}

	us = (us == 0U) ? RC_RX_TIMEOUT_DEFAULT_US : (us + RC_RX_TIMEOUT_MARGIN_US);
	if (api->set_rx_timeout != NULL) {
		us = CLAMP(us, data->caps.rx_timeout_min_us, data->caps.rx_timeout_max_us);
	} else {
		us = MIN(us, RC_RAW_DURATION_MASK);
	}

	ret = rc_rx_timeout_apply(dev, us);
	if (ret != 0) {
		LOG_ERR("%s: rx timeout %u us failed (%d)", dev->name, us, ret);
	}

	return ret;
}

/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MBOX_H_
#error "Should only be included by zephyr/drivers/mbox.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_MBOX_INTERNAL_MBOX_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MBOX_INTERNAL_MBOX_IMPL_H_

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_mbox_send(const struct device *dev,
				   mbox_channel_id_t channel_id,
				   const struct mbox_msg *msg)
{
	const struct mbox_driver_api *api =
		(const struct mbox_driver_api *)dev->api;

	if (api->send == NULL) {
		return -ENOSYS;
	}

	return api->send(dev, channel_id, msg);
}

static inline int z_impl_mbox_mtu_get(const struct device *dev)
{
	const struct mbox_driver_api *api =
		(const struct mbox_driver_api *)dev->api;

	if (api->mtu_get == NULL) {
		return -ENOSYS;
	}

	return api->mtu_get(dev);
}

static inline int z_impl_mbox_set_enabled(const struct device *dev,
					  mbox_channel_id_t channel_id,
					  bool enabled)
{
	const struct mbox_driver_api *api =
		(const struct mbox_driver_api *)dev->api;

	if (api->set_enabled == NULL) {
		return -ENOSYS;
	}

	return api->set_enabled(dev, channel_id, enabled);
}

static inline uint32_t z_impl_mbox_max_channels_get(const struct device *dev)
{
	const struct mbox_driver_api *api =
		(const struct mbox_driver_api *)dev->api;

	if (api->max_channels_get == NULL) {
		return -ENOSYS;
	}

	return api->max_channels_get(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MBOX_INTERNAL_MBOX_IMPL_H_ */

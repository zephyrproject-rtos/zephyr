/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>

#define FAKE_DRV_NAME "FAKE_DRV"

struct fake_api {
	int (*sync_int_call)(struct device *dev);
	int (*sync_poll_call)(struct device *dev);
	int (*lock_int_call)(struct device *dev, int *val);
	int (*lock_poll_call)(struct device *dev, int *val);
	int (*no_to_call)(struct device *dev, int *val);

};

static inline int fake_sync_int_call(struct device *dev)
{
	const struct fake_api *api = (const struct fake_api *)dev->driver_api;

	return api->sync_int_call(dev);
}

static inline int fake_sync_poll_call(struct device *dev)
{
	const struct fake_api *api = (const struct fake_api *)dev->driver_api;

	return api->sync_poll_call(dev);
}

static inline int fake_lock_int_call(struct device *dev, int *val)
{
	const struct fake_api *api = (const struct fake_api *)dev->driver_api;

	return api->lock_int_call(dev, val);
}

static inline int fake_lock_poll_call(struct device *dev, int *val)
{
	const struct fake_api *api = (const struct fake_api *)dev->driver_api;

	return api->lock_poll_call(dev, val);
}

static inline int fake_no_timeout_call(struct device *dev, int *val)
{
	const struct fake_api *api = (const struct fake_api *)dev->driver_api;

	return api->no_to_call(dev, val);
}

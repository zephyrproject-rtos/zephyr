/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <counter.h>

int _impl_counter_start(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->start(dev);
}

int _impl_counter_stop(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->stop(dev);
}

u32_t _impl_counter_read(struct device *dev)
{
	const struct counter_driver_api *api = dev->driver_api;

	return api->read(dev);
}

int _impl_counter_get_pending_int(struct device *dev)
{
	struct counter_driver_api *api;

	api = (struct counter_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}

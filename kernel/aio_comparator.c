/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <device.h>
#include <aio_comparator.h>


int _impl_aio_cmp_disable(struct device *dev, u8_t index)
{
	const struct aio_cmp_driver_api *api = dev->driver_api;

	return api->disable(dev, index);
}

int _impl_aio_cmp_get_pending_int(struct device *dev)
{
	struct aio_cmp_driver_api *api;

	api = (struct aio_cmp_driver_api *)dev->driver_api;
	return api->get_pending_int(dev);
}

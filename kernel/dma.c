/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <dma.h>

int _impl_dma_start(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->driver_api;

	return api->start(dev, channel);
}

int _impl_dma_stop(struct device *dev, u32_t channel)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->driver_api;

	return api->stop(dev, channel);
}

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for DMA driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_H_
#error "Should only be included by zephyr/drivers/dma.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_INTERNAL_DMA_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_INTERNAL_DMA_IMPL_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_dma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	return api->start(dev, channel);
}

static inline int z_impl_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	return api->stop(dev, channel);
}

static inline int z_impl_dma_suspend(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api = (const struct dma_driver_api *)dev->api;

	if (api->suspend == NULL) {
		return -ENOSYS;
	}
	return api->suspend(dev, channel);
}

static inline int z_impl_dma_resume(const struct device *dev, uint32_t channel)
{
	const struct dma_driver_api *api = (const struct dma_driver_api *)dev->api;

	if (api->resume == NULL) {
		return -ENOSYS;
	}
	return api->resume(dev, channel);
}

static inline int z_impl_dma_request_channel(const struct device *dev,
					     void *filter_param)
{
	int i = 0;
	int channel = -EINVAL;
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;
	/* dma_context shall be the first one in dev data */
	struct dma_context *dma_ctx = (struct dma_context *)dev->data;

	if (dma_ctx->magic != DMA_MAGIC) {
		return channel;
	}

	for (i = 0; i < dma_ctx->dma_channels; i++) {
		if (!atomic_test_and_set_bit(dma_ctx->atomic, i)) {
			if (api->chan_filter &&
			    !api->chan_filter(dev, i, filter_param)) {
				atomic_clear_bit(dma_ctx->atomic, i);
				continue;
			}
			channel = i;
			break;
		}
	}

	return channel;
}

static inline void z_impl_dma_release_channel(const struct device *dev,
					      uint32_t channel)
{
	struct dma_context *dma_ctx = (struct dma_context *)dev->data;

	if (dma_ctx->magic != DMA_MAGIC) {
		return;
	}

	if ((int)channel < dma_ctx->dma_channels) {
		atomic_clear_bit(dma_ctx->atomic, channel);
	}

}

static inline int z_impl_dma_chan_filter(const struct device *dev,
					      int channel, void *filter_param)
{
	const struct dma_driver_api *api =
		(const struct dma_driver_api *)dev->api;

	if (api->chan_filter) {
		return api->chan_filter(dev, channel, filter_param);
	}

	return -ENOSYS;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_INTERNAL_DMA_IMPL_H_ */

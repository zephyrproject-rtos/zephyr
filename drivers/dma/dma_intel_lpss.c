/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_lpss

#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_intel_lpss.h>
#include "dma_dw_common.h"
#include <soc.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dma_intel_lpss, CONFIG_DMA_LOG_LEVEL);

struct dma_intel_lpss_cfg {
	struct dw_dma_dev_cfg dw_cfg;
};

int dma_intel_lpss_setup(const struct device *dev)
{
	struct dma_intel_lpss_cfg *dev_cfg = (struct dma_intel_lpss_cfg *)dev->config;

	if (dev_cfg->dw_cfg.base != 0) {
		return dw_dma_setup(dev);
	}

	return 0;
}

void dma_intel_lpss_set_base(const struct device *dev, uintptr_t base)
{
	struct dma_intel_lpss_cfg *dev_cfg = (struct dma_intel_lpss_cfg *)dev->config;

	dev_cfg->dw_cfg.base = base;
}

#ifdef CONFIG_DMA_64BIT
int dma_intel_lpss_reload(const struct device *dev, uint32_t channel,
			      uint64_t src, uint64_t dst, size_t size)
#else
int dma_intel_lpss_reload(const struct device *dev, uint32_t channel,
			      uint32_t src, uint32_t dst, size_t size)
#endif
{
	struct dw_dma_dev_data *const dev_data = dev->data;
	struct dma_intel_lpss_cfg *lpss_dev_cfg = (struct dma_intel_lpss_cfg *)dev->config;
	struct dw_dma_dev_cfg *const dev_cfg = &lpss_dev_cfg->dw_cfg;
	struct dw_dma_chan_data *chan_data;
	uint32_t ctrl_hi = 0;

	if (channel >= DW_CHAN_COUNT) {
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	chan_data->lli_current->sar = src;
	chan_data->lli_current->dar = dst;
	chan_data->ptr_data.current_ptr = dst;
	chan_data->ptr_data.buffer_bytes = size;

	ctrl_hi = dw_read(dev_cfg->base, DW_CTRL_HIGH(channel));
	ctrl_hi &= ~(DW_CTLH_DONE(1) | DW_CTLH_BLOCK_TS_MASK);
	ctrl_hi |= size & DW_CTLH_BLOCK_TS_MASK;

	chan_data->lli_current->ctrl_hi = ctrl_hi;
	chan_data->ptr_data.start_ptr = DW_DMA_LLI_ADDRESS(chan_data->lli_current,
							 chan_data->direction);
	chan_data->ptr_data.end_ptr = chan_data->ptr_data.start_ptr +
				    chan_data->ptr_data.buffer_bytes;
	chan_data->ptr_data.hw_ptr = chan_data->ptr_data.start_ptr;

	chan_data->state = DW_DMA_PREPARED;

	return 0;
}

int dma_intel_lpss_get_status(const struct device *dev, uint32_t channel,
			      struct dma_status *stat)
{
	struct dma_intel_lpss_cfg *lpss_dev_cfg = (struct dma_intel_lpss_cfg *)dev->config;
	struct dw_dma_dev_cfg *const dev_cfg = &lpss_dev_cfg->dw_cfg;
	struct dw_dma_dev_data *const dev_data = dev->data;
	struct dw_dma_chan_data *chan_data;
	uint32_t ctrl_hi;
	size_t current_length;
	bool done;

	if (channel >= DW_CHAN_COUNT) {
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];
	ctrl_hi = dw_read(dev_cfg->base, DW_CTRL_HIGH(channel));
	current_length = ctrl_hi & DW_CTLH_BLOCK_TS_MASK;
	done = ctrl_hi & DW_CTLH_DONE(1);

	if (!(dw_read(dev_cfg->base, DW_DMA_CHAN_EN) & DW_CHAN(channel))) {
		stat->busy = false;
		stat->pending_length = chan_data->ptr_data.buffer_bytes;
		return 0;
	}
	stat->busy = true;

	if (done) {
		stat->pending_length = 0;
	} else if (current_length == chan_data->ptr_data.buffer_bytes) {
		stat->pending_length = chan_data->ptr_data.buffer_bytes;
	} else {
		stat->pending_length =
			chan_data->ptr_data.buffer_bytes - current_length;
	}

	return 0;
}

void dma_intel_lpss_isr(const struct device *dev)
{
	dw_dma_isr(dev);
}

static const struct dma_driver_api dma_intel_lpss_driver_api = {
	.config = dw_dma_config,
	.start = dw_dma_start,
	.reload = dma_intel_lpss_reload,
	.get_status = dma_intel_lpss_get_status,
	.stop = dw_dma_stop,
};

#define DMA_INTEL_LPSS_INIT(n)						\
									\
	static struct dw_drv_plat_data dma_intel_lpss##n = {		\
		.chan[0] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[1] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
	};								\
									\
									\
	static struct dma_intel_lpss_cfg dma_intel_lpss##n##_config = {	\
		.dw_cfg = {						\
			.base = 0,					\
		},							\
	};								\
									\
	static struct dw_dma_dev_data dma_intel_lpss##n##_data = {	\
		.channel_data = &dma_intel_lpss##n,			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    NULL,					\
			    NULL,					\
			    &dma_intel_lpss##n##_data,			\
			    &dma_intel_lpss##n##_config, PRE_KERNEL_1,	\
			    CONFIG_DMA_INIT_PRIORITY,			\
			    &dma_intel_lpss_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(DMA_INTEL_LPSS_INIT)

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
	const struct device *parent;
};

static int dma_intel_lpss_init(const struct device *dev)
{
	struct dma_intel_lpss_cfg *dev_cfg = (struct dma_intel_lpss_cfg *)dev->config;
	uint32_t base;
	int ret;

	if (!device_is_ready(dev_cfg->parent)) {
		LOG_ERR("LPSS DMA parent not ready");
		ret = -ENODEV;
		goto out;
	}

	base = DEVICE_MMIO_GET(dev_cfg->parent) + DMA_INTEL_LPSS_OFFSET;
	dev_cfg->dw_cfg.base = base;

	ret = dw_dma_setup(dev);

	if (ret != 0) {
		LOG_ERR("failed to initialize LPSS DMA %s", dev->name);
		goto out;
	}
	ret = 0;
out:
	return ret;
}

void dma_intel_lpss_isr(const struct device *dev)
{
	dw_dma_isr(dev);
}

static const struct dma_driver_api dma_intel_lpss_driver_api = {
	.config = dw_dma_config,
	.start = dw_dma_start,
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
		.parent = DEVICE_DT_GET(DT_INST_PHANDLE(n, dma_parent)),\
	};								\
									\
	static struct dw_dma_dev_data dma_intel_lpss##n##_data = {	\
		.channel_data = &dma_intel_lpss##n,			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &dma_intel_lpss_init,			\
			    NULL,					\
			    &dma_intel_lpss##n##_data,			\
			    &dma_intel_lpss##n##_config, POST_KERNEL,	\
			    DMA_INTEL_LPSS_INIT_PRIORITY,		\
			    &dma_intel_lpss_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(DMA_INTEL_LPSS_INIT)

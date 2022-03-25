/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_dma

#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/dma.h>
#include <soc.h>
#include "dma_dw_common.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_dw);

/* Device constant configuration parameters */
struct dw_dma_cfg {
	struct dw_dma_dev_cfg dw_cfg;
	void (*irq_config)(void);
};

static int dw_dma_init(const struct device *dev)
{
	const struct dw_dma_cfg *const dev_cfg = dev->config;

	/* Disable all channels and Channel interrupts */
	dw_dma_setup(dev);

	/* Configure interrupts */
	dev_cfg->irq_config();

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct dma_driver_api dw_dma_driver_api = {
	.config = dw_dma_config,
	.reload = dw_dma_reload,
	.start = dw_dma_transfer_start,
	.stop = dw_dma_transfer_stop,
};

#define DW_DMAC_INIT(inst)						\
									\
	static struct dw_drv_plat_data dmac##inst = {			\
		.chan[0] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[1] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[2] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[3] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[4] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[5] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[6] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[7] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
	};								\
									\
	static void dw_dma##inst##_irq_config(void);			\
									\
	static const struct dw_dma_cfg dw_dma##inst##_config = {	\
		.dw_cfg = {						\
			.base = DT_INST_REG_ADDR(inst),			\
		},							\
		.irq_config = dw_dma##inst##_irq_config			\
	};								\
									\
	static struct dw_dma_dev_data dw_dma##inst##_data = {		\
		.channel_data = &dmac##inst,				\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    &dw_dma_init,				\
			    NULL,					\
			    &dw_dma##inst##_data,			\
			    &dw_dma##inst##_config, POST_KERNEL,	\
			    CONFIG_DMA_INIT_PRIORITY,			\
			    &dw_dma_driver_api);			\
									\
	static void dw_dma##inst##_irq_config(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority), dw_dma_isr,	\
			    DEVICE_DT_INST_GET(inst),			\
			    DT_INST_IRQ(inst, sense));			\
		irq_enable(DT_INST_IRQN(inst));				\
	}

DT_INST_FOREACH_STATUS_OKAY(DW_DMAC_INIT)

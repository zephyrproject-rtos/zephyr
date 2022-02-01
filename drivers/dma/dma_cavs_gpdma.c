/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_cavs_gpdma

#define GPDMA_CTL_OFFSET 0x004
#define GPDMA_CTL_FDCGB BIT(0)

#include "dma_dw_common.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_cavs_gpdma);


/* Device run time data */
struct cavs_gpdma_data {
	struct dw_dma_dev_data dw_data;
};

/* Device constant configuration parameters */
struct cavs_gpdma_cfg {
	struct dw_dma_dev_cfg dw_cfg;
	uint32_t shim;
	uint32_t irq;
};

/* Disables automatic clock gating (force disable clock gate) */
static void cavs_gpdma_clock_enable(const struct device *dev)
{
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;

	sys_write32(GPDMA_CTL_FDCGB, reg);
}


int cavs_gpdma_init(const struct device *dev)
{
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;

	/* Disable dynamic clock gating appropriately before initializing */
	cavs_gpdma_clock_enable(dev);

	/* Disable all channels and Channel interrupts */
	dw_dma_setup(dev);

	/* Configure interrupts */
	irq_enable(dev_cfg->irq);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct dma_driver_api cavs_gpdma_driver_api = {
	.config = dw_dma_config,
	.reload = dw_dma_reload,
	.start = dw_dma_transfer_start,
	.stop = dw_dma_transfer_stop,
};


#define CAVS_GPDMA_CHAN_ARB_DATA(inst)					\
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
	}

#define CAVS_GPDMA_INIT(inst)						\
	CAVS_GPDMA_CHAN_ARB_DATA(inst);					\
									\
	static const struct cavs_gpdma_cfg cavs_gpdma##inst##_config = { \
		.dw_cfg = {						\
			.base = DT_INST_REG_ADDR(inst),			\
		},							\
		.shim = DT_INST_PROP_BY_IDX(inst, shim, 0),		\
		.irq = DT_INST_IRQN(inst),				\
	};								\
									\
	static struct cavs_gpdma_data cavs_gpdma##inst##_data = {	\
		.dw_data = {						\
			.channel_data = &dmac##inst,			\
		},							\
	};								\
									\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			      &cavs_gpdma_init,				\
			      NULL,					\
			      &cavs_gpdma##inst##_data,			\
			      &cavs_gpdma##inst##_config, POST_KERNEL,	\
			      CONFIG_DMA_INIT_PRIORITY,			\
			      &cavs_gpdma_driver_api);			\
									\
	IRQ_CONNECT(DT_INST_IRQN(inst),					\
		    DT_INST_IRQ(inst, priority), dw_dma_isr,		\
		    DEVICE_DT_INST_GET(inst),				\
		    DT_INST_IRQ(inst, sense));				\
									\

DT_INST_FOREACH_STATUS_OKAY(CAVS_GPDMA_INIT)

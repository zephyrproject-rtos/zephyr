/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#define DT_DRV_COMPAT intel_cavs_gpdma

#define GPDMA_CTL_OFFSET 0x0004
#define GPDMA_CTL_FDCGB BIT(0)

#define GPDMA_CHLLPC_OFFSET(channel) (0x0010 + (channel) * 0x10)
#define GPDMA_CHLLPC_EN BIT(7)
#define GPDMA_CHLLPC_DHRS(x) SET_BITS(6, 0, x)

#define GPDMA_CHLLPL(channel) (0x0018 + (channel) * 0x10)
#define GPDMA_CHLLPU(channel) (0x001c + (channel) * 0x10)

#include "dma_dw_common.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_cavs_gpdma);


/* Device run time data */
struct cavs_gpdma_data {
	struct dw_dma_dev_data dw_data;
};

/* Device constant configuration parameters */
struct cavs_gpdma_cfg {
	struct dw_dma_dev_cfg dw_cfg;
	uint32_t shim;
};

/* Disables automatic clock gating (force disable clock gate) */
static void cavs_gpdma_clock_enable(const struct device *dev)
{
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t reg = dev_cfg->shim + GPDMA_CTL_OFFSET;

	sys_write32(GPDMA_CTL_FDCGB, reg);
}

static void cavs_gpdma_llp_config(const struct device *dev, uint32_t channel,
				  uint32_t addr)
{
#ifdef CONFIG_DMA_CAVS_GPDMA_HAS_LLP
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;

	dw_write(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel), GPDMA_CHLLPC_DHRS(addr));
#endif
}

static inline void cavs_gpdma_llp_enable(const struct device *dev,
					      uint32_t channel)
{
#ifdef CONFIG_DMA_CAVS_GPDMA_HAS_LLP
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t val;

	val = dw_read(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel));
	if (!(val & GPDMA_CHLLPC_EN)) {
		dw_write(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel), val | GPDMA_CHLLPC_EN);
	}
#endif
}

static inline void cavs_gpdma_llp_disable(const struct device *dev,
					       uint32_t channel)
{
#ifdef CONFIG_DMA_CAVS_GPDMA_HAS_LLP
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;
	uint32_t val;

	val = dw_read(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel));
	dw_write(dev_cfg->shim, GPDMA_CHLLPC_OFFSET(channel), val | GPDMA_CHLLPC_EN);
#endif
}

static inline void cavs_gpdma_llp_read(const struct device *dev,
					    uint32_t channel,
					    uint32_t *llp_l,
					    uint32_t *llp_u)
{
#ifdef CONFIG_DMA_CAVS_GPDMA_HAS_LLP
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;

	*llp_l = dw_read(dev_cfg->shim, GPDMA_CHLLPL(channel));
	*llp_u = dw_read(dev_cfg->shim, GPDMA_CHLLPU(channel));
#endif
}


static int cavs_gpdma_config(const struct device *dev, uint32_t channel,
		      struct dma_config *cfg)
{
	int res = dw_dma_config(dev, channel, cfg);

	if (res != 0) {
		return res;
	}

	struct dma_block_config *block_cfg = cfg->head_block;

	/* Assume all scatter/gathers are for the same device? */
	switch (cfg->channel_direction) {
	case MEMORY_TO_PERIPHERAL:
		LOG_DBG("%s: dma %s configuring llp for destination %x",
			__func__, dev->name, block_cfg->dest_address);
		cavs_gpdma_llp_config(dev, channel, block_cfg->dest_address);
		break;
	case PERIPHERAL_TO_MEMORY:
		LOG_DBG("%s: dma %s configuring llp for source %x",
			__func__, dev->name, block_cfg->source_address);
		cavs_gpdma_llp_config(dev, channel, block_cfg->source_address);
		break;
	default:
		break;
	}

	return res;
}

static int cavs_gpdma_start(const struct device *dev, uint32_t channel)
{
	int ret;

	cavs_gpdma_llp_enable(dev, channel);
	ret = dw_dma_start(dev, channel);
	if (ret != 0) {
		cavs_gpdma_llp_disable(dev, channel);
	}
	return ret;
}

static int cavs_gpdma_stop(const struct device *dev, uint32_t channel)
{
	int ret;

	ret = dw_dma_stop(dev, channel);
	if (ret == 0) {
		cavs_gpdma_llp_disable(dev, channel);
	}
	return ret;
}

int cavs_gpdma_copy(const struct device *dev, uint32_t channel,
		    uint32_t src, uint32_t dst, size_t size)
{
	struct dw_dma_dev_data *const dev_data = dev->data;
	struct dw_dma_chan_data *chan_data;
	int i = 0;

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	/* default action is to clear the DONE bit for all LLI making
	 * sure the cache is coherent between DSP and DMAC.
	 */
	for (i = 0; i < chan_data->lli_count; i++) {
		chan_data->lli[i].ctrl_hi &= ~DW_CTLH_DONE(1);
	}

	chan_data->ptr_data.current_ptr += size;
	if (chan_data->ptr_data.current_ptr >= chan_data->ptr_data.end_ptr) {
		chan_data->ptr_data.current_ptr = chan_data->ptr_data.start_ptr +
			(chan_data->ptr_data.current_ptr - chan_data->ptr_data.end_ptr);
	}

	return 0;
}

int cavs_gpdma_init(const struct device *dev)
{
	const struct cavs_gpdma_cfg *const dev_cfg = dev->config;

	/* Disable dynamic clock gating appropriately before initializing */
	cavs_gpdma_clock_enable(dev);

	/* Disable all channels and Channel interrupts */
	int ret = dw_dma_setup(dev);

	if (ret != 0) {
		LOG_ERR("%s: dma %s failed to initialize", __func__, dev->name);
		goto out;
	}

	/* Configure interrupts */
	dev_cfg->dw_cfg.irq_config();

	LOG_INF("%s: dma %s initialized", __func__, dev->name);

out:
	return 0;
}


static const struct dma_driver_api cavs_gpdma_driver_api = {
	.config = cavs_gpdma_config,
	.reload = cavs_gpdma_copy,
	.start = cavs_gpdma_start,
	.stop = cavs_gpdma_stop,
	.suspend = dw_dma_suspend,
	.resume = dw_dma_resume,
	.get_status = dw_dma_get_status,
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
	static void cavs_gpdma##inst##_irq_config(void);		\
									\
	static const struct cavs_gpdma_cfg cavs_gpdma##inst##_config = { \
		.dw_cfg = {						\
			.base = DT_INST_REG_ADDR(inst),			\
			.irq_config = cavs_gpdma##inst##_irq_config,	\
		},							\
		.shim = DT_INST_PROP_BY_IDX(inst, shim, 0),		\
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
	static void cavs_gpdma##inst##_irq_config(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority), dw_dma_isr,	\
			    DEVICE_DT_INST_GET(inst),			\
			    DT_INST_IRQ(inst, sense));			\
		irq_enable(DT_INST_IRQN(inst));				\
	}

DT_INST_FOREACH_STATUS_OKAY(CAVS_GPDMA_INIT)

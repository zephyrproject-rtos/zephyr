/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_sedi_dma

#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/devicetree.h>
#include <zephyr/cache.h>
#include <soc.h>

#include "sedi_driver_dma.h"
#include "sedi_driver_core.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sedi_dma, CONFIG_DMA_LOG_LEVEL);

extern void dma_isr(sedi_dma_t dma_device);

struct dma_sedi_config_info {
	sedi_dma_t peripheral_id; /* Controller instance. */
	uint8_t chn_num;
	void (*irq_config)(void);
};

struct dma_sedi_driver_data {
	struct dma_config dma_configs[DMA_CHANNEL_NUM];
};

#define DEV_DATA(dev) ((struct dma_sedi_driver_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct dma_sedi_config_info *const)(dev)->config)

/*
 * this function will be called when dma transferring is completed
 * or error happened
 */
static void dma_handler(sedi_dma_t dma_device, int channel, int event_id,
			void *args)
{
	ARG_UNUSED(args);
	const struct device *dev = (const struct device *)args;
	struct dma_sedi_driver_data *const data = DEV_DATA(dev);
	struct dma_config *config = &(data->dma_configs[channel]);

	/* run user-defined callback */
	if (config->dma_callback) {
		if ((event_id == SEDI_DMA_EVENT_TRANSFER_DONE) &&
		    (config->complete_callback_en)) {
			config->dma_callback(dev, config->user_data,
					channel, 0);
		} else if (!config->error_callback_dis) {
			config->dma_callback(dev, config->user_data,
					channel, event_id);
		}
	}
}

/* map width to certain macros*/
static int width_index(uint32_t num_bytes, uint32_t *index)
{
	switch (num_bytes) {
	case 1:
		*index = DMA_TRANS_WIDTH_8;
		break;
	case 2:
		*index = DMA_TRANS_WIDTH_16;
		break;
	case 4:
		*index = DMA_TRANS_WIDTH_32;
		break;
	case 8:
		*index = DMA_TRANS_WIDTH_64;
		break;
	case 16:
		*index = DMA_TRANS_WIDTH_128;
		break;
	case 32:
		*index = DMA_TRANS_WIDTH_256;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* map burst size to certain macros*/
static int burst_index(uint32_t num_units, uint32_t *index)
{
	switch (num_units) {
	case 1:
		*index = DMA_BURST_TRANS_LENGTH_1;
		break;
	case 4:
		*index = DMA_BURST_TRANS_LENGTH_4;
		break;
	case 8:
		*index = DMA_BURST_TRANS_LENGTH_8;
		break;
	case 16:
		*index = DMA_BURST_TRANS_LENGTH_16;
		break;
	case 32:
		*index = DMA_BURST_TRANS_LENGTH_32;
		break;
	case 64:
		*index = DMA_BURST_TRANS_LENGTH_64;
		break;
	case 128:
		*index = DMA_BURST_TRANS_LENGTH_128;
		break;
	case 256:
		*index = DMA_BURST_TRANS_LENGTH_256;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void dma_config_convert(struct dma_config *config,
			       dma_memory_type_t *src_mem,
			       dma_memory_type_t *dst_mem,
			       uint8_t *sedi_dma_dir)
{

	*src_mem = DMA_SRAM_MEM;
	*dst_mem = DMA_SRAM_MEM;
	*sedi_dma_dir = MEMORY_TO_MEMORY;
	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
	case PERIPHERAL_TO_PERIPHERAL:
		*sedi_dma_dir = config->channel_direction;
		break;
	case MEMORY_TO_HOST:
		*dst_mem = DMA_DRAM_MEM;
		break;
	case HOST_TO_MEMORY:
		*src_mem = DMA_DRAM_MEM;
		break;
#ifdef MEMORY_TO_IMR
	case MEMORY_TO_IMR:
		*dst_mem = DMA_UMA_MEM;
		break;
#endif
#ifdef IMR_TO_MEMORY
	case IMR_TO_MEMORY:
		*src_mem = DMA_UMA_MEM;
		break;
#endif
	}
}

/* config basic dma */
static int dma_sedi_apply_common_config(sedi_dma_t dev, uint32_t channel,
					struct dma_config *config, uint8_t *dir)
{
	uint8_t direction = MEMORY_TO_MEMORY;
	dma_memory_type_t src_mem = DMA_SRAM_MEM, dst_mem = DMA_SRAM_MEM;

	dma_config_convert(config, &src_mem, &dst_mem, &direction);

	if (dir) {
		*dir = direction;
	}

	/* configure dma transferring direction*/
	sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_DIRECTION,
			 direction);

	if (direction == MEMORY_TO_MEMORY) {
		sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_SR_MEM_TYPE,
				 src_mem);
		sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_DT_MEM_TYPE,
				 dst_mem);
	} else if (direction == MEMORY_TO_PERIPHERAL) {
		sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_HS_DEVICE_ID,
				 config->dma_slot);
		sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_HS_POLARITY,
				 DMA_HS_POLARITY_HIGH);
		sedi_dma_control(dev, channel,
				 SEDI_CONFIG_DMA_HS_DEVICE_ID_PER_DIR,
				 DMA_HS_PER_TX);
	} else if (direction == PERIPHERAL_TO_MEMORY) {
		sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_HS_DEVICE_ID,
				 config->dma_slot);
		sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_HS_POLARITY,
				 DMA_HS_POLARITY_HIGH);
		sedi_dma_control(dev, channel,
				 SEDI_CONFIG_DMA_HS_DEVICE_ID_PER_DIR,
				 DMA_HS_PER_RX);
	} else {
		return -1;
	}
	return 0;
}

static int dma_sedi_apply_single_config(sedi_dma_t dev, uint32_t channel,
					struct dma_config *config)
{
	int ret = 0;
	uint32_t temp = 0;

	ret = dma_sedi_apply_common_config(dev, channel, config, NULL);
	if (ret != 0) {
		goto INVALID_ARGS;
	}
	/* configurate dma width of source data*/
	ret = width_index(config->source_data_size, &temp);
	if (ret != 0) {
		goto INVALID_ARGS;
	}
	sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_SR_TRANS_WIDTH, temp);

	/* configurate dma width of destination data*/
	ret = width_index(config->dest_data_size, &temp);
	if (ret != 0) {
		goto INVALID_ARGS;
	}
	sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_DT_TRANS_WIDTH, temp);

	/* configurate dma burst size*/
	ret = burst_index(config->source_burst_length, &temp);
	if (ret != 0) {
		goto INVALID_ARGS;
	}
	sedi_dma_control(dev, channel, SEDI_CONFIG_DMA_BURST_LENGTH, temp);
	return 0;

INVALID_ARGS:
	return ret;
}

static int dma_sedi_chan_config(const struct device *dev, uint32_t channel,
				struct dma_config *config)
{
	if ((dev == NULL) || (channel >= DEV_CFG(dev)->chn_num)
		|| (config == NULL)
		|| (config->block_count != 1)) {
		goto INVALID_ARGS;
	}

	const struct dma_sedi_config_info *const info = DEV_CFG(dev);
	struct dma_sedi_driver_data *const data = DEV_DATA(dev);

	memcpy(&(data->dma_configs[channel]), config, sizeof(struct dma_config));

	/* initialize the dma controller, following the sedi api*/
	sedi_dma_event_cb_t cb = dma_handler;

	sedi_dma_init(info->peripheral_id, (int)channel, cb, (void *)dev);

	return 0;

INVALID_ARGS:
	return -1;
}

static int dma_sedi_reload(const struct device *dev, uint32_t channel,
			      uint64_t src, uint64_t dst, size_t size)
{
	if ((dev == NULL) || (channel >= DEV_CFG(dev)->chn_num)) {
		LOG_ERR("dma reload failed for invalid args");
		return -ENOTSUP;
	}

	int ret = 0;
	struct dma_sedi_driver_data *const data = DEV_DATA(dev);
	struct dma_config *config = &(data->dma_configs[channel]);
	struct dma_block_config *block_config;

	if ((config == NULL) || (config->head_block == NULL)) {
		LOG_ERR("dma reload failed, no config found");
		return -ENOTSUP;
	}
	block_config = config->head_block;

	if ((config->block_count == 1) || (block_config->next_block == NULL)) {
		block_config->source_address = src;
		block_config->dest_address = dst;
		block_config->block_size = size;
	} else {
		LOG_ERR("no reload support for multi-linkedlist mode");
		return -ENOTSUP;
	}
	return ret;
}

static int dma_sedi_start(const struct device *dev, uint32_t channel)
{
	if ((dev == NULL) || (channel >= DEV_CFG(dev)->chn_num)) {
		LOG_ERR("dma transferring failed for invalid args");
		return -ENOTSUP;
	}

	int ret = -1;
	const struct dma_sedi_config_info *const info = DEV_CFG(dev);
	struct dma_sedi_driver_data *const data = DEV_DATA(dev);
	struct dma_config *config = &(data->dma_configs[channel]);
	struct dma_block_config *block_config = config->head_block;
	uint64_t src_addr, dst_addr;

	if (config->block_count == 1) {
		/* call sedi start function */
		ret = dma_sedi_apply_single_config(info->peripheral_id,
						   channel, config);
		if (ret) {
			goto ERR;
		}
		src_addr = block_config->source_address;
		dst_addr = block_config->dest_address;

		ret = sedi_dma_start_transfer(info->peripheral_id, channel,
						src_addr, dst_addr, block_config->block_size);
	} else {
		LOG_ERR("MULTIPLE_BLOCK CONFIG is not set");
		goto ERR;
	}

	if (ret != SEDI_DRIVER_OK) {
		goto ERR;
	}

	return ret;

ERR:
	LOG_ERR("dma transfer failed");
	return ret;
}

static int dma_sedi_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_sedi_config_info *const info = DEV_CFG(dev);

	LOG_DBG("stopping dma: %p, %d", dev, channel);
	sedi_dma_abort_transfer(info->peripheral_id, channel);

	return 0;
}

static DEVICE_API(dma, dma_funcs) = {
	.config = dma_sedi_chan_config,
	.start = dma_sedi_start,
	.stop = dma_sedi_stop,
	.reload = dma_sedi_reload,
	.get_status = NULL,
};

static int dma_sedi_init(const struct device *dev)
{
	const struct dma_sedi_config_info *const config = DEV_CFG(dev);

	config->irq_config();

	return 0;
}

#define DMA_DEVICE_INIT_SEDI(inst) \
	static void dma_sedi_##inst##_irq_config(void);			\
									\
	static struct dma_sedi_driver_data dma_sedi_dev_data_##inst; \
	static const struct dma_sedi_config_info dma_sedi_config_data_##inst = { \
		.peripheral_id = DT_INST_PROP(inst, peripheral_id), \
		.chn_num = DT_INST_PROP(inst, dma_channels), \
		.irq_config = dma_sedi_##inst##_irq_config \
	}; \
	DEVICE_DT_INST_DEFINE(inst, &dma_sedi_init, \
	      NULL, &dma_sedi_dev_data_##inst, &dma_sedi_config_data_##inst, PRE_KERNEL_2, \
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, (void *)&dma_funcs); \
									\
	static void dma_sedi_##inst##_irq_config(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority), dma_isr,	\
			    (void *)DT_INST_PROP(inst, peripheral_id),			\
			    DT_INST_IRQ(inst, sense));			\
		irq_enable(DT_INST_IRQN(inst));				\
	}

DT_INST_FOREACH_STATUS_OKAY(DMA_DEVICE_INIT_SEDI)

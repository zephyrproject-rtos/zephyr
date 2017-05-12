/*
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <dma.h>
#include <soc.h>
#include <fsl_edma.h>
#include <fsl_dmamux.h>
#include <fsl_clock.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DMA_LEVEL
#include <logging/sys_log.h>

#define NUMOF_DMA_CHANNELS	CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS

#if CONFIG_DMA_MCUX_TCD0_QUEUE_SIZE
#define TCD0_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD0_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd0_pool[TCD0_QUEUE_SIZE];
#define INSTALL_TCD0_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd0_pool,	\
						      TCD0_QUEUE_SIZE)
#else
#define INSTALL_TCD0_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD1_QUEUE_SIZE
#define TCD1_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD1_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd1_pool[TCD1_QUEUE_SIZE];
#define INSTALL_TCD1_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd1_pool,	\
						      TCD1_QUEUE_SIZE)
#else
#define INSTALL_TCD1_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD2_QUEUE_SIZE
#define TCD2_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD2_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd2_pool[TCD2_QUEUE_SIZE];
#define INSTALL_TCD2_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd2_pool,	\
						      TCD2_QUEUE_SIZE)
#else
#define INSTALL_TCD2_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD3_QUEUE_SIZE
#define TCD3_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD3_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd3_pool[TCD3_QUEUE_SIZE];
#define INSTALL_TCD3_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd3_pool,	\
						      TCD3_QUEUE_SIZE)
#else
#define INSTALL_TCD3_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD4_QUEUE_SIZE
#define TCD4_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD4_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd4_pool[TCD4_QUEUE_SIZE];
#define INSTALL_TCD4_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd4_pool,	\
						      TCD4_QUEUE_SIZE)
#else
#define INSTALL_TCD4_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD5_QUEUE_SIZE
#define TCD5_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD5_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd5_pool[TCD5_QUEUE_SIZE];
#define INSTALL_TCD5_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd5_pool,	\
						      TCD5_QUEUE_SIZE)
#else
#define INSTALL_TCD5_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD6_QUEUE_SIZE
#define TCD6_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD6_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd6_pool[TCD6_QUEUE_SIZE];
#define INSTALL_TCD6_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd6_pool,	\
						      TCD6_QUEUE_SIZE)
#else
#define INSTALL_TCD6_QUEUE(h)
#endif

#if CONFIG_DMA_MCUX_TCD7_QUEUE_SIZE
#define TCD7_QUEUE_SIZE	CONFIG_DMA_MCUX_TCD7_QUEUE_SIZE
edma_tcd_t __aligned(32) tcd7_pool[TCD7_QUEUE_SIZE];
#define INSTALL_TCD7_QUEUE(h)	EDMA_InstallTCDMemory(h, tcd7_pool,	\
						      TCD7_QUEUE_SIZE)
#else
#define INSTALL_TCD7_QUEUE(h)
#endif

#define DEFINE_MCUX_ISR_CH(__num)				\
	static void dma_mcux_isr_ch##__num(void *arg)		\
	{							\
		struct device *dev = (struct device *)arg;	\
		struct dma_mcux_data *data = dev->driver_data;	\
		EDMA_HandleIRQ(&data[__num].handle);		\
	}

struct dma_mcux_config {
	DMA_Type *base;
	void (*irq_config_func)(struct device *dev);
};


struct dma_mcux_data {
	edma_handle_t handle;
	void (*dma_callback)(struct device *dev, u32_t channel,
			     int error_code);
};

DEFINE_MCUX_ISR_CH(0)

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 2)
DEFINE_MCUX_ISR_CH(1)
#endif
#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 3)
DEFINE_MCUX_ISR_CH(2)
#endif
#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 4)
DEFINE_MCUX_ISR_CH(3)
#endif
#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 5)
DEFINE_MCUX_ISR_CH(4)
#endif
#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 6)
DEFINE_MCUX_ISR_CH(5)
#endif
#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 7)
DEFINE_MCUX_ISR_CH(6)
#endif
#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS == 8)
DEFINE_MCUX_ISR_CH(7)
#endif

static void edma_mcux_callback(edma_handle_t *handle, void *param,
			       bool done, u32_t tcds)
{
	struct device *dev = param;
	struct dma_mcux_data *data = dev->driver_data;
	u8_t ch = handle->channel;

	if (data[ch].dma_callback) {
		data[ch].dma_callback(dev, ch, 0);
	}
}

static void _install_tcd_memory(edma_handle_t *handle, u8_t channel)
{
	switch (channel) {
	case 0:
		INSTALL_TCD0_QUEUE(handle);
		break;
	case 1:
		INSTALL_TCD1_QUEUE(handle);
		break;
	case 2:
		INSTALL_TCD2_QUEUE(handle);
		break;
	case 3:
		INSTALL_TCD3_QUEUE(handle);
		break;
	case 4:
		INSTALL_TCD4_QUEUE(handle);
		break;
	case 5:
		INSTALL_TCD5_QUEUE(handle);
		break;
	case 6:
		INSTALL_TCD6_QUEUE(handle);
		break;
	case 7:
		INSTALL_TCD7_QUEUE(handle);
		break;
	default:
		break;
	}

}

static int dma_mcux_channel_config(struct device *dev, u32_t channel,
				   struct dma_config *dma_cfg)
{
	const struct dma_mcux_config *config = dev->config->config_info;
	struct dma_mcux_data *data = dev->driver_data;
	DMA_Type *base = config->base;
	struct dma_block_config *block_cfg = dma_cfg->head_block;
	edma_transfer_config_t edma_t_cfg;
	edma_transfer_type_t type;
	status_t status;
	u32_t bytes_each_req = dma_cfg->source_burst_length *
			       dma_cfg->source_data_size;

	if ((bytes_each_req == 0) || (bytes_each_req !=
	    (dma_cfg->dest_burst_length * dma_cfg->dest_data_size))) {
		SYS_LOG_ERR("wrong burst length configuration");
		return -EINVAL;
	}

	if (block_cfg->block_size < bytes_each_req) {
		SYS_LOG_ERR("block size is less than request size!");
		return -EINVAL;
	}

	if (block_cfg->block_size % bytes_each_req) {
		SYS_LOG_INF("block size is not aligned to burst length!");
	}

	if ((channel + 1) > CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS) {
		SYS_LOG_ERR("unsupported channel %d", channel);
		return -EINVAL;
	}

	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		type = kEDMA_MemoryToMemory;
		dma_cfg->dma_slot = (u8_t)(kDmaRequestMux0AlwaysOn63 & 0x3f);
		SYS_LOG_DBG("direction: MEMORY_TO_MEMORY");
	} else if (dma_cfg->channel_direction == MEMORY_TO_PERIPHERAL) {
		type = kEDMA_MemoryToPeripheral;
		SYS_LOG_DBG("direction: MEMORY_TO_PERIPHERAL");
	} else if (dma_cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
		type = kEDMA_PeripheralToMemory;
		SYS_LOG_DBG("direction: PERIPHERAL_TO_MEMORY");
	} else {
		return -EINVAL;
	}

	DMAMUX_SetSource(DMAMUX0, channel, dma_cfg->dma_slot);

	/* TODO: support for the period trigger ? */
	/*DMAMUX_EnablePeriodTrigger(DMAMUX0, channel);*/

	DMAMUX_EnableChannel(DMAMUX0, channel);

	EDMA_CreateHandle(&data[channel].handle, base, channel);
	EDMA_SetCallback(&data[channel].handle, edma_mcux_callback, dev);
	/* TODO: necessary ? */
	/*EDMA_ResetChannel(base, channel); ? */

	_install_tcd_memory(&data[channel].handle, channel);

	data[channel].dma_callback = dma_cfg->dma_callback;
	SYS_LOG_DBG("source_data_size %d", dma_cfg->source_data_size);
	SYS_LOG_DBG("dest_data_size %d", dma_cfg->dest_data_size);
	SYS_LOG_DBG("bytes each request %d", bytes_each_req);
	SYS_LOG_DBG("block size %d", block_cfg->block_size);

	EDMA_PrepareTransfer(&edma_t_cfg,
			     (void *)block_cfg->source_address,
			     (u32_t)dma_cfg->source_data_size,
			     (void *)block_cfg->dest_address,
			     (u32_t)dma_cfg->dest_data_size,
			     bytes_each_req,
			     (u32_t)block_cfg->block_size,
			     type);

	status = EDMA_SubmitTransfer(&data[channel].handle, &edma_t_cfg);

	if (status != kStatus_Success) {
		SYS_LOG_ERR("Transfer could not submit");
		return -EIO;
	}

	return 0;
}

static int dma_mcux_start(struct device *dev, u32_t channel)
{
	struct dma_mcux_data *data = dev->driver_data;

	if ((channel + 1) > CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS) {
		SYS_LOG_ERR("unsupported channel %d", channel);
		return -EINVAL;
	}

	EDMA_StartTransfer(&data[channel].handle);

	return 0;
}

static int dma_mcux_stop(struct device *dev, u32_t channel)
{
	struct dma_mcux_data *data = dev->driver_data;

	if ((channel + 1) > CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS) {
		SYS_LOG_ERR("unsupported channel %d", channel);
		return -EINVAL;
	}

	EDMA_StopTransfer(&data[channel].handle);

	return 0;
}

static const struct dma_driver_api dma_mcux_driver_api = {
	.config = dma_mcux_channel_config,
	.start = dma_mcux_start,
	.stop = dma_mcux_stop,
};

static int dma_mcux_init(struct device *dev)
{
	const struct dma_mcux_config *config = dev->config->config_info;
	DMA_Type *base = config->base;
	edma_config_t edma_config;

	SYS_LOG_DBG("");

	DMAMUX_Init(DMAMUX0);

	EDMA_GetDefaultConfig(&edma_config);

#ifdef CONFIG_DMA_MCUX_ROUND_ROBIN_ARBITRATION_MODE
	edma_config.enableRoundRobinArbitration = true;
#endif

#ifdef CONFIG_DMA_MCUX_ENABLE_DEBUG_MODE
	SYS_LOG_DBG("EDBG bit is set");
	edma_config.enableDebugMode = true;
#endif

	EDMA_Init(base, &edma_config);

	config->irq_config_func(dev);

	return 0;
}

static void dma_mcux_config_func_0(struct device *dev);

static const struct dma_mcux_config dma_mcux_config_0 = {
	.base = DMA0,
	.irq_config_func = dma_mcux_config_func_0,
};

static struct dma_mcux_data dma_mcux_data_0[NUMOF_DMA_CHANNELS];

DEVICE_AND_API_INIT(dma_mcux_0, CONFIG_DMA_0_NAME, &dma_mcux_init,
		    dma_mcux_data_0, &dma_mcux_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &dma_mcux_driver_api);

static void dma_mcux_config_func_0(struct device *dev)
{
	IRQ_CONNECT(IRQ_DMA_CHAN0, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch0, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN0);

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 2)
	IRQ_CONNECT(IRQ_DMA_CHAN1, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch1, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN1);
#endif

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 3)
	IRQ_CONNECT(IRQ_DMA_CHAN2, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch2, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN2);
#endif

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 4)
	IRQ_CONNECT(IRQ_DMA_CHAN3, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch3, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN3);
#endif

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 5)
	IRQ_CONNECT(IRQ_DMA_CHAN4, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch4, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN4);
#endif

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 6)
	IRQ_CONNECT(IRQ_DMA_CHAN5, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch5, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN5);
#endif

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS >= 7)
	IRQ_CONNECT(IRQ_DMA_CHAN6, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch6, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN6);
#endif

#if (CONFIG_DMA_MCUX_NUMOF_DMA_CHANNELS == 8)
	IRQ_CONNECT(IRQ_DMA_CHAN7, CONFIG_DMA_0_IRQ_PRI,
		    dma_mcux_isr_ch7, DEVICE_GET(dma_mcux_0), 0);
	irq_enable(IRQ_DMA_CHAN7);
#endif
}

/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_mcux_pxp.h>
#include <zephyr/devicetree.h>

#include <fsl_pxp.h>
#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#define DT_DRV_COMPAT nxp_pxp

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_mcux_pxp, CONFIG_DMA_LOG_LEVEL);

struct dma_mcux_pxp_config {
	PXP_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct dma_mcux_pxp_data {
	void *user_data;
	dma_callback_t dma_callback;
	uint32_t ps_buf_addr;
	uint32_t ps_buf_size;
	uint32_t out_buf_addr;
	uint32_t out_buf_size;
};

static void dma_mcux_pxp_irq_handler(const struct device *dev)
{
	const struct dma_mcux_pxp_config *config = dev->config;
	struct dma_mcux_pxp_data *data = dev->data;

	PXP_ClearStatusFlags(config->base, kPXP_CompleteFlag);
#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t) data->out_buf_addr, data->out_buf_size);
#endif
	if (data->dma_callback) {
		data->dma_callback(dev, data->user_data, 0, 0);
	}
}

/* Configure a channel */
static int dma_mcux_pxp_configure(const struct device *dev, uint32_t channel,
				   struct dma_config *config)
{
	const struct dma_mcux_pxp_config *dev_config = dev->config;
	struct dma_mcux_pxp_data *dev_data = dev->data;
	pxp_ps_buffer_config_t ps_buffer_cfg;
	pxp_output_buffer_config_t output_buffer_cfg;
	uint8_t bytes_per_pixel;
	pxp_rotate_degree_t rotate;

	ARG_UNUSED(channel);
	if (config->channel_direction != MEMORY_TO_MEMORY) {
		return -ENOTSUP;
	}
	/*
	 * Use the DMA slot value to get the pixel format and rotation
	 * settings
	 */
	switch ((config->dma_slot & DMA_MCUX_PXP_CMD_MASK) >> DMA_MCUX_PXP_CMD_SHIFT) {
	case DMA_MCUX_PXP_CMD_ROTATE_0:
		rotate = kPXP_Rotate0;
		break;
	case DMA_MCUX_PXP_CMD_ROTATE_90:
		rotate = kPXP_Rotate90;
		break;
	case DMA_MCUX_PXP_CMD_ROTATE_180:
		rotate = kPXP_Rotate180;
		break;
	case DMA_MCUX_PXP_CMD_ROTATE_270:
		rotate = kPXP_Rotate270;
		break;
	default:
		return -ENOTSUP;
	}
	switch ((config->dma_slot & DMA_MCUX_PXP_FMT_MASK) >> DMA_MCUX_PXP_FMT_SHIFT) {
	case DMA_MCUX_PXP_FMT_RGB565:
		ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatRGB565;
		output_buffer_cfg.pixelFormat = kPXP_OutputPixelFormatRGB565;
		bytes_per_pixel = 2;
		break;
	case DMA_MCUX_PXP_FMT_RGB888:
#if (!(defined(FSL_FEATURE_PXP_HAS_NO_EXTEND_PIXEL_FORMAT) && \
	FSL_FEATURE_PXP_HAS_NO_EXTEND_PIXEL_FORMAT)) && \
	(!(defined(FSL_FEATURE_PXP_V3) && FSL_FEATURE_PXP_V3))
		ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatARGB8888;
#else
		ps_buffer_cfg.pixelFormat = kPXP_PsPixelFormatRGB888;
#endif
		output_buffer_cfg.pixelFormat = kPXP_OutputPixelFormatRGB888;
		bytes_per_pixel = 3;
		break;
	default:
		return -ENOTSUP;
	}
	DCACHE_CleanByRange((uint32_t) config->head_block->source_address,
				config->head_block->block_size);

	/*
	 * Some notes on how specific fields of the DMA config are used by
	 * the PXP:
	 * head block source address: PS buffer source address
	 * head block destination address: Output buffer address
	 * head block block size: size of destination and source buffer
	 * source data size: width of source buffer in bytes (pitch)
	 * source burst length: height of source buffer in pixels
	 * dest data size: width of destination buffer in bytes (pitch)
	 * dest burst length: height of destination buffer in pixels
	 */
	ps_buffer_cfg.swapByte = false;
	ps_buffer_cfg.bufferAddr = config->head_block->source_address;
	ps_buffer_cfg.bufferAddrU = 0U;
	ps_buffer_cfg.bufferAddrV = 0U;
	ps_buffer_cfg.pitchBytes = config->source_data_size;
	PXP_SetProcessSurfaceBufferConfig(dev_config->base, &ps_buffer_cfg);

	output_buffer_cfg.interlacedMode = kPXP_OutputProgressive;
	output_buffer_cfg.buffer0Addr = config->head_block->dest_address;
	output_buffer_cfg.buffer1Addr = 0U;
	output_buffer_cfg.pitchBytes = config->dest_data_size;
	output_buffer_cfg.width = (config->dest_data_size  / bytes_per_pixel);
	output_buffer_cfg.height = config->dest_burst_length;
	PXP_SetOutputBufferConfig(dev_config->base, &output_buffer_cfg);
	/* We only support a process surface that covers the full buffer */
	PXP_SetProcessSurfacePosition(dev_config->base, 0U, 0U,
			output_buffer_cfg.width, output_buffer_cfg.height);
	/* Setup rotation */
	PXP_SetRotateConfig(dev_config->base, kPXP_RotateProcessSurface,
				rotate, kPXP_FlipDisable);

	dev_data->ps_buf_addr = config->head_block->source_address;
	dev_data->ps_buf_size = config->head_block->block_size;
	dev_data->out_buf_addr = config->head_block->dest_address;
	dev_data->out_buf_size = config->head_block->block_size;
	dev_data->dma_callback = config->dma_callback;
	dev_data->user_data = config->user_data;
	return 0;
}

static int dma_mcux_pxp_start(const struct device *dev, uint32_t channel)
{
	const struct dma_mcux_pxp_config *config = dev->config;
	struct dma_mcux_pxp_data *data = dev->data;
#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t) data->ps_buf_addr, data->ps_buf_size);
#endif

	ARG_UNUSED(channel);
	PXP_Start(config->base);
	return 0;
}

static const struct dma_driver_api dma_mcux_pxp_api = {
	.config = dma_mcux_pxp_configure,
	.start = dma_mcux_pxp_start,
};

static int dma_mcux_pxp_init(const struct device *dev)
{
	const struct dma_mcux_pxp_config *config = dev->config;

	PXP_Init(config->base);
	PXP_SetProcessSurfaceBackGroundColor(config->base, 0U);
	/* Disable alpha surface and CSC1 */
	PXP_SetAlphaSurfacePosition(config->base, 0xFFFFU, 0xFFFFU, 0U, 0U);
	PXP_EnableCsc1(config->base, false);
	PXP_EnableInterrupts(config->base, kPXP_CompleteInterruptEnable);
	config->irq_config_func(dev);
	return 0;
}

#define DMA_INIT(n)						       \
	static void dma_pxp_config_func##n(const struct device *dev)   \
	{							       \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (		       \
			   IRQ_CONNECT(DT_INST_IRQN(n),		       \
				       DT_INST_IRQ(n, priority),       \
				       dma_mcux_pxp_irq_handler,       \
				       DEVICE_DT_INST_GET(n), 0);      \
			   irq_enable(DT_INST_IRQ(n, irq));	       \
			   ))					       \
	}							       \
								       \
	static const struct dma_mcux_pxp_config dma_config_##n = {     \
		.base = (PXP_Type *)DT_INST_REG_ADDR(n),	       \
		.irq_config_func = dma_pxp_config_func##n,	       \
	};							       \
								       \
	static struct dma_mcux_pxp_data dma_data_##n;		       \
								       \
	DEVICE_DT_INST_DEFINE(n,				       \
			      &dma_mcux_pxp_init, NULL,		       \
			      &dma_data_##n, &dma_config_##n,	       \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,  \
			      &dma_mcux_pxp_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_INIT)

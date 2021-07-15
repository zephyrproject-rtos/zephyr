/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_pxp

#include <zephyr.h>

#include <drivers/pxp.h>

struct mcux_pxp_config {
	PXP_Type *base;
};

struct mcux_pxp_data {
	struct k_sem sem;
};

static int mcux_pxp_start(const struct device *dev)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_Start(base);
	return 0;
}

static int mcux_pxp_set_process_blocksize(const struct device *dev,
									pxp_block_size_t size)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessBlockSize(base, size);
	return 0;
}

static int mcux_pxp_set_as_buffer(const struct device *dev,
							const pxp_as_buffer_config_t *config)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetAlphaSurfaceBufferConfig(base, config);
	return 0;
}

static int mcux_pxp_set_as_blend(const struct device *dev,
								const pxp_as_blend_config_t *config)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetAlphaSurfaceBlendConfig(base, config);
	return 0;
}

static int mcux_pxp_set_as_overlay_color(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetAlphaSurfaceOverlayColorKey(base, colorLow, colorHigh);
	return 0;
}

static int mcux_pxp_enable_as_overlay_color(const struct device *dev,
											bool enable)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_EnableAlphaSurfaceOverlayColorKey(base, enable);
	return 0;
}


static int mcux_pxp_set_as_position(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetAlphaSurfacePosition(base, upperLeftX, upperLeftY,
								lowerRightX, lowerRightY);
	return 0;
}

static int mcux_pxp_set_ps_bg_color(const struct device *dev,
									uint32_t bgColor)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessSurfaceBackGroundColor(base, bgColor);
	return 0;
}

static int mcux_pxp_set_ps_buffer(const struct device *dev,
							const pxp_ps_buffer_config_t *config)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessSurfaceBufferConfig(base, config);
	return 0;
}

static int mcux_pxp_set_ps_scaler(const struct device *dev,
								uint16_t inputWidth,
								uint16_t inputHeight,
								uint16_t outputWidth,
								uint16_t outputHeight)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessSurfaceScaler(base, inputWidth, inputHeight,
								outputWidth, outputHeight);
	return 0;
}

static int mcux_pxp_set_ps_position(const struct device *dev,
									uint16_t upperLeftX,
									uint16_t upperLeftY,
									uint16_t lowerRightX,
									uint16_t lowerRightY)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessSurfacePosition(base, upperLeftX, upperLeftY,
									lowerRightX, lowerRightX);
	return 0;
}

static int mcux_pxp_set_ps_color(const struct device *dev,
										uint32_t colorLow,
										uint32_t colorHigh)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessSurfaceColorKey(base, colorLow, colorHigh);
	return 0;
}

static int mcux_pxp_set_ps_YUVFormat(const struct device *dev,
									pxp_ps_yuv_format_t format)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetProcessSurfaceYUVFormat(base, format);
	return 0;
}

static int mcux_pxp_set_output_buffer(const struct device *dev,
							const pxp_output_buffer_config_t *config)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetOutputBufferConfig(base, config);
	return 0;
}

static int mcux_pxp_set_overwritten_alpha_value(const struct device *dev,
										uint8_t alpha)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetOverwrittenAlphaValue(base, alpha);
	return 0;
}

static int mcux_pxp_enable_overwritten_alpha(const struct device *dev,
										bool enable)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_EnableOverWrittenAlpha(base, enable);
	return 0;
}

static int mcux_pxp_set_rotate(const struct device *dev,
							   pxp_rotate_position_t position,
							   pxp_rotate_degree_t degree,
							   pxp_flip_mode_t flipMode)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetRotateConfig(base, position, degree, flipMode);
	return 0;
}

static int mcux_pxp_set_csc1(const struct device *dev,
							pxp_csc1_mode_t mode)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_SetCsc1Mode(base, mode);
	return 0;
}

static int mcux_pxp_enable_csc1(const struct device *dev,
								bool enable)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_EnableCsc1(base, enable);
	return 0;
}

static int mcux_pxp_start_pic_copy(const struct device *dev,
							const pxp_pic_copy_config_t *config)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	return PXP_StartPictureCopy(base, config);
}

static int mcux_pxp_start_mem_copy(const struct device *dev,
									uint32_t srcAddr,
									uint32_t destAddr,
									uint32_t size)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	return PXP_StartMemCopy(base, srcAddr, destAddr, size);
}

static int mcux_pxp_wait_complete(const struct device *dev)
{
	struct mcux_pxp_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	return 0;
}

static int mcux_pxp_stop(const struct device *dev)
{
	struct mcux_pxp_config *pxp_config = (struct mcux_pxp_config *)dev->config;
	PXP_Type *base = pxp_config->base;

	PXP_Deinit(base);
	return 0;
}

static const struct pxp_driver_api mcux_pxp_driver_api = {
	.start = mcux_pxp_start,
	.set_process_blocksize = mcux_pxp_set_process_blocksize,
	.set_as_buffer = mcux_pxp_set_as_buffer,
	.set_as_blend = mcux_pxp_set_as_blend,
	.set_as_overlay_color = mcux_pxp_set_as_overlay_color,
	.enable_as_overlay_color = mcux_pxp_enable_as_overlay_color,
	.set_as_position = mcux_pxp_set_as_position,
	.set_ps_bg_color = mcux_pxp_set_ps_bg_color,
	.set_ps_buffer = mcux_pxp_set_ps_buffer,
	.set_ps_scaler = mcux_pxp_set_ps_scaler,
	.set_ps_position = mcux_pxp_set_ps_position,
	.set_ps_color = mcux_pxp_set_ps_color,
	.set_ps_YUVFormat = mcux_pxp_set_ps_YUVFormat,
	.set_output_buffer = mcux_pxp_set_output_buffer,
	.set_overwritten_alpha_value = mcux_pxp_set_overwritten_alpha_value,
	.enable_overwritten_alpha = mcux_pxp_enable_overwritten_alpha,
	.set_rotate = mcux_pxp_set_rotate,
	.set_csc1 = mcux_pxp_set_csc1,
	.enable_csc1 = mcux_pxp_enable_csc1,
	.start_pic_copy = mcux_pxp_start_pic_copy,
	.start_mem_copy = mcux_pxp_start_mem_copy,
	.wait_complete = mcux_pxp_wait_complete,
	.stop = mcux_pxp_stop,
};

static void mcux_pxp_isr(struct device *dev)
{
	const struct mcux_pxp_config *config = dev->config;
	struct mcux_pxp_data *data = dev->data;
	uint32_t status;

	status = PXP_GetStatusFlags(config->base);
	PXP_ClearStatusFlags(config->base, status);

	k_sem_give(&data->sem);

}

static int mcux_pxp_init(const struct device *dev)
{
	const struct mcux_pxp_config *config = dev->config;
	struct mcux_pxp_data *data = dev->data;

	k_sem_init(&data->sem, 0, 1);

	PXP_Init(config->base);

	PXP_EnableInterrupts(config->base, kPXP_CompleteInterruptEnable);

	return 0;
}

static const struct mcux_pxp_config mcux_pxp_config_0 = {
	.base = (PXP_Type *)DT_INST_REG_ADDR(0),
};

static struct mcux_pxp_data mcux_pxp_data_0;

static int mcux_pxp_init_0(const struct device *dev)
{

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    mcux_pxp_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

	return mcux_pxp_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &mcux_pxp_init_0,
		    device_pm_control_nop, &mcux_pxp_data_0,
		    &mcux_pxp_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_pxp_driver_api);

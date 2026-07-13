/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_dma2d

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/graphic.h>
#include <zephyr/drivers/reset.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stm32_dma2d, CONFIG_GRAPHIC_LOG_LEVEL);

typedef void (*irq_config_func_t)(const struct device *dev);

struct stm32_dma2d_data {
	struct graphic_context ctx;
	const struct device *dev;
	DMA2D_HandleTypeDef hdma2d;
	struct graphic_operation slots[CONFIG_GRAPHIC_STM32_DMA2D_POOL_SIZE];
};

struct stm32_dma2d_config {
	const struct stm32_pclken hclken[1];
	irq_config_func_t irq_config;
	const struct reset_dt_spec reset;
};

static int get_dma2d_output_fmt(uint32_t pixelformat, uint32_t *dma2d_output_fmt)
{
	switch (pixelformat) {
	case PIXEL_FORMAT_ARGB_8888:
		*dma2d_output_fmt = DMA2D_OUTPUT_ARGB8888;
		break;
	case PIXEL_FORMAT_RGB_888:
		*dma2d_output_fmt = DMA2D_OUTPUT_RGB888;
		break;
	case PIXEL_FORMAT_RGB_565:
		*dma2d_output_fmt = DMA2D_OUTPUT_RGB565;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int get_dma2d_input_fmt(uint32_t pixelformat, uint32_t *dma2d_input_fmt)
{
	switch (pixelformat) {
	case PIXEL_FORMAT_ARGB_8888:
		*dma2d_input_fmt = DMA2D_INPUT_ARGB8888;
		break;
	case PIXEL_FORMAT_RGB_888:
		*dma2d_input_fmt = DMA2D_INPUT_RGB888;
		break;
	case PIXEL_FORMAT_RGB_565:
		*dma2d_input_fmt = DMA2D_INPUT_RGB565;
		break;
	case PIXEL_FORMAT_L_8:
		*dma2d_input_fmt = DMA2D_INPUT_L8;
		break;
	case PIXEL_FORMAT_AL_88:
		*dma2d_input_fmt = DMA2D_INPUT_AL88;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void stm32_dma2d_transfer_complete(DMA2D_HandleTypeDef *hdma2d)
{
	struct stm32_dma2d_data *data = CONTAINER_OF(hdma2d, struct stm32_dma2d_data, hdma2d);

	HAL_DMA2D_DeInit(&data->hdma2d);

	graphic_driver_operation_done(data->dev, GRAPHIC_OP_COMPLETED);
}

static void stm32_dma2d_transfer_error(DMA2D_HandleTypeDef *hdma2d)
{
	struct stm32_dma2d_data *data = CONTAINER_OF(hdma2d, struct stm32_dma2d_data, hdma2d);

	HAL_DMA2D_DeInit(&data->hdma2d);

	graphic_driver_operation_done(data->dev, GRAPHIC_OP_ERROR);
}

static int stm32_dma2d_run_m2m_pfc(const struct device *dev, struct graphic_operation *op)
{
	struct stm32_dma2d_data *data = dev->data;
	uint32_t output_fmt, input_fmt;
	HAL_StatusTypeDef hal_ret;
	int ret;

	/* Check formats */
	ret = get_dma2d_input_fmt(op->u.input.pixelformat, &input_fmt);
	if (ret < 0) {
		goto out;
	}
	ret = get_dma2d_output_fmt(op->output.pixelformat, &output_fmt);
	if (ret < 0) {
		goto out;
	}

	/* Init & Configure layers */
	data->hdma2d.Init.Mode = DMA2D_M2M_PFC;
	data->hdma2d.Init.ColorMode = output_fmt;
	data->hdma2d.Init.OutputOffset = op->output.stride - op->u.input.width;

	hal_ret = HAL_DMA2D_Init(&data->hdma2d);
	if (hal_ret != HAL_OK) {
		ret = -EIO;
		goto out;
	}

	data->hdma2d.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset =
		op->u.input.stride - op->u.input.width;
	data->hdma2d.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = input_fmt;
	data->hdma2d.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaInverted = DMA2D_REGULAR_ALPHA;
	data->hdma2d.LayerCfg[DMA2D_FOREGROUND_LAYER].RedBlueSwap = DMA2D_RB_REGULAR;

	hal_ret = HAL_DMA2D_ConfigLayer(&data->hdma2d, DMA2D_FOREGROUND_LAYER);
	if (hal_ret != HAL_OK) {
		ret = -EIO;
		goto out_deinit;
	}

	data->hdma2d.XferCpltCallback = stm32_dma2d_transfer_complete;
	data->hdma2d.XferErrorCallback = stm32_dma2d_transfer_error;

	/* Start the transfer */
	hal_ret = HAL_DMA2D_Start_IT(&data->hdma2d, (uint32_t)op->u.input.addr,
				     (uint32_t)op->output.addr, op->u.input.width,
				     op->u.input.height);
	if (hal_ret != HAL_OK) {
		ret = -EIO;
		goto out_deinit;
	}

	return 0;

out_deinit:
	HAL_DMA2D_DeInit(&data->hdma2d);

out:
	return ret;
}

static int stm32_dma2d_run_fill(const struct device *dev, struct graphic_operation *op)
{
	struct stm32_dma2d_data *data = dev->data;
	uint32_t output_fmt;
	HAL_StatusTypeDef hal_ret;
	uint32_t fill_color;
	int ret;

	/* Check formats */
	ret = get_dma2d_output_fmt(op->output.pixelformat, &output_fmt);
	if (ret < 0) {
		goto out;
	}

	/* Init & Configure layers */
	data->hdma2d.Init.Mode = DMA2D_R2M;
	data->hdma2d.Init.ColorMode = output_fmt;
	data->hdma2d.Init.OutputOffset = op->output.stride - op->u.fill.width;

	hal_ret = HAL_DMA2D_Init(&data->hdma2d);
	if (hal_ret != HAL_OK) {
		ret = -EIO;
		goto out;
	}
	data->hdma2d.XferCpltCallback = stm32_dma2d_transfer_complete;
	data->hdma2d.XferErrorCallback = stm32_dma2d_transfer_error;

	fill_color = op->u.fill.red << 16 | op->u.fill.green << 8 | op->u.fill.blue;

	/* Start the transfer */
	hal_ret = HAL_DMA2D_Start_IT(&data->hdma2d, fill_color, (uint32_t)op->output.addr,
				     op->u.fill.width, op->u.fill.height);
	if (hal_ret != HAL_OK) {
		ret = -EIO;
		goto out_deinit;
	}

	return 0;

out_deinit:
	HAL_DMA2D_DeInit(&data->hdma2d);

out:
	return ret;
}

static int stm32_dma2d_process_operation(const struct device *dev, struct graphic_operation *op)
{
	int ret;

	switch (op->type) {
	case GRAPHIC_OP_MEMCPY_CONV:
		ret = stm32_dma2d_run_m2m_pfc(dev, op);
		break;
	case GRAPHIC_OP_FILL:
		ret = stm32_dma2d_run_fill(dev, op);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static DEVICE_API(graphic, stm32_dma2d_driver_api) = {
	.process_operation = stm32_dma2d_process_operation,
};

static int stm32_dma2d_enable_clock(const struct device *dev)
{
	const struct stm32_dma2d_config *config = dev->config;
	const struct device *cc_node = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	return clock_control_on(cc_node, (clock_control_subsys_t)&config->hclken);
}

static int stm32_dma2d_init(const struct device *dev)
{
	const struct stm32_dma2d_config *cfg = dev->config;
	struct stm32_dma2d_data *data = dev->data;
	int ret;

	data->dev = dev;

	ret = graphic_driver_init(dev, data->slots, CONFIG_GRAPHIC_STM32_DMA2D_POOL_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to init graphic driver");
		return ret;
	}

	ret = stm32_dma2d_enable_clock(dev);
	if (ret < 0) {
		LOG_ERR("Clock enabling failed.");
		return ret;
	}

	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}
	ret = reset_line_toggle_dt(&cfg->reset);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to reset the device.");
		return ret;
	}

	/* Run IRQ init */
	cfg->irq_config(dev);

	LOG_DBG("%s initialized", dev->name);

	return 0;
}

static void stm32_dma2d_isr(const struct device *dev)
{
	struct stm32_dma2d_data *dma2d = dev->data;

	HAL_DMA2D_IRQHandler(&dma2d->hdma2d);
}

#define STM32_DMA2D_INIT(inst)                                                                     \
	static void stm32_dma2d_irq_config_##inst(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), stm32_dma2d_isr,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static struct stm32_dma2d_data stm32_dma2d_data_##inst = {                                 \
		.hdma2d = {.Instance = (DMA2D_TypeDef *)DT_INST_REG_ADDR(inst)},                   \
	};                                                                                         \
												   \
	static const struct stm32_dma2d_config stm32_dma2d_config_##inst = {                       \
		.hclken = STM32_DT_INST_CLOCKS(inst),                                              \
		.irq_config = stm32_dma2d_irq_config_##inst,                                       \
		.reset = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),                                   \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(inst, &stm32_dma2d_init, NULL, &stm32_dma2d_data_##inst,             \
			      &stm32_dma2d_config_##inst, POST_KERNEL,                             \
			      CONFIG_GRAPHIC_INIT_PRIORITY, &stm32_dma2d_driver_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_DMA2D_INIT)

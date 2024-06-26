/*
 * Copyright (c) 2024 espros photonics Co.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_cam

#include <soc/gdma_channel.h>
#include <soc/lcd_cam_reg.h>
#include <soc/lcd_cam_struct.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_cam, LOG_LEVEL_INF);

#define VIDEO_ESP32_F160M               160000000UL
#define VIDEO_ESP32_DMA_BUFFER_MAX_SIZE 4095

enum video_esp32_cam_clk_sel_values {
	VIDEO_ESP32_CAM_CLK_SEL_NONE = 0,
	VIDEO_ESP32_CAM_CLK_SEL_XTAL = 1,
	VIDEO_ESP32_CAM_CLK_SEL_PLL_DIV2 = 2,
	VIDEO_ESP32_CAM_CLK_SEL_PLL_F160M = 3,
};

struct video_esp32_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *source_dev;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	const struct device *dma_dev;
	uint8_t rx_dma_channel;
	uint32_t clock_mclk;
	int irq_source;
	int irq_priority;
	uint8_t enable_16_bit;
	uint8_t invert_de;
	uint8_t invert_byte_order;
	uint8_t invert_bit_order;
	uint8_t invert_pclk;
	uint8_t invert_hsync;
	uint8_t invert_vsync;
};

struct video_esp32_data {
	const struct video_esp32_config *config;
	struct video_format video_format;
	struct video_buffer *active_vbuf;
	bool is_streaming;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct dma_block_config dma_blocks[CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM];
};

static int video_esp32_reload_dma(struct video_esp32_data *data);

static void video_esp32_irq_handler(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;
	lcd_cam_lc_dma_int_st_reg_t status = LCD_CAM.lc_dma_int_st;

	if (!status.cam_vsync_int_st) {
		LOG_WRN("Unexpected interrupt. status = %x", status.val);
		return;
	}

	LCD_CAM.lc_dma_int_clr.val = status.val;
	data->active_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);

	if (data->active_vbuf == NULL) {
		LOG_WRN("Frame dropped. No buffer available");
		return;
	}

	video_esp32_reload_dma(data);
}

void video_esp32_dma_rx_done(const struct device *dev, void *user_data, uint32_t channel,
			     int status)
{
	struct video_esp32_data *data = user_data;
	int ret = 0;

	if (status == DMA_STATUS_BLOCK) {
		LOG_DBG("received block");
		return;
	}

	if (status != DMA_STATUS_COMPLETE) {
		LOG_ERR("DMA error: %d", status);
		return;
	}

	if (data->active_vbuf == NULL) {
		LOG_ERR("No video buffer available. Enque some buffers first.");
		return;
	}

	k_fifo_put(&data->fifo_out, data->active_vbuf);
	data->active_vbuf = NULL;
}

static int video_esp32_reload_dma(struct video_esp32_data *data)
{
	const struct video_esp32_config *cfg = data->config;
	int ret = 0;

	if (data->active_vbuf == NULL) {
		LOG_ERR("No video buffer available. Enque some buffers first.");
		return -EAGAIN;
	}

	ret = dma_reload(cfg->dma_dev, cfg->rx_dma_channel, 0, (uint32_t)data->active_vbuf->buffer,
			 data->active_vbuf->bytesused);
	if (ret) {
		LOG_ERR("Unable to reload DMA (%d)", ret);
		return ret;
	}

	ret = dma_start(cfg->dma_dev, cfg->rx_dma_channel);
	if (ret) {
		LOG_ERR("Unable to start DMA (%d)", ret);
		return ret;
	}

	return 0;
}

static int video_esp32_stream_start(const struct device *dev)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;
	struct dma_status dma_status = {0};
	struct dma_config dma_cfg = {0};
	struct dma_block_config *dma_block_iter = data->dma_blocks;
	uint32_t buffer_size = 0;
	int error = 0;

	if (data->is_streaming) {
		return -EBUSY;
	}

	LOG_DBG("Start streaming");

	error = dma_get_status(cfg->dma_dev, cfg->rx_dma_channel, &dma_status);

	if (error) {
		LOG_ERR("Unable to get Rx status (%d)", error);
		return error;
	}

	if (dma_status.busy) {
		LOG_ERR("Rx DMA Channel %d is busy", cfg->rx_dma_channel);
		return -EBUSY;
	}

	data->active_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (!data->active_vbuf) {
		LOG_ERR("No enqueued video buffers available.");
		return -EAGAIN;
	}

	buffer_size = data->active_vbuf->bytesused;
	memset(data->dma_blocks, 0, sizeof(data->dma_blocks));
	for (int i = 0; i < CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM; ++i) {
		dma_block_iter->dest_address =
			(uint32_t)data->active_vbuf->buffer + i * VIDEO_ESP32_DMA_BUFFER_MAX_SIZE;
		if (buffer_size < VIDEO_ESP32_DMA_BUFFER_MAX_SIZE) {
			dma_block_iter->block_size = buffer_size;
			dma_block_iter->next_block = NULL;
			dma_cfg.block_count = i + 1;
			break;
		}
		dma_block_iter->block_size = VIDEO_ESP32_DMA_BUFFER_MAX_SIZE;
		dma_block_iter->next_block = dma_block_iter + 1;
		dma_block_iter++;
		buffer_size -= VIDEO_ESP32_DMA_BUFFER_MAX_SIZE;
	}

	if (dma_block_iter->next_block) {
		LOG_ERR("Not enough descriptors available. Increase DMA_ESP32_DESCRIPTOR_NUM");
		return -ENOBUFS;
	}

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback = video_esp32_dma_rx_done;
	dma_cfg.user_data = data;
	dma_cfg.dma_slot = SOC_GDMA_TRIG_PERIPH_CAM0;
	dma_cfg.complete_callback_en = 1;
	dma_cfg.head_block = &data->dma_blocks[0];

	error = dma_config(cfg->dma_dev, cfg->rx_dma_channel, &dma_cfg);
	if (error) {
		LOG_ERR("Unable to configure DMA (%d)", error);
		return error;
	}

	LCD_CAM.cam_ctrl1.cam_reset = 1;
	LCD_CAM.cam_ctrl1.cam_reset = 0;
	LCD_CAM.cam_ctrl1.cam_afifo_reset = 1;
	LCD_CAM.cam_ctrl1.cam_afifo_reset = 0;

	error = dma_start(cfg->dma_dev, cfg->rx_dma_channel);
	if (error) {
		LOG_ERR("Unable to start DMA (%d)", error);
		return error;
	}

	LCD_CAM.cam_ctrl.cam_update = 1;
	LCD_CAM.cam_ctrl.cam_update = 0;
	LCD_CAM.cam_ctrl1.cam_start = 1;

	data->is_streaming = true;

	return 0;
}

static int video_esp32_stream_stop(const struct device *dev)
{
	int ret = 0;
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;

	LOG_DBG("Stop streaming");

	data->is_streaming = false;
	dma_stop(cfg->dma_dev, cfg->rx_dma_channel);
	LCD_CAM.cam_ctrl1.cam_start = 0;
	return ret;
}

static int video_esp32_get_caps(const struct device *dev, enum video_endpoint_id ep,
				struct video_caps *caps)
{
	const struct video_esp32_config *config = dev->config;
	int ret = -ENODEV;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	/* Forward the message to the source device */
	ret = video_get_caps(config->source_dev, ep, caps);

	return ret;
}

static int video_esp32_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	const struct video_esp32_config *cfg = dev->config;
	int ret = 0;

	LOG_DBG("Get format");

	if (fmt == NULL || ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	ret = video_get_format(cfg->source_dev, ep, fmt);
	if (ret) {
		LOG_ERR("Failed to get format from source");
		return ret;
	}

	return ret;
}

static int video_esp32_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;

	if (fmt == NULL || ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	data->video_format = *fmt;

	return video_set_format(cfg->source_dev, ep, fmt);
}

static int video_esp32_enqueue(const struct device *dev, enum video_endpoint_id ep,
			       struct video_buffer *vbuf)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;
	int ret = 0;
	unsigned int key;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	vbuf->bytesused = data->video_format.pitch * data->video_format.height;

	k_fifo_put(&data->fifo_in, vbuf);

	return ret;
}

static int video_esp32_dequeue(const struct device *dev, enum video_endpoint_id ep,
			       struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct video_esp32_data *data = dev->data;
	int ret = 0;
	uint8_t key;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	LOG_DBG("Dequeue done, vbuf = %p", *vbuf);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_esp32_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_esp32_config *cfg = dev->config;

	return video_set_ctrl(cfg->source_dev, cid, value);
}

static int video_esp32_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_esp32_config *cfg = dev->config;

	return video_get_ctrl(cfg->source_dev, cid, value);
}

static void video_esp32_cam_ctrl_init(const struct device *dev)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;

	LCD_CAM.cam_ctrl.cam_stop_en = 0;
	LCD_CAM.cam_ctrl.cam_vsync_filter_thres = 4;
	LCD_CAM.cam_ctrl.cam_byte_order = cfg->invert_byte_order;
	LCD_CAM.cam_ctrl.cam_bit_order = cfg->invert_bit_order;
	LCD_CAM.cam_ctrl.cam_line_int_en = 0;
	LCD_CAM.cam_ctrl.cam_vs_eof_en = 1;

	LCD_CAM.cam_ctrl1.val = 0;
	LCD_CAM.cam_ctrl1.cam_rec_data_bytelen = 0;
	LCD_CAM.cam_ctrl1.cam_line_int_num = 0;
	LCD_CAM.cam_ctrl1.cam_clk_inv = cfg->invert_pclk;
	LCD_CAM.cam_ctrl1.cam_vsync_filter_en = 1;
	LCD_CAM.cam_ctrl1.cam_2byte_en = cfg->enable_16_bit;
	LCD_CAM.cam_ctrl1.cam_de_inv = cfg->invert_de;
	LCD_CAM.cam_ctrl1.cam_hsync_inv = cfg->invert_hsync;
	LCD_CAM.cam_ctrl1.cam_vsync_inv = cfg->invert_vsync;
	LCD_CAM.cam_ctrl1.cam_vh_de_mode_en = 0;

	LCD_CAM.cam_rgb_yuv.val = 0;
}

static int esp32_interrupt_init(const struct device *dev)
{
	const struct video_esp32_config *cfg = dev->config;
	int ret = 0;

	LCD_CAM.lc_dma_int_clr.cam_hs_int_clr = 1;
	LCD_CAM.lc_dma_int_clr.cam_vsync_int_clr = 1;
	LCD_CAM.lc_dma_int_ena.cam_vsync_int_ena = 1;
	LCD_CAM.lc_dma_int_ena.cam_hs_int_ena = 1;

	ret = esp_intr_alloc(cfg->irq_source, cfg->irq_priority,
			     (intr_handler_t)video_esp32_irq_handler, (void *)dev, NULL);

	return ret;
}

static int esp32_init(const struct device *dev)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;
	int ret = 0;

	data->config = cfg;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("video pinctrl setup failed (%d)", ret);
		return ret;
	}

	video_esp32_cam_ctrl_init(dev);

	ret = esp32_interrupt_init(dev);
	if (ret) {
		LOG_ERR("Failed to initialize interrupt");
		return ret;
	}

	if (!device_is_ready(cfg->dma_dev)) {
		return -ENODEV;
	}

	LOG_DBG("esp32 video driver loaded");

	return ret;
}

static const struct video_driver_api esp32_driver_api = {
	/* mandatory callbacks */
	.set_format = video_esp32_set_fmt,
	.get_format = video_esp32_get_fmt,
	.stream_start = video_esp32_stream_start,
	.stream_stop = video_esp32_stream_stop,
	.get_caps = video_esp32_get_caps,
	/* optional callbacks */
	.enqueue = video_esp32_enqueue,
	.dequeue = video_esp32_dequeue,
	.flush = NULL,
	.set_ctrl = video_esp32_set_ctrl,
	.get_ctrl = video_esp32_get_ctrl,
	.set_signal = NULL,
};

PINCTRL_DT_INST_DEFINE(0);

static const struct video_esp32_config esp32_config = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.source_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, source)),
	.dma_dev = ESP32_DT_INST_DMA_CTLR(0, rx),
	.rx_dma_channel = DT_INST_DMAS_CELL_BY_NAME(0, rx, channel),
	.irq_source = DT_INST_IRQN(0),
	.irq_priority = ESP_INTR_FLAG_LOWMED,
	.enable_16_bit = DT_INST_PROP(0, enable_16bit_mode),
	.invert_bit_order = DT_INST_PROP(0, invert_bit_order),
	.invert_byte_order = DT_INST_PROP(0, invert_byte_order),
	.invert_pclk = DT_INST_PROP(0, invert_pclk),
	.invert_de = DT_INST_PROP(0, invert_de),
	.invert_hsync = DT_INST_PROP(0, invert_hsync),
	.invert_vsync = DT_INST_PROP(0, invert_vsync)};

static struct video_esp32_data esp32_data = {0};

DEVICE_DT_INST_DEFINE(0, &esp32_init, PM_DEVICE_DT_INST_GET(idx), &esp32_data, &esp32_config,
		      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &esp32_driver_api);

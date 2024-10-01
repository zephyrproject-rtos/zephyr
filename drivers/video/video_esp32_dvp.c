/*
 * Copyright (c) 2024 espros photonics Co.
 * Copyright (c) 2024 Espressif Systems (Shanghai) CO LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lcd_cam

#include <soc/gdma_channel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/esp32_clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <hal/cam_hal.h>
#include <hal/cam_ll.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(video_esp32_lcd_cam, CONFIG_VIDEO_LOG_LEVEL);

#define VIDEO_ESP32_DMA_BUFFER_MAX_SIZE 4095

enum video_esp32_cam_clk_sel_values {
	VIDEO_ESP32_CAM_CLK_SEL_NONE = 0,
	VIDEO_ESP32_CAM_CLK_SEL_XTAL = 1,
	VIDEO_ESP32_CAM_CLK_SEL_PLL_DIV2 = 2,
	VIDEO_ESP32_CAM_CLK_SEL_PLL_F160M = 3,
};

struct video_esp32_config {
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	const struct device *dma_dev;
	const struct device *source_dev;
	uint32_t cam_clk;
	uint8_t rx_dma_channel;
	uint8_t data_width;
	uint8_t invert_de;
	uint8_t invert_byte_order;
	uint8_t invert_bit_order;
	uint8_t invert_pclk;
	uint8_t invert_hsync;
	uint8_t invert_vsync;
};

struct video_esp32_data {
	cam_hal_context_t hal;
	const struct video_esp32_config *config;
	struct video_format video_format;
	struct video_buffer *active_vbuf;
	bool is_streaming;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct dma_block_config dma_blocks[CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM];
};

static int video_esp32_reload_dma(struct video_esp32_data *data)
{
	const struct video_esp32_config *cfg = data->config;
	int ret = 0;

	if (data->active_vbuf == NULL) {
		LOG_ERR("No video buffer available. Enqueue some buffers first.");
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

void video_esp32_dma_rx_done(const struct device *dev, void *user_data, uint32_t channel,
			     int status)
{
	struct video_esp32_data *data = user_data;

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
	data->active_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);

	if (data->active_vbuf == NULL) {
		LOG_WRN("Frame dropped. No buffer available");
		return;
	}
	video_esp32_reload_dma(data);
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
			(uint32_t)data->active_vbuf->buffer + (i * VIDEO_ESP32_DMA_BUFFER_MAX_SIZE);
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
		LOG_ERR("Not enough descriptors available. Increase "
			"CONFIG_DMA_ESP32_MAX_DESCRIPTOR_NUM");
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

	error = dma_start(cfg->dma_dev, cfg->rx_dma_channel);
	if (error) {
		LOG_ERR("Unable to start DMA (%d)", error);
		return error;
	}

	cam_hal_start_streaming(&data->hal);

	if (video_stream_start(cfg->source_dev)) {
		return -EIO;
	}

	data->is_streaming = true;

	return 0;
}

static int video_esp32_stream_stop(const struct device *dev)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;
	int ret = 0;

	LOG_DBG("Stop streaming");

	if (video_stream_stop(cfg->source_dev)) {
		return -EIO;
	}

	data->is_streaming = false;
	ret = dma_stop(cfg->dma_dev, cfg->rx_dma_channel);
	if (ret) {
		LOG_ERR("Unable to stop DMA (%d)", ret);
		return ret;
	}

	cam_hal_stop_streaming(&data->hal);
	return 0;
}

static int video_esp32_get_caps(const struct device *dev, enum video_endpoint_id ep,
				struct video_caps *caps)
{
	const struct video_esp32_config *config = dev->config;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	/* Forward the message to the source device */
	return video_get_caps(config->source_dev, ep, caps);
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

	return 0;
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
	struct video_esp32_data *data = dev->data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	vbuf->bytesused = data->video_format.pitch * data->video_format.height;

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_esp32_dequeue(const struct device *dev, enum video_endpoint_id ep,
			       struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct video_esp32_data *data = dev->data;

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

	const cam_hal_config_t hal_cfg = {
		.port = 0,
		.byte_swap_en = cfg->invert_byte_order,
	};

	cam_hal_init(&data->hal, &hal_cfg);

	cam_ll_reverse_dma_data_bit_order(data->hal.hw, cfg->invert_bit_order);
	cam_ll_enable_invert_pclk(data->hal.hw, cfg->invert_pclk);
	cam_ll_set_input_data_width(data->hal.hw, cfg->data_width);
	cam_ll_enable_invert_de(data->hal.hw, cfg->invert_de);
	cam_ll_enable_invert_vsync(data->hal.hw, cfg->invert_vsync);
	cam_ll_enable_invert_hsync(data->hal.hw, cfg->invert_hsync);
}

static int video_esp32_init(const struct device *dev)
{
	const struct video_esp32_config *cfg = dev->config;
	struct video_esp32_data *data = dev->data;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	data->config = cfg;
	video_esp32_cam_ctrl_init(dev);

	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	return 0;
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
	.data_width = DT_INST_PROP_OR(0, data_width, 8),
	.invert_bit_order = DT_INST_PROP(0, invert_bit_order),
	.invert_byte_order = DT_INST_PROP(0, invert_byte_order),
	.invert_pclk = DT_INST_PROP(0, invert_pclk),
	.invert_de = DT_INST_PROP(0, invert_de),
	.invert_hsync = DT_INST_PROP(0, invert_hsync),
	.invert_vsync = DT_INST_PROP(0, invert_vsync),
	.cam_clk = DT_INST_PROP_OR(0, cam_clk, 0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
};

static struct video_esp32_data esp32_data = {0};

DEVICE_DT_INST_DEFINE(0, video_esp32_init, NULL, &esp32_data, &esp32_config,
		      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &esp32_driver_api);

static int video_esp32_cam_init_master_clock(void)
{
	int ret = 0;

	ret = pinctrl_apply_state(esp32_config.pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		printk("video pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Enable peripheral */
	if (!device_is_ready(esp32_config.clock_dev)) {
		return -ENODEV;
	}

	clock_control_on(esp32_config.clock_dev, esp32_config.clock_subsys);

	if (!esp32_config.cam_clk) {
		printk("No cam_clk specified\n");
		return -EINVAL;
	}

	if (ESP32_CLK_CPU_PLL_160M % esp32_config.cam_clk) {
		printk("Invalid cam_clk value. It must be a divisor of 160M\n");
		return -EINVAL;
	}

	/* Enable camera master clock output */
	cam_ll_select_clk_src(0, LCD_CLK_SRC_PLL160M);
	cam_ll_set_group_clock_coeff(0, ESP32_CLK_CPU_PLL_160M / esp32_config.cam_clk, 0, 0);

	return 0;
}

SYS_INIT(video_esp32_cam_init_master_clock, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

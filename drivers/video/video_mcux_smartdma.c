/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_video_smartdma

#include <fsl_smartdma.h>
#include <fsl_inputmux.h>

#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_mcux_smartdma.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/pinctrl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_video_sdma);

struct nxp_video_sdma_config {
	const struct device *dma_dev;
	const struct device *sensor_dev;
	const struct pinctrl_dev_config *pincfg;
	uint8_t vsync_pin;
	uint8_t hsync_pin;
	uint8_t pclk_pin;
};

/* Firmware reads 30 lines of data per video buffer */
#define SDMA_LINE_COUNT 30
/* Firmware only supports 320x240 */
#define SDMA_VBUF_HEIGHT 240
#define SDMA_VBUF_WIDTH 320

struct nxp_video_sdma_data {
	/* Must be aligned on 4 byte boundary, as lower 2 bits of ARM2SDMA register
	 * are used to enable interrupts
	 */
	smartdma_camera_param_t params __aligned(4);
	uint32_t smartdma_stack[64] __aligned(32);
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_sem stream_empty; /* Signals stream has run out of buffers */
	bool stream_starved;
	bool buf_reload_flag;
	struct video_buffer *active_buf;
	struct video_buffer *queued_buf;
	const struct nxp_video_sdma_config *config;
	uint32_t frame_idx;
};

/* Executed in interrupt context */
static void nxp_video_sdma_callback(const struct device *dev, void *user_data,
				uint32_t channel, int status)
{
	struct nxp_video_sdma_data *data = user_data;

	if (status < 0) {
		LOG_ERR("Transfer failed: %d, stopping DMA", status);
		dma_stop(data->config->dma_dev, 0);
		return;
	}
	/*
	 * SmartDMA engine streams 15 lines of RGB565 data, then interrupts the
	 * system. The engine will reload the framebuffer pointer after sending
	 * the first interrupt, and before sending the second interrupt.
	 *
	 * Based on this, we alternate between reloading the framebuffer
	 * pointer and queueing a completed frame every other interrupt
	 */
	if (data->buf_reload_flag) {
		/* Save old framebuffer, we will dequeue it next interrupt */
		data->active_buf = data->queued_buf;
		/* Load new framebuffer */
		data->queued_buf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
		if (data->queued_buf == NULL) {
			data->stream_starved = true;
		} else {
			data->params.p_buffer_ping_pong = (uint32_t *)data->queued_buf->buffer;
		}
	} else {
		if (data->stream_starved) {
			/* Signal any waiting threads */
			k_sem_give(&data->stream_empty);
		}
		data->active_buf->line_offset = (data->frame_idx / 2) * SDMA_LINE_COUNT;
		data->active_buf->timestamp = k_uptime_get_32();
		k_fifo_put(&data->fifo_out, data->active_buf);

	}
	/* Toggle buffer reload flag*/
	data->buf_reload_flag = !data->buf_reload_flag;
}

static int nxp_video_sdma_stream_start(const struct device *dev)
{
	const struct nxp_video_sdma_config *config = dev->config;
	struct nxp_video_sdma_data *data = dev->data;
	struct dma_config sdma_config = {0};
	int ret;

	/* Setup dma configuration for SmartDMA */
	sdma_config.dma_slot = kSMARTDMA_CameraDiv16FrameQVGA;
	sdma_config.dma_callback = nxp_video_sdma_callback;
	sdma_config.user_data = data;
	/* Setting bit 1 here enables the SmartDMA to interrupt ARM core
	 * when writing to SMARTDMA2ARM register
	 */
	sdma_config.head_block = (struct dma_block_config *)(((uint32_t)&data->params) | 0x2);

	/* Setup parameters for SmartDMA engine */
	data->params.smartdma_stack = data->smartdma_stack;
	/* SmartDMA continuously streams data once started. If user
	 * has not provided a framebuffer, we can't start DMA.
	 */
	data->queued_buf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (data->queued_buf == NULL) {
		return -EIO;
	}
	data->params.p_buffer_ping_pong = (uint32_t *)data->queued_buf->buffer;
	/* The firmware writes the index of the frame slice
	 * (from 0-15) into this buffer
	 */
	data->params.p_stripe_index = &data->frame_idx;

	/* Start DMA engine */
	ret = dma_config(config->dma_dev, 0, &sdma_config);
	if (ret < 0) {
		return ret;
	}
	/* Reset stream state variables */
	k_sem_reset(&data->stream_empty);
	data->buf_reload_flag = true;
	data->stream_starved = false;

	ret = dma_start(config->dma_dev, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int nxp_video_sdma_stream_stop(const struct device *dev)
{
	const struct nxp_video_sdma_config *config = dev->config;

	/* Stop DMA engine */
	return dma_stop(config->dma_dev, 0);
}

static int nxp_video_sdma_enqueue(const struct device *dev,
				  enum video_endpoint_id ep,
				  struct video_buffer *vbuf)
{
	struct nxp_video_sdma_data *data = dev->data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	/* SmartDMA will read 30 lines of RGB565 video data into framebuffer */
	vbuf->bytesused = SDMA_VBUF_WIDTH * SDMA_LINE_COUNT * sizeof(uint16_t);
	if (vbuf->size < vbuf->bytesused) {
		return -EINVAL;
	}

	/* Put buffer into FIFO */
	k_fifo_put(&data->fifo_in, vbuf);
	if (data->stream_starved) {
		/* Kick SmartDMA off */
		nxp_video_sdma_stream_start(dev);
	}
	return 0;
}

static int nxp_video_sdma_dequeue(const struct device *dev,
				  enum video_endpoint_id ep,
				  struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct nxp_video_sdma_data *data = dev->data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int nxp_video_sdma_flush(const struct device *dev,
				enum video_endpoint_id ep,
				bool cancel)
{
	const struct nxp_video_sdma_config *config = dev->config;
	struct nxp_video_sdma_data *data = dev->data;
	struct video_buf *vbuf;

	if (!cancel) {
		/* Wait for DMA to signal it is empty */
		k_sem_take(&data->stream_empty, K_FOREVER);
	} else {
		/* Stop DMA engine */
		dma_stop(config->dma_dev, 0);
		/* Forward all buffers in fifo_in to fifo_out */
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
			k_fifo_put(&data->fifo_out, vbuf);
		}
	}
	return 0;
}

/* SDMA only supports 320x240 RGB565 */
static const struct video_format_cap fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width_min = SDMA_VBUF_WIDTH,
		.width_max = SDMA_VBUF_WIDTH,
		.height_min = SDMA_VBUF_HEIGHT,
		.height_max = SDMA_VBUF_HEIGHT,
		.width_step = 0,
		.height_step = 0,
	},
	{ 0 },
};

static int nxp_video_sdma_set_format(const struct device *dev,
				     enum video_endpoint_id ep,
				     struct video_format *fmt)
{
	const struct nxp_video_sdma_config *config = dev->config;

	if (fmt == NULL || ep != VIDEO_EP_OUT)  {
		return -EINVAL;
	}

	if (!device_is_ready(config->sensor_dev)) {
		LOG_ERR("Sensor device not ready");
		return -ENODEV;
	}

	if ((fmt->pixelformat != fmts[0].pixelformat) ||
	    (fmt->width != fmts[0].width_min) ||
	    (fmt->height != fmts[0].height_min) ||
	    (fmt->pitch != fmts[0].width_min * 2)) {
		LOG_ERR("Unsupported format");
		return -ENOTSUP;
	}

	/* Forward format to sensor device */
	return video_set_format(config->sensor_dev, ep, fmt);
}

static int nxp_video_sdma_get_format(const struct device *dev,
				     enum video_endpoint_id ep,
				     struct video_format *fmt)
{
	const struct nxp_video_sdma_config *config = dev->config;
	int ret;

	if (fmt == NULL || ep != VIDEO_EP_OUT)  {
		return -EINVAL;
	}

	if (!device_is_ready(config->sensor_dev)) {
		LOG_ERR("Sensor device not ready");
		return -ENODEV;
	}

	/*
	 * Check sensor format. If it is not RGB565 320x240
	 * reconfigure the sensor,
	 * as this is the only format supported.
	 */
	ret = video_get_format(config->sensor_dev, VIDEO_EP_OUT, fmt);
	if (ret < 0) {
		return ret;
	}

	/* Verify that format is RGB565 */
	if ((fmt->pixelformat != fmts[0].pixelformat) ||
	    (fmt->width != fmts[0].width_min) ||
	    (fmt->height != fmts[0].height_min) ||
	    (fmt->pitch != fmts[0].width_min * 2)) {
		/* Update format of sensor */
		fmt->pixelformat = fmts[0].pixelformat;
		fmt->width = fmts[0].width_min;
		fmt->height = fmts[0].height_min;
		fmt->pitch = fmts[0].width_min * 2;
		ret = video_set_format(config->sensor_dev, VIDEO_EP_OUT, fmt);
		if (ret < 0) {
			LOG_ERR("Sensor device does not support RGB565");
			return ret;
		}
	}

	return 0;
}

static int nxp_video_sdma_get_caps(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_caps *caps)
{
	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	/* SmartDMA needs at least two buffers allocated before starting */
	caps->min_vbuf_count = 2;
	/* Firmware reads 30 lines per queued vbuf */
	caps->min_line_count = caps->max_line_count = SDMA_LINE_COUNT;
	caps->format_caps = fmts;
	return 0;
}

static int nxp_video_sdma_init(const struct device *dev)
{
	const struct nxp_video_sdma_config *config = dev->config;
	struct nxp_video_sdma_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->dma_dev)) {
		LOG_ERR("SmartDMA not ready");
		return -ENODEV;
	}

	INPUTMUX_Init(INPUTMUX0);
	/* Attach Camera VSYNC, HSYNC, and PCLK as inputs 0, 1, and 2 of the SmartDMA */
	INPUTMUX_AttachSignal(INPUTMUX0, 0,
			      config->vsync_pin + (SMARTDMAARCHB_INMUX0 << PMUX_SHIFT));
	INPUTMUX_AttachSignal(INPUTMUX0, 1,
			      config->hsync_pin + (SMARTDMAARCHB_INMUX0 << PMUX_SHIFT));
	INPUTMUX_AttachSignal(INPUTMUX0, 2,
			      config->pclk_pin + (SMARTDMAARCHB_INMUX0 << PMUX_SHIFT));
	/* Turnoff clock to inputmux to save power. Clock is only needed to make changes */
	INPUTMUX_Deinit(INPUTMUX0);

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	/* Given to when the DMA engine runs out of buffers */
	k_sem_init(&data->stream_empty, 0, 1);

	/* Install camera firmware used by SmartDMA */
	dma_smartdma_install_fw(config->dma_dev, (uint8_t *)s_smartdmaCameraFirmware,
				s_smartdmaCameraFirmwareSize);
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static const struct video_driver_api nxp_video_sdma_api = {
	.get_format = nxp_video_sdma_get_format,
	.set_format = nxp_video_sdma_set_format,
	.get_caps = nxp_video_sdma_get_caps,
	.stream_start = nxp_video_sdma_stream_start,
	.stream_stop = nxp_video_sdma_stream_stop,
	.enqueue = nxp_video_sdma_enqueue,
	.dequeue = nxp_video_sdma_dequeue,
	.flush = nxp_video_sdma_flush
};

#define NXP_VIDEO_SDMA_INIT(inst)						\
	PINCTRL_DT_INST_DEFINE(inst);						\
	const struct nxp_video_sdma_config sdma_config_##inst = {		\
		.dma_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.sensor_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, sensor)),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.vsync_pin = DT_INST_PROP(inst, vsync_pin),			\
		.hsync_pin = DT_INST_PROP(inst, hsync_pin),			\
		.pclk_pin = DT_INST_PROP(inst, pclk_pin),			\
	};									\
	struct nxp_video_sdma_data sdma_data_##inst = {				\
		.config = &sdma_config_##inst,					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, nxp_video_sdma_init, NULL,			\
				&sdma_data_##inst, &sdma_config_##inst,		\
				POST_KERNEL,					\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
				&nxp_video_sdma_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_VIDEO_SDMA_INIT)

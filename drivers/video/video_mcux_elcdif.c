/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fsl_imx6sx_lcdif

#include <zephyr.h>

#include <drivers/video.h>

#include <fsl_elcdif.h>
#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#ifdef CONFIG_VIDEO_DISPLAY
#include <display/video.h>
#endif

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mcux_elcdif);

struct video_mcux_elcdif_config {
	LCDIF_Type *base;
	char *lcd_label;
};

enum video_mcux_elcdif_status {
	ELCDIF_STOPPED,
	ELCDIF_STARTED,
};

struct video_mcux_elcdif_data {
	/* config as expected by fsl_elcdif hal */
	elcdif_rgb_mode_config_t elcdif_config;
	/* lcd video subdevice connected to elcdif */
	struct device *lcd_dev;
	/* video buffers TO display */
	struct k_fifo fifo_in;
	/* video buffers displayed, to be dequeued */
	struct k_fifo fifo_out;
	/* Current displayed video buffer */
	struct video_buffer *current_vbuf;
	/* next buffer to display */
	struct video_buffer *next_vbuf;
	/* configured Pixel format */
	uint32_t pixelformat;
	/* status */
	enum video_mcux_elcdif_status status;
	/* For generating frame done events */
	struct k_poll_signal *signal;
};

static int video_mcux_elcdif_set_fmt(struct device *dev,
				     enum video_endpoint_id ep,
				     struct video_format *fmt)
{
	const struct video_mcux_elcdif_config *config = dev->config_info;
	struct video_mcux_elcdif_data *data = dev->driver_data;

	if (ep != VIDEO_EP_IN) {
		return -EINVAL;
	}

	if (data->status == ELCDIF_STARTED) {
		return -EBUSY;
	}

	data->pixelformat = fmt->pixelformat;

	if (data->lcd_dev && video_set_format(data->lcd_dev, ep, fmt)) {
		return -ENOTSUP;
	}

	data->elcdif_config.panelWidth = fmt->width;
	data->elcdif_config.panelHeight = fmt->height;

	switch (fmt->pixelformat) {
	case VIDEO_PIX_FMT_RGB565:
		data->elcdif_config.pixelFormat = kELCDIF_PixelFormatRGB565;
		data->elcdif_config.dataBus = kELCDIF_DataBus16Bit;
		break;
	default:
		return -ENOTSUP;
	}

	ELCDIF_RgbModeInit(config->base, &data->elcdif_config);

	return 0;
}

static int video_mcux_elcdif_get_fmt(struct device *dev,
				     enum video_endpoint_id ep,
				     struct video_format *fmt)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;

	if (fmt == NULL || ep != VIDEO_EP_IN) {
		return -EINVAL;
	}

	if (data->lcd_dev && !video_get_format(data->lcd_dev, ep, fmt)) {
		if (data->pixelformat != fmt->pixelformat
		    || data->elcdif_config.panelHeight != fmt->height
		    || data->elcdif_config.panelWidth != fmt->width) {
			/* align elcdif with display/lcd format */
			return video_mcux_elcdif_set_fmt(dev, ep, fmt);
		}
	}

	fmt->pixelformat = data->pixelformat;
	fmt->height = data->elcdif_config.panelHeight;
	fmt->width = data->elcdif_config.panelWidth;
	fmt->pitch = 2 * data->elcdif_config.panelWidth;

	return 0;
}

static int video_mcux_elcdif_stream_start(struct device *dev)
{
	const struct video_mcux_elcdif_config *config = dev->config_info;
	struct video_mcux_elcdif_data *data = dev->driver_data;

	data->status = ELCDIF_STARTED;
	ELCDIF_RgbModeStart(config->base);
	ELCDIF_EnableInterrupts(config->base,
				kELCDIF_CurFrameDoneInterruptEnable);

	return 0;
}

static int video_mcux_elcdif_stream_stop(struct device *dev)
{
	const struct video_mcux_elcdif_config *config = dev->config_info;
	struct video_mcux_elcdif_data *data = dev->driver_data;

	if (data->lcd_dev && video_stream_stop(data->lcd_dev)) {
		return -EIO;
	}

	ELCDIF_RgbModeStop(config->base);
	ELCDIF_DisableInterrupts(config->base,
				 kELCDIF_CurFrameDoneInterruptEnable);
	data->status = ELCDIF_STOPPED;

	return 0;
}


static int video_mcux_elcdif_flush(struct device *dev,
				   enum video_endpoint_id ep,
				   bool cancel)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;
	struct video_buffer *vbuf;

	if (data->status == ELCDIF_STARTED) {
		return -EBUSY;
	}

	if (data->next_vbuf && data->next_vbuf != data->current_vbuf) {
		k_fifo_put(&data->fifo_out, data->next_vbuf);
	}
	data->next_vbuf = NULL;

	if (data->current_vbuf) {
		k_fifo_put(&data->fifo_out, data->current_vbuf);
	}
	data->current_vbuf = NULL;

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
		k_fifo_put(&data->fifo_out, data->current_vbuf);
	}

	return -ENOTSUP;
}

static int video_mcux_elcdif_enqueue(struct device *dev,
				     enum video_endpoint_id ep,
				     struct video_buffer *vbuf)
{
	const struct video_mcux_elcdif_config *config = dev->config_info;
	struct video_mcux_elcdif_data *data = dev->driver_data;

	if (ep != VIDEO_EP_IN) {
		return -EINVAL;
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_CleanByRange((uint32_t)vbuf->buffer, vbuf->bytesused);
#endif

	k_fifo_put(&data->fifo_in, vbuf);

	/* Interrupt could have been disabled */
	ELCDIF_EnableInterrupts(config->base,
				kELCDIF_CurFrameDoneInterruptEnable);

	return 0;
}

static int video_mcux_elcdif_dequeue(struct device *dev,
				     enum video_endpoint_id ep,
				     struct video_buffer **vbuf,
				     k_timeout_t timeout)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;

	if (ep != VIDEO_EP_IN) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static inline int video_mcux_elcdif_set_ctrl(struct device *dev,
					     unsigned int cid,
					     void *value)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;
	int ret = -ENOTSUP;

	/* Forward to sensor dev if any */
	if (data->lcd_dev) {
		ret = video_set_ctrl(data->lcd_dev, cid, value);
	}

	return ret;
}

static inline int video_mcux_elcdif_get_ctrl(struct device *dev,
					     unsigned int cid,
					     void *value)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;
	int ret = -ENOTSUP;

	/* Forward to display/lcd subdev if any */
	if (data->lcd_dev) {
		ret = video_get_ctrl(data->lcd_dev, cid, value);
	}

	return ret;
}

static int video_mcux_elcdif_get_caps(struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_caps *caps)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;
	int err = -ENODEV;

	if (ep != VIDEO_EP_IN) {
		return -EINVAL;
	}

	/* Forward to display/lcd subdev */
	if (data->lcd_dev) {
		err = video_get_caps(data->lcd_dev, ep, caps);
	}

	caps->min_vbuf_count = 0;

	return err;
}

static void video_mcux_elcdif_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct video_mcux_elcdif_config *config = dev->config_info;
	struct video_mcux_elcdif_data *data = dev->driver_data;
	struct video_buffer *next_vbuf;
	uint32_t status;

	status = ELCDIF_GetInterruptStatus(config->base);
	ELCDIF_ClearInterruptStatus(config->base, status);

	if (!(status & kELCDIF_CurFrameDone)) {
		return;
	}

	if (data->current_vbuf == NULL)
		goto next;

	if (data->current_vbuf != data->next_vbuf) {
		/* current buffer done, release it */
		k_fifo_put(&data->fifo_out, data->current_vbuf);
		if (IS_ENABLED(CONFIG_POLL) && data->signal) {
			k_poll_signal_raise(data->signal, VIDEO_BUF_DONE);
		}
	}

next:
	/* next buffer is transferred now (possibly the same) */
	data->current_vbuf = data->next_vbuf;

	/* Do we have any new buffer for next ? */
	next_vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (next_vbuf) {
		data->next_vbuf = next_vbuf;
		ELCDIF_SetNextBufferAddr(config->base,
					 (uint32_t)data->current_vbuf->buffer);
	}
}

static int video_mcux_elcdif_init(struct device *dev)
{
	const struct video_mcux_elcdif_config *config = dev->config_info;
	struct video_mcux_elcdif_data *data = dev->driver_data;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	ELCDIF_RgbModeGetDefaultConfig(&data->elcdif_config);

	/* Some manual tunning for now */
	data->elcdif_config.polarityFlags =
			 kELCDIF_DataEnableActiveHigh |
			 kELCDIF_VsyncActiveLow |
			 kELCDIF_HsyncActiveLow |
			 kELCDIF_DriveDataOnRisingClkEdge;
	data->elcdif_config.dataBus = kELCDIF_DataBus16Bit;
	data->elcdif_config.pixelFormat = kELCDIF_PixelFormatRGB565;

	/* check if there is any attached lcd device */
	if (config->lcd_label) {
		data->lcd_dev = device_get_binding(config->lcd_label);
		if (data->lcd_dev == NULL) {
			LOG_ERR("Unable to get lcd device %s\n",
				config->lcd_label);
			return -ENODEV;
		} else if (data->lcd_dev == dev) {
			LOG_ERR("No lcd device specified");
			data->lcd_dev = NULL;
			return -ENODEV;
		}
	}

	/* todo remove */
	ELCDIF_RgbModeInit(config->base, &data->elcdif_config);

#ifdef CONFIG_VIDEO_DISPLAY
	display_video_init(dev);
#endif

	return 0;
}

#ifdef CONFIG_POLL
static int video_mcux_elcdif_set_signal(struct device *dev,
				     enum video_endpoint_id ep,
				     struct k_poll_signal *signal)
{
	struct video_mcux_elcdif_data *data = dev->driver_data;

	if (data->signal && signal != NULL) {
		return -EALREADY;
	}

	data->signal = signal;

	return 0;
}
#endif

static const struct video_driver_api video_mcux_elcdif_driver_api = {
	.set_format = video_mcux_elcdif_set_fmt,
	.get_format = video_mcux_elcdif_get_fmt,
	.stream_start = video_mcux_elcdif_stream_start,
	.stream_stop = video_mcux_elcdif_stream_stop,
	.flush = video_mcux_elcdif_flush,
	.enqueue = video_mcux_elcdif_enqueue,
	.dequeue = video_mcux_elcdif_dequeue,
	.set_ctrl = video_mcux_elcdif_set_ctrl,
	.get_ctrl = video_mcux_elcdif_get_ctrl,
	.get_caps = video_mcux_elcdif_get_caps,
#ifdef CONFIG_POLL
	.set_signal = video_mcux_elcdif_set_signal,
#endif
#ifdef CONFIG_VIDEO_DISPLAY
	.display_drv_api.blanking_on = display_video_blanking_on,
	.display_drv_api.blanking_off = display_video_blanking_off,
	.display_drv_api.write = display_video_write,
	.display_drv_api.read = display_video_read,
	.display_drv_api.get_framebuffer = display_video_get_framebuffer,
	.display_drv_api.set_brightness = display_video_set_brightness,
	.display_drv_api.set_contrast = display_video_set_contrast,
	.display_drv_api.get_capabilities = display_video_get_capabilities,
	.display_drv_api.set_pixel_format = display_video_set_pixel_format,
	.display_drv_api.set_orientation = display_video_set_orientation,
#endif
};

/* unique instance */
static const struct video_mcux_elcdif_config video_mcux_elcdif_config_0 = {
	.base = (LCDIF_Type *)DT_INST_REG_ADDR(0),
#if DT_INST_NODE_HAS_PROP(0, lcd)
	.lcd_label = DT_LABEL(DT_INST_PHANDLE(0, lcd)),
#endif
};

static struct video_mcux_elcdif_data video_mcux_elcdif_data_0;
static int video_mcux_elcdif_init_0(struct device *dev);

DEVICE_AND_API_INIT(video_mcux_elcdif, DT_INST_LABEL(0),
		    &video_mcux_elcdif_init_0, &video_mcux_elcdif_data_0,
		    &video_mcux_elcdif_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &video_mcux_elcdif_driver_api);

static int video_mcux_elcdif_init_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    video_mcux_elcdif_isr, DEVICE_GET(video_mcux_elcdif), 0);

	irq_enable(DT_INST_IRQN(0));

	video_mcux_elcdif_init(dev);

	return 0;
}

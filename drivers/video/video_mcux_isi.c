/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_isi

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/video.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <fsl_isi.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(video_mcux_isi, CONFIG_VIDEO_LOG_LEVEL);

#define ISI_MAX_ACTIVE_BUF 2U

struct video_mcux_isi_config {
	ISI_Type *base;
	const struct device *source_dev;
	uint8_t input_port;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct video_mcux_isi_data {
	const struct device *dev;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;

	/* ISI configuration */
	isi_config_t isi_config;

	/* Format information */
	struct video_format fmt;
	isi_output_format_t isi_format;
	bool format_set;

	/* Active buffers being used by ISI hardware */
	uint32_t active_buffer[ISI_MAX_ACTIVE_BUF];
	struct video_buffer *active_vbuf[ISI_MAX_ACTIVE_BUF];
	volatile uint8_t buffer_index;
	uint8_t active_buf_cnt;

	/* Drop frame buffer */
	uint32_t drop_frame;

	/* Transfer state */
	volatile bool is_transfer_started;
};

/* Map for the fourcc pixelformat to ISI format */
struct isi_format_map {
	uint32_t pixelformat;
	isi_output_format_t isi_format;
};

static const struct isi_format_map isi_formats[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.isi_format = kISI_OutputRGB565,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_XRGB32,
		.isi_format = kISI_OutputXRGB8888,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_ABGR32,
		.isi_format = kISI_OutputARGB8888,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_YUYV,
		.isi_format = kISI_OutputYUV422_1P8P,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_XYUV32,
		.isi_format = kISI_OutputYUV444_1P8P,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_SRGGB8,
		.isi_format = kISI_OutputRaw8,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_SRGGB10,
		.isi_format = kISI_OutputRaw16P,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_SRGGB12,
		.isi_format = kISI_OutputRaw16P,
	},
	{
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.isi_format = kISI_OutputRaw8,
	},
};

#define ISI_VIDEO_FORMAT_CAP(width, height, format)			\
	{.pixelformat = (format),					\
	 .width_min = 1,						\
	 .width_max = (width),						\
	 .height_min = 1,						\
	 .height_max = (height),					\
	 .width_step = 1,						\
	 .height_step = 1}

/* Limit maximum width to 2K for CSC/resize, 8K for raw passthrough */
static const struct video_format_cap isi_fmts[] = {
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_RGB565),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_XRGB32),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_ABGR32),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_YUYV),
	ISI_VIDEO_FORMAT_CAP(2048, 8191, VIDEO_PIX_FMT_XYUV32),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_SRGGB8),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_SRGGB10),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_SRGGB12),
	ISI_VIDEO_FORMAT_CAP(8191, 8191, VIDEO_PIX_FMT_GREY),
	{0},
};

static int find_isi_format(uint32_t pixelformat)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(isi_formats); i++) {
		if (isi_formats[i].pixelformat == pixelformat) {
			return i;
		}
	}

	return -EPIPE;
}

static int isi_channel_config(const struct device *dev)
{
	struct video_mcux_isi_data *data = dev->data;
	const struct video_mcux_isi_config *config = dev->config;
	ISI_Type *base = config->base;

	if (!data->format_set) {
		LOG_ERR("Video format not configured");
		return -EPIPE;
	}

	/* Configure ISI with output format dimensions */
	data->isi_config.inputWidth = data->fmt.width;
	data->isi_config.inputHeight = data->fmt.height;
	data->isi_config.outputFormat = data->isi_format;
	data->isi_config.outputLinePitchBytes = data->fmt.width *
		video_bits_per_pixel(data->fmt.pixelformat) / 8;

	ISI_Init(base);
	ISI_SetConfig(base, &data->isi_config);

	/* Disable scaling - passthrough mode */
	ISI_SetScalerConfig(base, data->fmt.width, data->fmt.height,
			    data->fmt.width, data->fmt.height);

	/* Disable color space conversion */
	ISI_EnableColorSpaceConversion(base, false);

	LOG_DBG("ISI configured: %dx%d, pitch=%u",
		data->fmt.width, data->fmt.height,
		data->isi_config.outputLinePitchBytes);

	return 0;
}

static int video_mcux_isi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	int i;

	/* Find and validate format */
	i = find_isi_format(fmt->pixelformat);
	if (i < 0) {
		return -ENOTSUP;
	}

	/* Save format information */
	memcpy(&data->fmt, fmt, sizeof(*fmt));
	data->isi_format = isi_formats[i].isi_format;
	data->format_set = true;

	/* Update pitch and size */
	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / 8;
	fmt->size = fmt->pitch * fmt->height;

	LOG_INF("ISI format set: %c%c%c%c, %dx%d, pitch=%u, size=%u",
		(char)fmt->pixelformat,
		(char)(fmt->pixelformat >> 8),
		(char)(fmt->pixelformat >> 16),
		(char)(fmt->pixelformat >> 24),
		fmt->width, fmt->height, fmt->pitch, fmt->size);

	/* Configure source device with same format if available */
	if (config->source_dev) {
		if (video_set_format(config->source_dev, fmt)) {
			LOG_ERR("Failed to set source device format");
			data->format_set = false;
			return -EIO;
		}
	}

	return 0;
}

static int video_mcux_isi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_mcux_isi_data *data = dev->data;

	if (!data->format_set) {
		memset(fmt, 0, sizeof(*fmt));
		LOG_DBG("Video format not configured, returning empty format");
		return 0;
	}

	memcpy(fmt, &data->fmt, sizeof(*fmt));
	return 0;
}

static int video_mcux_isi_stream_start(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	ISI_Type *base = config->base;
	uint8_t i;
	uint32_t buffer_addr;
	struct video_buffer *vbuf;
	int ret;

	ret = isi_channel_config(dev);
	if (ret < 0) {
		return ret;
	}

	if (data->active_buf_cnt != ISI_MAX_ACTIVE_BUF) {
		LOG_ERR("ISI requires at least %d active frame buffers", ISI_MAX_ACTIVE_BUF);
		return -ENOMEM;
	}

	/* Set initial buffers */
	for (i = 0; i < ISI_MAX_ACTIVE_BUF; i++) {
		buffer_addr = data->active_buffer[i];
		ISI_SetOutputBufferAddr(base, i, buffer_addr, 0, 0);
		/* Remove buffers from input queue */
		vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	}

	data->buffer_index = 0;
	data->is_transfer_started = true;

	/* Enable interrupt and start ISI */
	ISI_ClearInterruptStatus(base, (uint32_t)kISI_FrameReceivedInterrupt);
	ISI_EnableInterrupts(base, (uint32_t)kISI_FrameReceivedInterrupt);
	ISI_Start(base);

	/* Start source device */
	if (config->source_dev) {
		ret = video_stream_start(config->source_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret) {
			LOG_ERR("ISI source device start stream failed");
			ISI_Stop(base);
			ISI_DisableInterrupts(base, (uint32_t)kISI_FrameReceivedInterrupt);
			data->is_transfer_started = false;
			return -EIO;
		}
	}

	LOG_INF("ISI stream started");
	return 0;
}

static int video_mcux_isi_stream_stop(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	ISI_Type *base = config->base;

	/* Stop source device first */
	if (config->source_dev) {
		if (video_stream_stop(config->source_dev, VIDEO_BUF_TYPE_OUTPUT)) {
			LOG_ERR("ISI source device stop stream failed");
			return -EIO;
		}
	}

	/* Stop ISI */
	ISI_Stop(base);
	ISI_DisableInterrupts(base, (uint32_t)kISI_FrameReceivedInterrupt);
	ISI_ClearInterruptStatus(base, (uint32_t)kISI_FrameReceivedInterrupt);

	data->is_transfer_started = false;
	data->active_buf_cnt = 0;

	LOG_INF("ISI stream stopped");
	return 0;
}

static int video_mcux_isi_set_stream(const struct device *dev, bool enable,
				     enum video_buf_type type)
{
	if (enable) {
		return video_mcux_isi_stream_start(dev);
	} else {
		return video_mcux_isi_stream_stop(dev);
	}
}

static int video_mcux_isi_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_mcux_isi_data *data = dev->data;
	const struct video_mcux_isi_config *config = dev->config;
	ISI_Type *base = config->base;
	uint32_t interrupts;

	if (!data->format_set) {
		LOG_ERR("Cannot enqueue buffer: format not set");
		return -EPIPE;
	}

	vbuf->bytesused = data->fmt.pitch * data->fmt.height;

	if (!data->is_transfer_started) {
		/* Before streaming starts, collect buffers */
		if (data->drop_frame == 0U) {
			/* First buffer is drop frame buffer */
			data->drop_frame = POINTER_TO_UINT(vbuf->buffer);
			LOG_DBG("Drop frame buffer set: 0x%08x", data->drop_frame);
		} else {
			if (data->active_buf_cnt < ISI_MAX_ACTIVE_BUF) {
				data->active_buffer[data->active_buf_cnt] =
					POINTER_TO_UINT(vbuf->buffer);
				data->active_vbuf[data->active_buf_cnt] = vbuf;
				LOG_DBG("Active buffer[%d] set: 0x%08x",
					data->active_buf_cnt,
					data->active_buffer[data->active_buf_cnt]);
				data->active_buf_cnt++;
			}
			k_fifo_put(&data->fifo_in, vbuf);
		}
	} else {
		/* During streaming, add to input queue */
		/* Disable interrupt to protect buffer management */
		interrupts = ISI_DisableInterrupts(base, (uint32_t)kISI_FrameReceivedInterrupt);

		k_fifo_put(&data->fifo_in, vbuf);

		if (interrupts & (uint32_t)kISI_FrameReceivedInterrupt) {
			ISI_EnableInterrupts(base, (uint32_t)kISI_FrameReceivedInterrupt);
		}
	}

	return 0;
}

static int video_mcux_isi_dequeue(const struct device *dev, struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct video_mcux_isi_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_mcux_isi_flush(const struct device *dev, bool cancel)
{
	struct video_mcux_isi_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		/* Wait for all buffers to be processed */
		do {
			k_sleep(K_MSEC(1));
		} while (!k_fifo_is_empty(&data->fifo_in));
	} else {
		/* Move all buffers from input to output queue */
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
			k_fifo_put(&data->fifo_out, vbuf);
		}
	}

	return 0;
}

static int video_mcux_isi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_mcux_isi_config *config = dev->config;

	caps->format_caps = isi_fmts;
	/* ISI requires at least 3 buffers (2 active + 1 drop frame) */
	caps->min_vbuf_count = 3;

	/* Also get capabilities from source device if available */
	if (config->source_dev) {
		struct video_caps source_caps;

		if (!video_get_caps(config->source_dev, &source_caps)) {
			/* Use higher minimum buffer count if source requires more */
			if (source_caps.min_vbuf_count > caps->min_vbuf_count) {
				caps->min_vbuf_count = source_caps.min_vbuf_count;
			}
		}
	}

	return 0;
}

static int video_mcux_isi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_mcux_isi_config *config = dev->config;

	/* Forward to source device */
	if (config->source_dev) {
		return video_set_frmival(config->source_dev, frmival);
	}

	return 0;
}

static int video_mcux_isi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_mcux_isi_config *config = dev->config;

	/* Forward to source device */
	if (config->source_dev) {
		return video_get_frmival(config->source_dev, frmival);
	}

	return 0;
}

static int video_mcux_isi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_mcux_isi_config *config = dev->config;

	/* Forward to source device */
	if (config->source_dev) {
		return video_enum_frmival(config->source_dev, fie);
	}

	return 0;
}

static DEVICE_API(video, video_mcux_isi_driver_api) = {
	.set_format = video_mcux_isi_set_fmt,
	.get_format = video_mcux_isi_get_fmt,
	.set_stream = video_mcux_isi_set_stream,
	.enqueue = video_mcux_isi_enqueue,
	.dequeue = video_mcux_isi_dequeue,
	.flush = video_mcux_isi_flush,
	.get_caps = video_mcux_isi_get_caps,
	.set_frmival = video_mcux_isi_set_frmival,
	.get_frmival = video_mcux_isi_get_frmival,
	.enum_frmival = video_mcux_isi_enum_frmival,
};

static void video_mcux_isi_isr(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;
	ISI_Type *base = config->base;
	uint32_t int_status;
	uint32_t buffer_addr;
	struct video_buffer *vbuf = NULL;

	int_status = ISI_GetInterruptStatus(base);
	ISI_ClearInterruptStatus(base, int_status);

	/* Check if frame received interrupt */
	if (!(int_status & (uint32_t)kISI_FrameReceivedInterrupt)) {
		return;
	}

	buffer_addr = data->active_buffer[data->buffer_index];

	/* If current finished frame is not drop frame, submit to output queue */
	if (buffer_addr != data->drop_frame) {
		vbuf = data->active_vbuf[data->buffer_index];
		vbuf->timestamp = k_uptime_get_32();
		vbuf->bytesused = data->isi_config.outputLinePitchBytes * data->fmt.height;
		k_fifo_put(&data->fifo_out, vbuf);
	}

	/* Get next buffer from input queue */
	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);

	/* If no available input buffer, use drop frame buffer */
	if (vbuf == NULL) {
		buffer_addr = data->drop_frame;
		LOG_WRN("No available input buffer, drop frame");
	} else {
		buffer_addr = POINTER_TO_UINT(vbuf->buffer);
		data->active_vbuf[data->buffer_index] = vbuf;
	}

	data->active_buffer[data->buffer_index] = buffer_addr;
	ISI_SetOutputBufferAddr(base, data->buffer_index, buffer_addr, 0, 0);

	/* Toggle buffer index */
	data->buffer_index ^= 1U;
}

static int video_mcux_isi_init(const struct device *dev)
{
	const struct video_mcux_isi_config *config = dev->config;
	struct video_mcux_isi_data *data = dev->data;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);

	data->dev = dev;
	data->buffer_index = 0;
	data->is_transfer_started = false;
	data->drop_frame = 0;
	data->active_buf_cnt = 0;
	data->format_set = false;

	/* Initialize format structure */
	memset(&data->fmt, 0, sizeof(data->fmt));

	/* Check if source device is ready */
	if (config->source_dev != NULL) {
		if (!device_is_ready(config->source_dev)) {
			LOG_ERR("%s: source device %s not ready",
				dev->name, config->source_dev->name);
			return -ENODEV;
		}
		LOG_INF("%s: source device %s ready",
			dev->name, config->source_dev->name);
	} else {
		LOG_WRN("%s: no source device configured", dev->name);
	}

	/* Initialize ISI default configuration */
	ISI_GetDefaultConfig(&data->isi_config);
	data->isi_config.isChannelBypassed = false;
	data->isi_config.isSourceMemory = false;
	data->isi_config.sourcePort = config->input_port;
	data->isi_config.mipiChannel = 0;

	/* Configure clock if available */
	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("%s: clock device not ready", dev->name);
			return -ENODEV;
		}

		if (clock_control_on(config->clock_dev, config->clock_subsys)) {
			LOG_ERR("%s: failed to enable clock", dev->name);
			return -EIO;
		}
		LOG_DBG("%s: clock enabled", dev->name);
	}

	LOG_INF("%s: initialized successfully", dev->name);
	return 0;
}

/* Device instantiation macro */
#define VIDEO_MCUX_ISI_DEVICE(n)							\
											\
	static const struct video_mcux_isi_config video_mcux_isi_config_##n = {		\
		.base = (ISI_Type *)DT_INST_REG_ADDR(n),				\
		.source_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, remote_endpoint),	\
			(DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(				\
				DT_INST_ENDPOINT_BY_ID(n, 0, 0)))), (NULL)),		\
		.input_port = DT_INST_PROP_OR(n, input_port, 0),			\
		.clock_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),		\
			(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n))), (NULL)),		\
		.clock_subsys = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clocks),		\
			((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name)),		\
			(NULL)),							\
	};										\
											\
	static struct video_mcux_isi_data video_mcux_isi_data_##n;			\
											\
	static int video_mcux_isi_init_##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    video_mcux_isi_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));						\
		return video_mcux_isi_init(dev);					\
	}										\
											\
	DEVICE_DT_INST_DEFINE(n, &video_mcux_isi_init_##n, NULL,			\
			      &video_mcux_isi_data_##n,					\
			      &video_mcux_isi_config_##n,				\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,			\
			      &video_mcux_isi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_MCUX_ISI_DEVICE)

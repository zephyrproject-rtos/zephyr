/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

#include <drivers/video.h>

#define VIDEO_PATTERN_COLOR_BAR	0
#define VIDEO_PATTERN_FPS	30

struct video_sw_generator_data {
	struct device *dev;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_delayed_work buf_work;
	int pattern;
	struct k_poll_signal *signal;
	bool ctrl_hflip;
	bool ctrl_vflip;
};

static int video_sw_generator_set_fmt(struct device *dev,
				      enum video_endpoint_id ep,
				      struct video_format *fmt)
{
	struct video_sw_generator_data *data = dev->driver_data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	data->fmt = *fmt;

	return 0;
}

static int video_sw_generator_get_fmt(struct device *dev,
				      enum video_endpoint_id ep,
				      struct video_format *fmt)
{
	struct video_sw_generator_data *data = dev->driver_data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	*fmt = data->fmt;

	return 0;
}

static int video_sw_generator_stream_start(struct device *dev)
{
	struct video_sw_generator_data *data = dev->driver_data;

	return k_delayed_work_submit(&data->buf_work, K_MSEC(33));
}

static int video_sw_generator_stream_stop(struct device *dev)
{
	struct video_sw_generator_data *data = dev->driver_data;

	k_delayed_work_cancel(&data->buf_work);

	return 0;
}

/* Black, Blue, Red, Purple, Green, Aqua, Yellow, White */
u16_t rgb565_colorbar_value[] = { 0x0000, 0x001F, 0xF800, 0xF81F,
				  0x07E0, 0x07FF, 0xFFE0, 0xFFFF };

static void __fill_buffer_colorbar(struct video_sw_generator_data *data,
				   struct video_buffer *vbuf)
{
	int bw = data->fmt.width / 8;
	int h, w, i = 0;

	for (h = 0; h < data->fmt.height; h++) {
		for (w = 0; w < data->fmt.width; w++) {
			int color_idx =  data->ctrl_vflip ? 7 - w / bw : w / bw;
			if (data->fmt.pixelformat == VIDEO_PIX_FMT_RGB565) {
				u16_t *pixel = (u16_t *)&vbuf->buffer[i];
				*pixel = rgb565_colorbar_value[color_idx];
				i += 2;
			}
		}
	}

	vbuf->timestamp = k_uptime_get_32();
	vbuf->bytesused = i;
}

static void __buffer_work(struct k_work *work)
{
	struct video_sw_generator_data *data;
	struct video_buffer *vbuf;

	data = CONTAINER_OF(work, struct video_sw_generator_data, buf_work);

	k_delayed_work_submit(&data->buf_work, 1000 / VIDEO_PATTERN_FPS);

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		return;
	}

	switch (data->pattern) {
	case VIDEO_PATTERN_COLOR_BAR:
		__fill_buffer_colorbar(data, vbuf);
		break;
	}

	k_fifo_put(&data->fifo_out, vbuf);

	/* Event */
	if (data->signal) {
		k_poll_signal_raise(data->signal, VIDEO_BUF_DONE);
	}

	k_yield();
}

static int video_sw_generator_enqueue(struct device *dev,
				      enum video_endpoint_id ep,
				      struct video_buffer *vbuf)
{
	struct video_sw_generator_data *data = dev->driver_data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_sw_generator_dequeue(struct device *dev,
				      enum video_endpoint_id ep,
				      struct video_buffer **vbuf, u32_t timeout)
{
	struct video_sw_generator_data *data = dev->driver_data;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_sw_generator_flush(struct device *dev,
				    enum video_endpoint_id ep,
				    bool cancel)
{
	struct video_sw_generator_data *data = dev->driver_data;
	struct video_buffer *vbuf;

	if (!cancel) {
		/* wait for all buffer to be processed */
		do {
			k_sleep(K_MSEC(1));
		} while (!k_fifo_is_empty(&data->fifo_in));
	} else {
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
			k_fifo_put(&data->fifo_out, vbuf);
			if (data->signal) {
				k_poll_signal_raise(data->signal,
						    VIDEO_BUF_ABORTED);
			}
		}
	}

	return 0;
}

static const struct video_format_cap fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width_min = 64,
		.width_max = 1920,
		.height_min = 64,
		.height_max = 1080,
		.width_step = 1,
		.height_step = 1,
	},
	{ 0 }
};

static int video_sw_generator_get_caps(struct device *dev,
				       enum video_endpoint_id ep,
				       struct video_caps *caps)
{
	caps->format_caps = fmts;
	caps->min_vbuf_count = 0;

	return 0;
}

static int video_sw_generator_set_signal(struct device *dev,
					 enum video_endpoint_id ep,
					 struct k_poll_signal *signal)
{
	struct video_sw_generator_data *data = dev->driver_data;

	if (data->signal && signal != NULL) {
		return -EALREADY;
	}

	data->signal = signal;

	return 0;
}

static inline int video_sw_generator_set_ctrl(struct device *dev,
					      unsigned int cid,
					      void *value)
{
	struct video_sw_generator_data *data = dev->driver_data;

	switch (cid) {
	case VIDEO_CID_VFLIP:
		data->ctrl_vflip = (bool)value;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct video_driver_api video_sw_generator_driver_api = {
	.set_format = video_sw_generator_set_fmt,
	.get_format = video_sw_generator_get_fmt,
	.stream_start = video_sw_generator_stream_start,
	.stream_stop = video_sw_generator_stream_stop,
	.flush = video_sw_generator_flush,
	.enqueue = video_sw_generator_enqueue,
	.dequeue = video_sw_generator_dequeue,
	.get_caps = video_sw_generator_get_caps,
	.set_ctrl = video_sw_generator_set_ctrl,
	.set_signal = video_sw_generator_set_signal,
};

static struct video_sw_generator_data video_sw_generator_data_0 = {
	.fmt.width = 320,
	.fmt.height = 160,
	.fmt.pitch = 320*2,
	.fmt.pixelformat = VIDEO_PIX_FMT_RGB565,
};

static int video_sw_generator_init(struct device *dev)
{
	struct video_sw_generator_data *data = dev->driver_data;

	data->dev = dev;
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_delayed_work_init(&data->buf_work, __buffer_work);

	return 0;
}

DEVICE_AND_API_INIT(video_sw_generator, "VIDEO_SW_GENERATOR",
		    &video_sw_generator_init, &video_sw_generator_data_0, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &video_sw_generator_driver_api);

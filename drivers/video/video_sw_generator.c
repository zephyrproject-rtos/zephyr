/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sw_generator

#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_sw_generator, CONFIG_VIDEO_LOG_LEVEL);

#define VIDEO_PATTERN_COLOR_BAR 0
#define DEFAULT_FRAME_RATE      30
/*
 * The pattern generator needs about 1.5 ms to fill out a 320x160 RGB565
 * buffer and 25 ms for a 720p XRGB32 buffer (tested on i.MX RT1064). So,
 * the max frame rate actually varies between 40 and 666 fps depending on
 * the buffer format. There is no way to determine this value for each
 * format. 60 fps is therefore chosen as a common value in practice.
 */
#define MAX_FRAME_RATE          60

struct sw_ctrls {
	struct video_ctrl hflip;
};

struct video_sw_generator_data {
	const struct device *dev;
	struct sw_ctrls ctrls;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_work_delayable buf_work;
	struct k_work_sync work_sync;
	int pattern;
	struct k_poll_signal *signal;
	uint32_t frame_rate;
};

static const struct video_format_cap fmts[] = {{
						       .pixelformat = VIDEO_PIX_FMT_RGB565,
						       .width_min = 64,
						       .width_max = 1920,
						       .height_min = 64,
						       .height_max = 1080,
						       .width_step = 1,
						       .height_step = 1,
					       }, {
						       .pixelformat = VIDEO_PIX_FMT_XRGB32,
						       .width_min = 64,
						       .width_max = 1920,
						       .height_min = 64,
						       .height_max = 1080,
						       .width_step = 1,
						       .height_step = 1,
					       },
					       {0}};

static int video_sw_generator_set_fmt(const struct device *dev, enum video_endpoint_id ep,
				      struct video_format *fmt)
{
	struct video_sw_generator_data *data = dev->data;
	int i = 0;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(fmts); ++i) {
		if (fmt->pixelformat == fmts[i].pixelformat && fmt->width >= fmts[i].width_min &&
		    fmt->width <= fmts[i].width_max && fmt->height >= fmts[i].height_min &&
		    fmt->height <= fmts[i].height_max) {
			break;
		}
	}

	if (i == ARRAY_SIZE(fmts)) {
		LOG_ERR("Unsupported pixel format or resolution");
		return -ENOTSUP;
	}

	data->fmt = *fmt;

	return 0;
}

static int video_sw_generator_get_fmt(const struct device *dev, enum video_endpoint_id ep,
				      struct video_format *fmt)
{
	struct video_sw_generator_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*fmt = data->fmt;

	return 0;
}

static int video_sw_generator_set_stream(const struct device *dev, bool enable)
{
	struct video_sw_generator_data *data = dev->data;

	if (enable) {
		k_work_schedule(&data->buf_work, K_MSEC(1000 / data->frame_rate));
	} else {
		k_work_cancel_delayable_sync(&data->buf_work, &data->work_sync);
	}

	return 0;
}

/* Black, Blue, Red, Purple, Green, Aqua, Yellow, White */
uint16_t rgb565_colorbar_value[] = {0x0000, 0x001F, 0xF800, 0xF81F, 0x07E0, 0x07FF, 0xFFE0, 0xFFFF};

uint32_t xrgb32_colorbar_value[] = {0xFF000000, 0xFF0000FF, 0xFFFF0000, 0xFFFF00FF,
				    0xFF00FF00, 0xFF00FFFF, 0xFFFFFF00, 0xFFFFFFFF};

static void __fill_buffer_colorbar(struct video_sw_generator_data *data, struct video_buffer *vbuf)
{
	int bw = data->fmt.width / 8;
	int h, w, i = 0;

	for (h = 0; h < data->fmt.height; h++) {
		for (w = 0; w < data->fmt.width; w++) {
			int color_idx = data->ctrls.hflip.val ? 7 - w / bw : w / bw;
			if (data->fmt.pixelformat == VIDEO_PIX_FMT_RGB565) {
				uint16_t *pixel = (uint16_t *)&vbuf->buffer[i];
				*pixel = rgb565_colorbar_value[color_idx];
				i += 2;
			} else if (data->fmt.pixelformat == VIDEO_PIX_FMT_XRGB32) {
				uint32_t *pixel = (uint32_t *)&vbuf->buffer[i];
				*pixel = xrgb32_colorbar_value[color_idx];
				i += 4;
			}
		}
	}

	vbuf->timestamp = k_uptime_get_32();
	vbuf->bytesused = i;
	vbuf->line_offset = 0;
}

static void __buffer_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct video_sw_generator_data *data;
	struct video_buffer *vbuf;

	data = CONTAINER_OF(dwork, struct video_sw_generator_data, buf_work);

	k_work_reschedule(&data->buf_work, K_MSEC(1000 / data->frame_rate));

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

	if (IS_ENABLED(CONFIG_POLL) && data->signal) {
		k_poll_signal_raise(data->signal, VIDEO_BUF_DONE);
	}

	k_yield();
}

static int video_sw_generator_enqueue(const struct device *dev, enum video_endpoint_id ep,
				      struct video_buffer *vbuf)
{
	struct video_sw_generator_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_sw_generator_dequeue(const struct device *dev, enum video_endpoint_id ep,
				      struct video_buffer **vbuf, k_timeout_t timeout)
{
	struct video_sw_generator_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_sw_generator_flush(const struct device *dev, enum video_endpoint_id ep,
				    bool cancel)
{
	struct video_sw_generator_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		/* wait for all buffer to be processed */
		do {
			k_sleep(K_MSEC(1));
		} while (!k_fifo_is_empty(&data->fifo_in));
	} else {
		while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT))) {
			k_fifo_put(&data->fifo_out, vbuf);
			if (IS_ENABLED(CONFIG_POLL) && data->signal) {
				k_poll_signal_raise(data->signal, VIDEO_BUF_ABORTED);
			}
		}
	}

	return 0;
}

static int video_sw_generator_get_caps(const struct device *dev, enum video_endpoint_id ep,
				       struct video_caps *caps)
{
	caps->format_caps = fmts;
	caps->min_vbuf_count = 0;

	/* SW generator produces full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	return 0;
}

#ifdef CONFIG_POLL
static int video_sw_generator_set_signal(const struct device *dev, enum video_endpoint_id ep,
					 struct k_poll_signal *signal)
{
	struct video_sw_generator_data *data = dev->data;

	if (data->signal && signal != NULL) {
		return -EALREADY;
	}

	data->signal = signal;

	return 0;
}
#endif

static int video_sw_generator_set_frmival(const struct device *dev, enum video_endpoint_id ep,
					  struct video_frmival *frmival)
{
	struct video_sw_generator_data *data = dev->data;

	if (frmival->denominator && frmival->numerator) {
		data->frame_rate = MIN(DIV_ROUND_CLOSEST(frmival->denominator, frmival->numerator),
				       MAX_FRAME_RATE);
	} else {
		return -EINVAL;
	}

	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;

	return 0;
}

static int video_sw_generator_get_frmival(const struct device *dev, enum video_endpoint_id ep,
					  struct video_frmival *frmival)
{
	struct video_sw_generator_data *data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;

	return 0;
}

static int video_sw_generator_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
					   struct video_frmival_enum *fie)
{
	int i = 0;

	if (ep != VIDEO_EP_OUT || fie->index) {
		return -EINVAL;
	}

	while (fmts[i].pixelformat && (fmts[i].pixelformat != fie->format->pixelformat)) {
		i++;
	}

	if ((i == ARRAY_SIZE(fmts)) || (fie->format->width > fmts[i].width_max) ||
	    (fie->format->width < fmts[i].width_min) ||
	    (fie->format->height > fmts[i].height_max) ||
	    (fie->format->height < fmts[i].height_min)) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_STEPWISE;
	fie->stepwise.min.numerator = 1;
	fie->stepwise.min.denominator = MAX_FRAME_RATE;
	fie->stepwise.max.numerator = UINT32_MAX;
	fie->stepwise.max.denominator = 1;
	/* The frame interval step size is the minimum resolution of K_MSEC(), which is 1ms */
	fie->stepwise.step.numerator = 1;
	fie->stepwise.step.denominator = 1000;

	return 0;
}

static DEVICE_API(video, video_sw_generator_driver_api) = {
	.set_format = video_sw_generator_set_fmt,
	.get_format = video_sw_generator_get_fmt,
	.set_stream = video_sw_generator_set_stream,
	.flush = video_sw_generator_flush,
	.enqueue = video_sw_generator_enqueue,
	.dequeue = video_sw_generator_dequeue,
	.get_caps = video_sw_generator_get_caps,
	.set_frmival = video_sw_generator_set_frmival,
	.get_frmival = video_sw_generator_get_frmival,
	.enum_frmival = video_sw_generator_enum_frmival,
#ifdef CONFIG_POLL
	.set_signal = video_sw_generator_set_signal,
#endif
};

static struct video_sw_generator_data video_sw_generator_data_0 = {
	.fmt.width = 320,
	.fmt.height = 160,
	.fmt.pitch = 320 * 2,
	.fmt.pixelformat = VIDEO_PIX_FMT_RGB565,
	.frame_rate = DEFAULT_FRAME_RATE,
};

static int video_sw_generator_init_controls(const struct device *dev)
{
	struct video_sw_generator_data *data = dev->data;

	return video_init_ctrl(&data->ctrls.hflip, dev, VIDEO_CID_HFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int video_sw_generator_init(const struct device *dev)
{
	struct video_sw_generator_data *data = dev->data;

	data->dev = dev;
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_work_init_delayable(&data->buf_work, __buffer_work);

	return video_sw_generator_init_controls(dev);
}

DEVICE_DEFINE(video_sw_generator, "VIDEO_SW_GENERATOR", &video_sw_generator_init, NULL,
	      &video_sw_generator_data_0, NULL, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,
	      &video_sw_generator_driver_api);

VIDEO_DEVICE_DEFINE(video_sw_generator, DEVICE_GET(video_sw_generator), NULL);

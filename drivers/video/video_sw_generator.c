/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2024, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sw_generator

#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/dsp/print_format.h>
#include <zephyr/dsp/types.h>
#include <zephyr/dsp/macros.h>

#include <video_common.h>

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
#define HUE(a)		        Q15f((double)(a) / 360.)
#define SMPTE_NUM               7

static const q15_t smpte_colorbar_hsv[3][SMPTE_NUM] = {
	/* white,  yellow,   cyan,     green,    magenta,  red,      blue */
	{HUE(0),   HUE(60),  HUE(180), HUE(120), HUE(300), HUE(0),   HUE(240)},
	{Q15f(0.), Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.)},
	{Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.), Q15f(1.)},
};

struct video_sw_generator_data {
	const struct device *dev;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_work_delayable buf_work;
	struct k_work_sync work_sync;
	int pattern;
	uint8_t colorbar_rgb565[SMPTE_NUM][2];
	uint8_t colorbar_xrgb32[SMPTE_NUM][4];
	uint8_t colorbar_yuyv[SMPTE_NUM][4];
	q7_t ctrl_hsv_q7[3];
	bool ctrl_hflip;
	struct k_poll_signal *signal;
	uint32_t frame_rate;
};

#define VIDEO_SW_GENERATOR_FORMAT_CAP(fourcc)                                                      \
	{                                                                                          \
		.pixelformat = (fourcc), .width_min = 64, .width_max = 1920, .height_min = 1,      \
		.height_max = 1080, .width_step = 1, .height_step = 1,                             \
	}

static const struct video_format_cap fmts[] = {
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_XRGB32),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	{0},
};

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

static int video_sw_generator_stream_start(const struct device *dev)
{
	struct video_sw_generator_data *data = dev->data;

	k_work_schedule(&data->buf_work, K_MSEC(1000 / data->frame_rate));

	return 0;
}

static int video_sw_generator_stream_stop(const struct device *dev)
{
	struct video_sw_generator_data *data = dev->data;

	k_work_cancel_delayable_sync(&data->buf_work, &data->work_sync);

	return 0;
}

static void hsv_to_rgb(q15_t h, q15_t s, q15_t v, q15_t *r, q15_t *g, q15_t *b)
{
	q15_t chroma = MULq15(s, v);
	q15_t x;

	if (h < Q15f(1. / 6.)) {
		x = SUBq15(h, Q15f(0. / 6.)) * 6;
		*r = chroma;
		*g = MULq15(chroma, x);
		*b = Q15f(0.);
	} else if (h < Q15f(2. / 6.)) {
		x = SUBq15(h, Q15f(1. / 6.)) * 6;
		*r = MULq15(chroma, SUBq15(Q15f(1.), x));
		*g = chroma;
		*b = Q15f(0.);
	} else if (h < Q15f(3. / 6.)) {
		x = SUBq15(h, Q15f(2. / 6.)) * 6;
		*r = Q15f(0.);
		*g = chroma;
		*b = MULq15(chroma, x);
	} else if (h < Q15f(4. / 6.)) {
		x = SUBq15(h, Q15f(3. / 6.)) * 6;
		*r = Q15f(0.);
		*g = MULq15(chroma, SUBq15(Q15f(1), x));
		*b = chroma;
	} else if (h < Q15f(5. / 6.)) {
		x = SUBq15(h, Q15f(4. / 6.)) * 6;
		*r = MULq15(chroma, x);
		*g = Q15f(0.);
		*b = chroma;
	} else {
		x = SUBq15(h, Q15f(5. / 6.)) * 6;
		*r = chroma;
		*g = Q15f(0.);
		*b = MULq15(chroma, SUBq15(Q15f(1), x));
	}

	*r = ADDq15(SUBq15(v, chroma), *r);
	*g = ADDq15(SUBq15(v, chroma), *g);
	*b = ADDq15(SUBq15(v, chroma), *b);
}

#define BT709_WR   0.2126
#define BT709_WG   0.7152
#define BT709_WB   0.0722
#define BT709_UMAX 0.436
#define BT709_VMAX 0.615

static void rgb_to_yuv(q15_t r, q15_t g, q15_t b, q15_t *y, q15_t *u, q15_t *v)
{
	q15_t ux = Q15f(BT709_UMAX / (1. - BT709_WB));
	q15_t vx = Q15f(BT709_VMAX / (1. - BT709_WR));

	/* Using BT.709 coefficients */
	*y = 0;
	*y = ADDq15(*y, MULq15(Q15f(BT709_WR), r));
	*y = ADDq15(*y, MULq15(Q15f(BT709_WG), g));
	*y = ADDq15(*y, MULq15(Q15f(BT709_WB), b));
	*u = MULq15(ux, SUBq15(b, *y));
	*v = MULq15(vx, SUBq15(r, *y));
}

static void hsv_adjust(q15_t *hue, q15_t *sat, q15_t *val, q15_t h, q15_t s, q15_t v)
{
	*hue = MODq15((q31_t)MAXq15 + (q31_t)*hue + (q31_t)h, MAXq15);
	*sat = CLAMP((q31_t)*sat + (q31_t)s, 0, MAXq15);
	*val = CLAMP((q31_t)*val + (q31_t)v, 0, MAXq15);
}

static void init_colors(const struct device *dev)
{
	struct video_sw_generator_data *data = dev->data;

	for (int i = 0; i < SMPTE_NUM; i++) {
		q15_t r, g, b;
		q15_t y, u, v;
		q15_t hue = smpte_colorbar_hsv[0][i];
		q15_t sat = smpte_colorbar_hsv[1][i];
		q15_t val = smpte_colorbar_hsv[2][i];
		uint16_t u16;

		hsv_adjust(&hue, &sat, &val, Q15q7(data->ctrl_hsv_q7[0]),
			   Q15q7(data->ctrl_hsv_q7[1]), Q15q7(data->ctrl_hsv_q7[2]));
		hsv_to_rgb(hue, sat, val, &r, &g, &b);
		rgb_to_yuv(r, g, b, &y, &u, &v);

		LOG_DBG("H%1"PRIq(3)" S%1"PRIq(3)" V%1"PRIq(3)", "
			"R%1"PRIq(3)" G%1"PRIq(3)" B%1"PRIq(3)", "
			"Y%1"PRIq(3)" U%1"PRIq(3)" V%1"PRIq(3),
			PRIq_arg(hue, 3, 0), PRIq_arg(sat, 3, 0), PRIq_arg(val, 3, 0),
			PRIq_arg(r, 3, 0), PRIq_arg(g, 3, 0), PRIq_arg(b, 3, 0),
			PRIq_arg(y, 3, 0), PRIq_arg(u, 3, 0), PRIq_arg(v, 3, 0));

		u16 = BITSq15(r, 5) | (BITSq15(g, 6) << 5) | (BITSq15(b, 5) << 11);
		data->colorbar_rgb565[i][0] = u16 >> 0;
		data->colorbar_rgb565[i][1] = u16 >> 8;

		data->colorbar_xrgb32[i][0] = 0x00;
		data->colorbar_xrgb32[i][1] = BITSq15(r, 8);
		data->colorbar_xrgb32[i][2] = BITSq15(g, 8);
		data->colorbar_xrgb32[i][3] = BITSq15(g, 8);

		data->colorbar_yuyv[i][0] = BITSq15(y, 8);
		data->colorbar_yuyv[i][1] = BITSq15(ADDq15(Q15f(0.5), u), 8);
		data->colorbar_yuyv[i][2] = BITSq15(y, 8);
		data->colorbar_yuyv[i][3] = BITSq15(ADDq15(Q15f(0.5), v), 8);
	}
}

static void __fill_buffer_colorbar(struct video_sw_generator_data *data, struct video_buffer *vbuf)
{
	int h, w, i = 0;

	for (h = 0; h < data->fmt.height; h++) {
		for (w = 0; w < data->fmt.width;) {
			int color_idx = w * SMPTE_NUM / data->fmt.width;

			if (data->ctrl_hflip) {
				color_idx = SMPTE_NUM - 1 - color_idx;
			}

			switch (data->fmt.pixelformat) {
			case VIDEO_PIX_FMT_RGB565:
				memcpy(&vbuf->buffer[i], data->colorbar_rgb565[color_idx], 2);
				i += 2;
				w += 1;
				break;
			case VIDEO_PIX_FMT_XRGB32:
				memcpy(&vbuf->buffer[i], data->colorbar_xrgb32[color_idx], 4);
				i += 4;
				w += 1;
				break;
			case VIDEO_PIX_FMT_YUYV:
				memcpy(&vbuf->buffer[i], data->colorbar_yuyv[color_idx], 4);
				i += 4;
				w += 2;
				break;
			default:
				__ASSERT_NO_MSG(false);
			}
		}
	}

	vbuf->timestamp = k_uptime_get_32();
	vbuf->bytesused = i;
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

static inline int video_sw_generator_set_ctrl(const struct device *dev, unsigned int cid,
					      void *value)
{
	struct video_sw_generator_data *data = dev->data;
	uint32_t u32 = (uint32_t)value;
	int ret;

	ret = video_check_range_u32(dev, cid, u32);
	if (ret < 0) {
		LOG_ERR("value %u not in range", u32);
		return ret;
	}

	switch (cid) {
	case VIDEO_CID_HFLIP:
		data->ctrl_hflip = (bool)value;
		break;
	case VIDEO_CID_HUE:
		data->ctrl_hsv_q7[0] = u32 - MINq7;
		init_colors(dev);
		break;
	case VIDEO_CID_SATURATION:
		data->ctrl_hsv_q7[1] = u32 - MINq7;
		init_colors(dev);
		break;
	case VIDEO_CID_BRIGHTNESS:
		data->ctrl_hsv_q7[2] = u32 - MINq7;
		init_colors(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

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
	.set_frmival = video_sw_generator_set_frmival,
	.get_frmival = video_sw_generator_get_frmival,
	.enum_frmival = video_sw_generator_enum_frmival,
#ifdef CONFIG_POLL
	.set_signal = video_sw_generator_set_signal,
#endif
};

static int video_sw_generator_init(const struct device *dev)
{
	struct video_sw_generator_data *data = dev->data;

	init_colors(dev);

	data->dev = dev;
	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_work_init_delayable(&data->buf_work, __buffer_work);

	return 0;
}

#define VIDEO_SW_GENERATOR_DEFINE(inst)                                                            \
	static struct video_sw_generator_data video_sw_generator_data_##inst = {                   \
		.fmt.width = 320,                                                                  \
		.fmt.height = 160,                                                                 \
		.fmt.pitch = 320 * 2,                                                              \
		.fmt.pixelformat = VIDEO_PIX_FMT_RGB565,                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &video_sw_generator_init, NULL,                                \
			      &video_sw_generator_data_##inst, NULL, POST_KERNEL,                  \
			      CONFIG_VIDEO_INIT_PRIORITY, &video_sw_generator_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_SW_GENERATOR_DEFINE)

/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_video_sw_generator

#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

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
#define MIN_FRAME_RATE          1

struct sw_ctrls {
	struct video_ctrl hflip;
};

struct video_sw_generator_data {
	const struct device *dev;
	struct sw_ctrls ctrls;
	struct video_format fmt;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_work_delayable work;
	int pattern;
	struct k_poll_signal *sig;
	uint32_t frame_rate;
};

#define VIDEO_SW_GENERATOR_FORMAT_CAP(pixfmt)                                                      \
	{                                                                                          \
		.pixelformat = pixfmt,                                                             \
		.width_min = 64,                                                                   \
		.width_max = 1920,                                                                 \
		.height_min = 64,                                                                  \
		.height_max = 1080,                                                                \
		.width_step = 1,                                                                   \
		.height_step = 1,                                                                  \
	}

static const struct video_format_cap fmts[] = {
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_RGB24),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_YUYV),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_RGB565),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_XRGB32),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_SRGGB8),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_SGRBG8),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_SBGGR8),
	VIDEO_SW_GENERATOR_FORMAT_CAP(VIDEO_PIX_FMT_SGBRG8),
	{0},
};

static int video_sw_generator_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_sw_generator_data *data = dev->data;
	size_t idx;
	int ret;

	ret = video_format_caps_index(fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return ret;
	}

	fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;

	data->fmt = *fmt;
	return 0;
}

static int video_sw_generator_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_sw_generator_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

static int video_sw_generator_set_stream(const struct device *dev, bool enable,
					 enum video_buf_type type)
{
	struct video_sw_generator_data *data = dev->data;
	struct k_work_sync work_sync = {0};

	if (enable) {
		k_work_schedule(&data->work, K_MSEC(1000 / data->frame_rate));
	} else {
		k_work_cancel_delayable_sync(&data->work, &work_sync);
	}

	return 0;
}

static const uint8_t pattern_rggb_idx[] = {0, 1, 1, 2};
static const uint8_t pattern_bggr_idx[] = {2, 1, 1, 0};
static const uint8_t pattern_gbrg_idx[] = {1, 2, 0, 1};
static const uint8_t pattern_grbg_idx[] = {1, 0, 2, 1};

/* White, Yellow, Cyan, Green, Magenta, Red, Blue, Black */

static const uint16_t pattern_8bars_yuv_bt709[8][3] = {
	{0xFE, 0x80, 0x7F}, {0xEC, 0x00, 0x8B}, {0xC8, 0x9D, 0x00}, {0xB6, 0x1D, 0x0C},
	{0x48, 0xE2, 0xF3}, {0x36, 0x62, 0xFF}, {0x12, 0xFF, 0x74}, {0x00, 0x80, 0x80},
};

static const uint16_t pattern_8bars_rgb[8][3] = {
	{0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0x00}, {0x00, 0xFF, 0xFF}, {0x00, 0xFF, 0x00},
	{0xFF, 0x00, 0xFF}, {0xFF, 0x00, 0x00}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0x00},
};

static inline int video_sw_generator_get_color_idx(uint16_t w, uint16_t width, bool hflip)
{
	/* If hflip is on, start from the right instead */
	w = (hflip) ? (width - w - 1) : (w);

	/* Downscale from w/width to #/8 */
	return 8 * w / width;
}

static uint16_t video_sw_generator_fill_yuyv(uint8_t *buffer, uint16_t width, bool hflip)
{
	if (width % 2 != 0) {
		LOG_ERR("YUYV pixels always go by pairs");
		return 0;
	}

	for (size_t w = 0; w < width; w += 2) {
		int color_idx = video_sw_generator_get_color_idx(w, width, hflip);

		buffer[w * 2 + 0] = pattern_8bars_yuv_bt709[color_idx][0];
		buffer[w * 2 + 1] = pattern_8bars_yuv_bt709[color_idx][1];
		buffer[w * 2 + 2] = pattern_8bars_yuv_bt709[color_idx][0];
		buffer[w * 2 + 3] = pattern_8bars_yuv_bt709[color_idx][2];
	}
	return 1;
}

static uint16_t video_sw_generator_fill_xrgb32(uint8_t *buffer, uint16_t width, bool hflip)
{
	for (size_t w = 0; w < width; w++) {
		int color_idx = video_sw_generator_get_color_idx(w, width, hflip);

		buffer[w * 4 + 0] = 0xff;
		buffer[w * 4 + 1] = pattern_8bars_rgb[color_idx][0];
		buffer[w * 4 + 2] = pattern_8bars_rgb[color_idx][1];
		buffer[w * 4 + 3] = pattern_8bars_rgb[color_idx][2];
	}
	return 1;
}

static uint16_t video_sw_generator_fill_rgb24(uint8_t *buffer, uint16_t width, bool hflip)
{
	for (size_t w = 0; w < width; w++) {
		int color_idx = video_sw_generator_get_color_idx(w, width, hflip);

		buffer[w * 3 + 0] = pattern_8bars_rgb[color_idx][0];
		buffer[w * 3 + 1] = pattern_8bars_rgb[color_idx][1];
		buffer[w * 3 + 2] = pattern_8bars_rgb[color_idx][2];
	}
	return 1;
}

static uint16_t video_sw_generator_fill_rgb565(uint8_t *buffer, uint16_t width, bool hflip)
{
	for (size_t w = 0; w < width; w++) {
		int color_idx = video_sw_generator_get_color_idx(w, width, hflip);
		uint8_t r = pattern_8bars_rgb[color_idx][0] >> (8 - 5);
		uint8_t g = pattern_8bars_rgb[color_idx][1] >> (8 - 6);
		uint8_t b = pattern_8bars_rgb[color_idx][2] >> (8 - 5);

		((uint16_t *)buffer)[w] = sys_cpu_to_le16((r << 11) | (g << 6) | (b << 0));
	}
	return 1;
}

static uint16_t video_sw_generator_fill_bayer8(uint8_t *buffer, uint16_t width, bool hflip,
					       const uint8_t *bayer_idx)
{
	uint8_t *row0 = buffer + 0;
	uint8_t *row1 = buffer + width;

	if (width % 2 != 0) {
		LOG_ERR("Bayer pixels always go by pairs (vertically and horizontally)");
		return 0;
	}

	for (size_t w = 0; w < width; w += 2) {
		int color_idx = video_sw_generator_get_color_idx(w, width, hflip);

		row0[w + 0] = pattern_8bars_rgb[color_idx][bayer_idx[0]];
		row0[w + 1] = pattern_8bars_rgb[color_idx][bayer_idx[1]];
		row1[w + 0] = pattern_8bars_rgb[color_idx][bayer_idx[2]];
		row1[w + 1] = pattern_8bars_rgb[color_idx][bayer_idx[3]];
	}
	return 2;
}

static int video_sw_generator_fill(const struct device *const dev, struct video_buffer *vbuf)
{
	struct video_sw_generator_data *data = dev->data;
	struct video_format *fmt = &data->fmt;
	size_t pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
	bool hflip = data->ctrls.hflip.val;
	uint16_t lines = 0;

	if (vbuf->size < pitch * 2) {
		LOG_ERR("At least 2 lines needed for bayer formats support");
		return -EINVAL;
	}

	/* Fill the first row of the emulated framebuffer */
	switch (data->fmt.pixelformat) {
	case VIDEO_PIX_FMT_YUYV:
		lines = video_sw_generator_fill_yuyv(vbuf->buffer, fmt->width, hflip);
		break;
	case VIDEO_PIX_FMT_XRGB32:
		lines = video_sw_generator_fill_xrgb32(vbuf->buffer, fmt->width, hflip);
		break;
	case VIDEO_PIX_FMT_RGB24:
		lines = video_sw_generator_fill_rgb24(vbuf->buffer, fmt->width, hflip);
		break;
	case VIDEO_PIX_FMT_RGB565:
		lines = video_sw_generator_fill_rgb565(vbuf->buffer, fmt->width, hflip);
		break;
	case VIDEO_PIX_FMT_SRGGB8:
		lines = video_sw_generator_fill_bayer8(vbuf->buffer, fmt->width, hflip,
						       pattern_rggb_idx);
		break;
	case VIDEO_PIX_FMT_SGBRG8:
		lines = video_sw_generator_fill_bayer8(vbuf->buffer, fmt->width, hflip,
						       pattern_gbrg_idx);
		break;
	case VIDEO_PIX_FMT_SBGGR8:
		lines = video_sw_generator_fill_bayer8(vbuf->buffer, fmt->width, hflip,
						       pattern_bggr_idx);
		break;
	case VIDEO_PIX_FMT_SGRBG8:
		lines = video_sw_generator_fill_bayer8(vbuf->buffer, fmt->width, hflip,
						       pattern_grbg_idx);
		break;
	default:
		CODE_UNREACHABLE;
		break;
	}

	if (lines == 0) {
		return -EINVAL;
	}

	/* How much was filled in so far */
	vbuf->bytesused = data->fmt.pitch * lines;

	/* Duplicate the first line(s) all over the buffer */
	for (int h = lines; h < data->fmt.height; h += lines) {
		if (vbuf->size < vbuf->bytesused + pitch * lines) {
			LOG_WRN("Generation stopped early: buffer too small");
			break;
		}
		memcpy(vbuf->buffer + h * pitch, vbuf->buffer, pitch * lines);
		vbuf->bytesused += pitch * lines;
	}

	vbuf->timestamp = k_uptime_get_32();
	vbuf->line_offset = 0;

	return 0;
}

static void video_sw_generator_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct video_sw_generator_data *data;
	struct video_buffer *vbuf;

	data = CONTAINER_OF(dwork, struct video_sw_generator_data, work);

	k_work_reschedule(&data->work, K_MSEC(1000 / data->frame_rate));

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		return;
	}

	switch (data->pattern) {
	case VIDEO_PATTERN_COLOR_BAR:
		video_sw_generator_fill(data->dev, vbuf);
		break;
	}

	k_fifo_put(&data->fifo_out, vbuf);

	if (IS_ENABLED(CONFIG_POLL) && data->sig) {
		k_poll_signal_raise(data->sig, VIDEO_BUF_DONE);
	}

	k_yield();
}

static int video_sw_generator_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_sw_generator_data *data = dev->data;

	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
}

static int video_sw_generator_dequeue(const struct device *dev, struct video_buffer **vbuf,
				      k_timeout_t timeout)
{
	struct video_sw_generator_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_sw_generator_flush(const struct device *dev, bool cancel)
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
			if (IS_ENABLED(CONFIG_POLL) && data->sig) {
				k_poll_signal_raise(data->sig, VIDEO_BUF_ABORTED);
			}
		}
	}

	return 0;
}

static int video_sw_generator_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = fmts;
	caps->min_vbuf_count = 1;

	/* SW generator produces full frames */
	caps->min_line_count = caps->max_line_count = LINE_COUNT_HEIGHT;

	return 0;
}

#ifdef CONFIG_POLL
static int video_sw_generator_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_sw_generator_data *data = dev->data;

	if (data->sig && sig != NULL) {
		return -EALREADY;
	}

	data->sig = sig;

	return 0;
}
#endif

static int video_sw_generator_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct video_sw_generator_data *data = dev->data;

	data->frame_rate = CLAMP(DIV_ROUND_CLOSEST(frmival->denominator, frmival->numerator),
				 MIN_FRAME_RATE, MAX_FRAME_RATE);
	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;

	return 0;
}

static int video_sw_generator_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct video_sw_generator_data *data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = data->frame_rate;

	return 0;
}

static int video_sw_generator_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	size_t idx;
	int ret;

	if (fie->index >= 1) {
		return -ERANGE;
	}

	ret = video_format_caps_index(fmts, fie->format, &idx);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return ret;
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
	k_work_init_delayable(&data->work, video_sw_generator_worker);

	return video_sw_generator_init_controls(dev);
}

#define VIDEO_SW_GENERATOR_DEFINE(n)                                                               \
	static struct video_sw_generator_data video_sw_generator_data_##n = {                      \
		.fmt.width = 320,                                                                  \
		.fmt.height = 160,                                                                 \
		.fmt.pitch = 320 * 2,                                                              \
		.fmt.pixelformat = VIDEO_PIX_FMT_RGB565,                                           \
		.frame_rate = DEFAULT_FRAME_RATE,                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &video_sw_generator_init, NULL, &video_sw_generator_data_##n,     \
			      NULL, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,                       \
			      &video_sw_generator_driver_api);                                     \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(video_sw_generator_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_SW_GENERATOR_DEFINE)

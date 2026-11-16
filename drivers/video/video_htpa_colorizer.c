/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT heimann_htpa_colorizer

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/htpa_sensor.h>
#include <zephyr/drivers/video/htpa_colorizer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_htpa_colorizer, CONFIG_VIDEO_LOG_LEVEL);

#define HTPA_HISTOGRAM_BINS 256U
#define HTPA_CLIP_PERCENT   1U

struct htpa_color_config {
	const struct device *source;
};

struct htpa_color_data {
	struct k_fifo pending;
	struct k_fifo completed;
	struct k_sem worker_sem;
	struct k_mutex lock;
	struct k_condvar changed;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_VIDEO_HTPA_COLORIZER_STACK_SIZE);
	uint32_t source_owned;
	uint32_t raw_size;
	uint8_t source_min_buffers;
	size_t source_alignment;
	bool source_active;
	struct video_format format;
	struct video_format_cap caps[4];
	uint16_t histogram[HTPA_HISTOGRAM_BINS];
	struct video_ctrl palette_ctrl;
	atomic_t palette;
	struct video_buffer *in_flight;
	bool streaming;
	bool flushing;
	bool canceling;
	bool source_failed;
};

static uint32_t htpa_color_bin(int16_t sample, int16_t minimum, uint32_t range)
{
	return (uint32_t)((int32_t)sample - minimum) * (HTPA_HISTOGRAM_BINS - 1U) / range;
}

static int16_t htpa_color_sample(const uint8_t *pixels, uint32_t index)
{
	int16_t sample;

	memcpy(&sample, pixels + index * sizeof(sample), sizeof(sample));
	return sample;
}

static void htpa_color_grey(struct htpa_color_data *data, uint8_t *output)
{
	uint32_t width = data->format.width;
	uint32_t height = data->format.height;
	const uint8_t *pixels = output;
	uint32_t clip_count = width * height * HTPA_CLIP_PERCENT / 100U;
	int16_t minimum = INT16_MAX, maximum = INT16_MIN;
	int16_t retained_min = INT16_MAX, retained_max = INT16_MIN;
	uint32_t lower_bin, upper_bin, cumulative, range;
	uint32_t lower_threshold, upper_threshold;
	float lower_bound, scaling_range;
	float scale;
	uint32_t start = k_cycle_get_32();
	uint32_t next;

	for (uint32_t y = 0U; y < height; y++) {
		for (uint32_t x = 0U; x < width; x++) {
			int16_t sample = htpa_color_sample(pixels, y * width + x);

			minimum = MIN(minimum, sample);
			maximum = MAX(maximum, sample);
		}
	}
	next = k_cycle_get_32();
	LOG_DBG("Pixel range: %u us", k_cyc_to_us_floor32(next - start));
	start = next;
	if (minimum == maximum) {
		memset(output, UINT8_MAX / 2U, width * height);
		return;
	}
	range = (uint32_t)((int32_t)maximum - minimum);
	memset(data->histogram, 0, sizeof(data->histogram));
	for (uint32_t y = 0U; y < height; y++) {
		for (uint32_t x = 0U; x < width; x++) {
			uint32_t bin = htpa_color_bin(htpa_color_sample(pixels, y * width + x),
						      minimum, range);

			data->histogram[bin]++;
		}
	}
	next = k_cycle_get_32();
	LOG_DBG("Histogram: %u us", k_cyc_to_us_floor32(next - start));
	start = next;
	cumulative = 0U;
	for (lower_bin = 0U; lower_bin < HTPA_HISTOGRAM_BINS; lower_bin++) {
		cumulative += data->histogram[lower_bin];
		if (cumulative > clip_count) {
			break;
		}
	}
	cumulative = 0U;
	for (upper_bin = HTPA_HISTOGRAM_BINS - 1U; upper_bin > 0U; upper_bin--) {
		cumulative += data->histogram[upper_bin];
		if (cumulative > clip_count) {
			break;
		}
	}
	lower_threshold = DIV_ROUND_UP(lower_bin * range, HTPA_HISTOGRAM_BINS - 1U);
	upper_threshold = DIV_ROUND_UP((upper_bin + 1U) * range, HTPA_HISTOGRAM_BINS - 1U);
	for (uint32_t y = 0U; y < height; y++) {
		for (uint32_t x = 0U; x < width; x++) {
			int16_t sample = htpa_color_sample(pixels, y * width + x);
			uint32_t value = (uint32_t)((int32_t)sample - minimum);

			if (value >= lower_threshold && value < upper_threshold) {
				retained_min = MIN(retained_min, sample);
				retained_max = MAX(retained_max, sample);
			}
		}
	}
	next = k_cycle_get_32();
	LOG_DBG("Clipping range: %u us", k_cyc_to_us_floor32(next - start));
	start = next;
	if (retained_min < retained_max) {
		minimum = retained_min;
		maximum = retained_max;
	}
	scaling_range =
		(float)MAX((int32_t)maximum - minimum, CONFIG_VIDEO_HTPA_COLORIZER_MIN_GREY_RANGE);
	lower_bound = ((float)minimum + maximum - scaling_range) / 2.0f;
	scale = (float)UINT8_MAX / scaling_range;
	for (uint32_t y = 0U; y < height; y++) {
		for (uint32_t x = 0U; x < width; x++) {
			float pixel =
				((float)htpa_color_sample(pixels, y * width + x) - lower_bound) *
				scale;

			output[y * width + x] = (uint8_t)CLAMP(pixel, 0.0f, (float)UINT8_MAX);
		}
	}
	LOG_DBG("Grayscale scaling: %u us", k_cyc_to_us_floor32(k_cycle_get_32() - start));
}

static uint16_t htpa_color_rgb565(uint8_t value, int palette)
{
	uint32_t r, g, b;

	if (palette == VIDEO_HTPA_PALETTE_RAINBOW) {
		uint32_t pos = (uint32_t)value * 4U;

		r = CLAMP((int32_t)pos - 510, 0, 255);
		g = CLAMP(510 - ((pos > 510U) ? (int32_t)pos - 510 : 510 - (int32_t)pos), 0, 255);
		b = CLAMP(510 - (int32_t)pos, 0, 255);
	} else if (palette == VIDEO_HTPA_PALETTE_INFERNO) {
		static const uint8_t anchors[][3] = {
			{0U, 0U, 4U},      {31U, 12U, 72U},   {85U, 15U, 109U},
			{136U, 34U, 106U}, {186U, 54U, 85U},  {227U, 89U, 51U},
			{249U, 140U, 10U}, {249U, 201U, 50U}, {252U, 255U, 164U},
		};
		uint32_t pos = (uint32_t)value * 8U;
		uint32_t index = pos / 255U;
		uint32_t fraction = pos % 255U;

		if (index == 8U) {
			index = 7U;
			fraction = 255U;
		}
		r = (anchors[index][0] * (255U - fraction) + anchors[index + 1U][0] * fraction) /
		    255U;
		g = (anchors[index][1] * (255U - fraction) + anchors[index + 1U][1] * fraction) /
		    255U;
		b = (anchors[index][2] * (255U - fraction) + anchors[index + 1U][2] * fraction) /
		    255U;
	} else {
		r = MIN((uint32_t)value * 3U, 255U);
		g = CLAMP((int32_t)value * 3 - 255, 0, 255);
		b = CLAMP((int32_t)value * 3 - 510, 0, 255);
	}
	return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void htpa_color_convert(struct htpa_color_data *data, uint8_t *buffer, int palette)
{
	uint32_t count = data->format.width * data->format.height;

	if (data->format.pixelformat == VIDEO_PIX_FMT_HTPA_S16) {
		return;
	}
	htpa_color_grey(data, buffer);
	if (data->format.pixelformat == VIDEO_PIX_FMT_RGB565) {
		while (count > 0U) {
			uint16_t pixel;

			count--;
			pixel = htpa_color_rgb565(buffer[count], palette);
			sys_put_le16(pixel, buffer + count * 2U);
		}
	}
}

static int htpa_color_set_ctrl(const struct device *dev, uint32_t id)
{
	struct htpa_color_data *data = dev->data;

	if (id != VIDEO_CID_HTPA_PALETTE) {
		return -ENOTSUP;
	}
	atomic_set(&data->palette, data->palette_ctrl.val);
	return 0;
}

/* Only the worker transfers ownership to and from the source. */
static int htpa_color_source_stop(const struct device *dev)
{
	const struct htpa_color_config *cfg = dev->config;
	struct htpa_color_data *data = dev->data;
	int ret;

	if (data->source_active) {
		ret = video_driver_set_stream(cfg->source, false, VIDEO_BUF_TYPE_OUTPUT);
		if (ret != 0) {
			return ret;
		}
		data->source_active = false;
	}
	ret = video_driver_flush(cfg->source, true);
	if (ret != 0) {
		return ret;
	}
	while (data->source_owned > 0U) {
		struct video_buffer *buffer = NULL;

		ret = video_driver_dequeue(cfg->source, &buffer, K_NO_WAIT);
		if (ret != 0 || buffer == NULL) {
			return ret != 0 ? ret : -EIO;
		}
		data->source_owned--;
		buffer->bytesused = 0U;
		k_fifo_put(&data->completed, buffer);
	}
	return 0;
}

static void htpa_color_worker(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	const struct htpa_color_config *cfg = dev->config;
	struct htpa_color_data *data = dev->data;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	while (true) {
		struct video_buffer *buffer;
		bool run;
		int ret;

		k_mutex_lock(&data->lock, K_FOREVER);
		run = (data->streaming || data->flushing) && !data->canceling &&
		      !data->source_failed;
		if (!run) {
			if (data->source_active || data->source_owned > 0U) {
				ret = htpa_color_source_stop(dev);
				if (ret != 0) {
					data->source_failed = true;
				}
			}
			k_condvar_broadcast(&data->changed);
			k_mutex_unlock(&data->lock);
			k_sem_take(&data->worker_sem, K_FOREVER);
			continue;
		}
		while ((buffer = k_fifo_get(&data->pending, K_NO_WAIT)) != NULL) {
			ret = video_driver_enqueue(cfg->source, buffer);
			if (ret != 0) {
				buffer->bytesused = 0U;
				k_fifo_put(&data->completed, buffer);
				LOG_ERR("Source enqueue failed: %d", ret);
			} else {
				data->source_owned++;
			}
		}
		if (!data->source_active && data->source_owned >= data->source_min_buffers &&
		    data->source_owned > 0U) {
			ret = video_stream_start(cfg->source, VIDEO_BUF_TYPE_OUTPUT);
			if (ret != 0) {
				if (htpa_color_source_stop(dev) != 0) {
					data->source_failed = true;
				}
				LOG_ERR("Source start failed: %d", ret);
			} else {
				data->source_active = true;
			}
		}
		if (data->source_owned == 0U || !data->source_active || data->source_failed) {
			if (data->flushing && !data->streaming && data->source_active) {
				if (htpa_color_source_stop(dev) != 0) {
					data->source_failed = true;
				}
			}
			k_condvar_broadcast(&data->changed);
			k_mutex_unlock(&data->lock);
			k_sem_take(&data->worker_sem, K_FOREVER);
			continue;
		}
		k_mutex_unlock(&data->lock);
		ret = video_driver_dequeue(cfg->source, &buffer, K_MSEC(10));
		k_mutex_lock(&data->lock, K_FOREVER);
		if (ret == 0) {
			data->source_owned--;
			data->in_flight = buffer;
			if (data->canceling || buffer->bytesused != data->format.width *
									    data->format.height *
									    sizeof(int16_t)) {
				buffer->bytesused = 0U;
			} else {
				k_mutex_unlock(&data->lock);
				htpa_color_convert(data, buffer->buffer,
						   atomic_get(&data->palette));
				k_mutex_lock(&data->lock, K_FOREVER);
				buffer->bytesused =
					data->canceling ? 0U
							: data->format.pitch * data->format.height;
				buffer->line_offset = 0U;
			}
			k_fifo_put(&data->completed, buffer);
			data->in_flight = NULL;
		} else if (ret != -EAGAIN) {
			LOG_ERR("Source dequeue failed: %d", ret);
			if (htpa_color_source_stop(dev) != 0) {
				data->source_failed = true;
			}
		}
		k_condvar_broadcast(&data->changed);
		k_mutex_unlock(&data->lock);
	}
}

static int htpa_color_get_format(const struct device *dev, struct video_format *fmt)
{
	struct htpa_color_data *data = dev->data;

	*fmt = data->format;
	return 0;
}

static int htpa_color_set_format(const struct device *dev, struct video_format *fmt)
{
	struct htpa_color_data *data = dev->data;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT ||
	    (fmt->pixelformat != VIDEO_PIX_FMT_HTPA_S16 && fmt->pixelformat != VIDEO_PIX_FMT_GREY &&
	     fmt->pixelformat != VIDEO_PIX_FMT_RGB565) ||
	    fmt->width != data->format.width || fmt->height != data->format.height) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->streaming || data->flushing || data->in_flight != NULL ||
	    data->source_owned != 0U || !k_fifo_is_empty(&data->pending)) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}
	data->format.pixelformat = fmt->pixelformat;
	data->format.pitch =
		fmt->width * (fmt->pixelformat == VIDEO_PIX_FMT_GREY ? 1U : sizeof(int16_t));
	data->format.size = data->raw_size;
	*fmt = data->format;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int htpa_color_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct htpa_color_data *data = dev->data;

	caps->type = VIDEO_BUF_TYPE_OUTPUT;
	caps->format_caps = data->caps;
	caps->min_vbuf_count = data->source_min_buffers;
	caps->buf_align = data->source_alignment;
	return 0;
}

static int htpa_color_flush(const struct device *dev, bool cancel);

static int htpa_color_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct htpa_color_data *data = dev->data;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	data->streaming = enable;
	if (enable) {
		k_sem_give(&data->worker_sem);
	}
	k_mutex_unlock(&data->lock);
	return enable ? 0 : htpa_color_flush(dev, true);
}

static int htpa_color_enqueue(const struct device *dev, struct video_buffer *buffer)
{
	struct htpa_color_data *data = dev->data;

	if (buffer->type != VIDEO_BUF_TYPE_OUTPUT || buffer->buffer == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (buffer->size < data->raw_size ||
	    (data->source_alignment > 1U &&
	     !IS_ALIGNED((uintptr_t)buffer->buffer, data->source_alignment))) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}
	if (data->flushing || data->source_failed) {
		k_mutex_unlock(&data->lock);
		return data->source_failed ? -EIO : -EBUSY;
	}
	buffer->bytesused = 0U;
	buffer->timestamp = 0U;
	buffer->line_offset = 0U;
	k_fifo_put(&data->pending, buffer);
	if (data->streaming) {
		k_sem_give(&data->worker_sem);
	}
	k_mutex_unlock(&data->lock);
	return 0;
}

static int htpa_color_dequeue(const struct device *dev, struct video_buffer **buffer,
			      k_timeout_t timeout)
{
	struct htpa_color_data *data = dev->data;

	*buffer = k_fifo_get(&data->completed, timeout);
	return *buffer == NULL ? -EAGAIN : 0;
}

static int htpa_color_flush(const struct device *dev, bool cancel)
{
	struct htpa_color_data *data = dev->data;
	struct video_buffer *buffer;

	k_mutex_lock(&data->lock, K_FOREVER);
	while (data->flushing) {
		k_condvar_wait(&data->changed, &data->lock, K_FOREVER);
	}
	data->flushing = true;
	data->canceling = cancel;
	if (cancel) {
		while ((buffer = k_fifo_get(&data->pending, K_NO_WAIT)) != NULL) {
			buffer->bytesused = 0U;
			k_fifo_put(&data->completed, buffer);
		}
	}
	k_sem_give(&data->worker_sem);
	while (!data->source_failed && (data->in_flight != NULL || data->source_owned > 0U ||
					(!cancel && !k_fifo_is_empty(&data->pending)) ||
					((cancel || !data->streaming) && data->source_active))) {
		k_condvar_wait(&data->changed, &data->lock, K_FOREVER);
	}
	data->canceling = false;
	data->flushing = false;
	k_condvar_broadcast(&data->changed);
	k_mutex_unlock(&data->lock);
	return data->source_failed ? -EIO : 0;
}

static int htpa_color_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct htpa_color_config *cfg = dev->config;

	return video_set_frmival(cfg->source, frmival);
}

static int htpa_color_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct htpa_color_config *cfg = dev->config;

	return video_get_frmival(cfg->source, frmival);
}

static int htpa_color_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct htpa_color_config *cfg = dev->config;
	struct htpa_color_data *data = dev->data;
	struct video_frmival_enum source_fie = *fie;
	struct video_format source_fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	int ret;

	if ((fie->format->pixelformat != VIDEO_PIX_FMT_GREY &&
	     fie->format->pixelformat != VIDEO_PIX_FMT_HTPA_S16 &&
	     fie->format->pixelformat != VIDEO_PIX_FMT_RGB565) ||
	    fie->format->width != data->format.width ||
	    fie->format->height != data->format.height) {
		return -EINVAL;
	}
	ret = video_get_format(cfg->source, &source_fmt);
	if (ret != 0) {
		return ret;
	}
	source_fie.format = &source_fmt;
	ret = video_enum_frmival(cfg->source, &source_fie);
	if (ret == 0) {
		fie->type = source_fie.type;
		if (fie->type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
			fie->discrete = source_fie.discrete;
		} else {
			fie->stepwise = source_fie.stepwise;
		}
	}
	return ret;
}

static DEVICE_API(video, htpa_color_api) = {
	.set_ctrl = htpa_color_set_ctrl,
	.set_format = htpa_color_set_format,
	.get_format = htpa_color_get_format,
	.set_frmival = htpa_color_set_frmival,
	.get_frmival = htpa_color_get_frmival,
	.enum_frmival = htpa_color_enum_frmival,
	.get_caps = htpa_color_get_caps,
	.set_stream = htpa_color_set_stream,
	.enqueue = htpa_color_enqueue,
	.dequeue = htpa_color_dequeue,
	.flush = htpa_color_flush,
};

static int htpa_color_init(const struct device *dev)
{
	const struct htpa_color_config *cfg = dev->config;
	struct htpa_color_data *data = dev->data;
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	size_t index;
	int ret;

	if (!device_is_ready(cfg->source)) {
		return -ENODEV;
	}
	ret = video_get_format(cfg->source, &fmt);
	if (ret != 0) {
		return ret;
	}
	if (fmt.type != VIDEO_BUF_TYPE_OUTPUT || fmt.pixelformat != VIDEO_PIX_FMT_HTPA_S16) {
		return -ENOTSUP;
	}
	ret = video_get_caps(cfg->source, &caps);
	if (ret != 0) {
		return ret;
	}
	if (caps.type != VIDEO_BUF_TYPE_OUTPUT || caps.format_caps == NULL ||
	    caps.min_vbuf_count != 1U) {
		return -ENOTSUP;
	}
	ret = video_format_caps_index(caps.format_caps, &fmt, &index);
	if (ret != 0) {
		return ret;
	}
	if (fmt.width == 0U || fmt.height == 0U ||
	    (uint64_t)fmt.width * fmt.height > UINT32_MAX / sizeof(int16_t) ||
	    fmt.size < fmt.width * fmt.height * sizeof(int16_t)) {
		return -ENOTSUP;
	}
	data->raw_size = fmt.size;
	data->source_min_buffers = caps.min_vbuf_count;
	data->source_alignment = caps.buf_align;
	data->format = (struct video_format){
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_HTPA_S16,
		.width = fmt.width,
		.height = fmt.height,
		.pitch = fmt.width * sizeof(int16_t),
		.size = fmt.size,
	};
	data->caps[0] = (struct video_format_cap){
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width_min = fmt.width,
		.width_max = fmt.width,
		.height_min = fmt.height,
		.height_max = fmt.height,
	};
	data->caps[1] = data->caps[0];
	data->caps[1].pixelformat = VIDEO_PIX_FMT_HTPA_S16;
	data->caps[2] = data->caps[0];
	data->caps[2].pixelformat = VIDEO_PIX_FMT_RGB565;
	ret = video_init_ctrl(&data->palette_ctrl, dev, VIDEO_CID_HTPA_PALETTE,
			      (struct video_ctrl_range){.min = VIDEO_HTPA_PALETTE_IRONBOW,
							.max = VIDEO_HTPA_PALETTE_INFERNO,
							.step = 1,
							.def = VIDEO_HTPA_PALETTE_IRONBOW});
	if (ret != 0) {
		return ret;
	}
	atomic_set(&data->palette, VIDEO_HTPA_PALETTE_IRONBOW);
	k_fifo_init(&data->pending);
	k_fifo_init(&data->completed);
	k_sem_init(&data->worker_sem, 0, 1);
	k_mutex_init(&data->lock);
	k_condvar_init(&data->changed);
	k_thread_create(&data->thread, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
			htpa_color_worker, (void *)dev, NULL, NULL,
			CONFIG_VIDEO_HTPA_COLORIZER_THREAD_PRIORITY, 0, K_NO_WAIT);
	return 0;
}

#define HTPA_COLOR_DEFINE(inst)                                                                    \
	static struct htpa_color_data htpa_color_data_##inst;                                      \
	static const struct htpa_color_config htpa_color_cfg_##inst = {                            \
		.source = DEVICE_DT_GET(DT_INST_PHANDLE(inst, source)),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, htpa_color_init, NULL, &htpa_color_data_##inst,                \
			      &htpa_color_cfg_##inst, POST_KERNEL,                                 \
			      CONFIG_VIDEO_HTPA_COLORIZER_INIT_PRIORITY, &htpa_color_api);         \
	VIDEO_DEVICE_DEFINE(htpa_color_##inst, DEVICE_DT_INST_GET(inst),                           \
			    DEVICE_DT_GET(DT_INST_PHANDLE(inst, source)));

DT_INST_FOREACH_STATUS_OKAY(HTPA_COLOR_DEFINE)

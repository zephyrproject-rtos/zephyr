/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT heimann_htpa_processor

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/htpa_sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_htpa_processor, CONFIG_VIDEO_LOG_LEVEL);

#define HTPA_HISTOGRAM_BINS 256U
#define HTPA_CLIP_PERCENT   1U
#define HTPA_RAW_BUFFERS    CONFIG_VIDEO_HTPA_PROCESSOR_RAW_BUFFER_COUNT

struct htpa_proc_config {
	const struct device *source;
};

struct htpa_proc_data {
	struct k_fifo pending;
	struct k_fifo completed;
	struct k_sem worker_sem;
	struct k_mutex lock;
	struct k_condvar changed;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_VIDEO_HTPA_PROCESSOR_STACK_SIZE);
	struct video_buffer raw[HTPA_RAW_BUFFERS];
	uint8_t raw_data[HTPA_RAW_BUFFERS]
			[ROUND_UP(CONFIG_VIDEO_HTPA_PROCESSOR_RAW_BUFFER_SIZE, 4U)] __aligned(4);
	/* A set bit means the source owns the corresponding raw buffer. */
	uint32_t source_owned;
	atomic_t source_active;
	struct video_format format;
	struct video_format_cap caps[3];
	uint16_t histogram[HTPA_HISTOGRAM_BINS];
	int16_t pixels[MIN(VIDEO_HTPA_160X120_WIDTH * VIDEO_HTPA_160X120_HEIGHT,
			   DIV_ROUND_UP(CONFIG_VIDEO_HTPA_PROCESSOR_RAW_BUFFER_SIZE, 2U))];
	struct video_htpa_raw_layout layout;
	struct video_buffer *in_flight;
	atomic_t canceled;
	bool streaming;
	bool flushing;
	bool canceling;
	atomic_t source_failed;
};

static void htpa_proc_pixels(const struct htpa_proc_data *data, const uint8_t *raw,
			     uint8_t *output)
{
	const struct video_htpa_raw_layout *layout = &data->layout;
	uint32_t width = data->format.width;
	uint32_t height = data->format.height;

	for (uint32_t half = 0U; half < 2U; half++) {
		uint32_t x = 0U;
		uint32_t y = half == 0U ? 0U : height - 1U;
		const uint8_t *offsets = raw + half * layout->block_length + layout->data_offset;
		const uint8_t *blocks = raw + (2U + half * layout->block_count) *
					      layout->block_length;

		for (uint32_t block = 0U; block < layout->block_count; block++) {
			for (uint32_t i = 0U; i < layout->samples_per_block; i++) {
				uint16_t pixel =
					sys_get_be16(blocks + block * layout->block_length +
						     layout->data_offset + i * 2U);
				uint16_t offset = sys_get_be16(offsets + i * 2U);
				int32_t difference = (int32_t)pixel - offset;
				int16_t corrected =
					(int16_t)CLAMP(difference, INT16_MIN, INT16_MAX);

				memcpy(output + (y * width + x) * sizeof(corrected),
				       &corrected, sizeof(corrected));
				x++;
				if (x == width) {
					x = 0U;
					if (half == 0U) {
						y++;
					} else {
						y--;
					}
				}
			}
		}
	}
}

static uint32_t htpa_proc_bin(int16_t sample, int16_t minimum, uint32_t range)
{
	return (uint32_t)((int32_t)sample - minimum) * (HTPA_HISTOGRAM_BINS - 1U) / range;
}

static void htpa_proc_grey(struct htpa_proc_data *data, const uint8_t *raw, uint8_t *output)
{
	uint32_t width = data->format.width;
	uint32_t height = data->format.height;
	const int16_t *pixels = data->pixels;
	uint32_t clip_count = width * height * HTPA_CLIP_PERCENT / 100U;
	int16_t minimum = INT16_MAX, maximum = INT16_MIN;
	int16_t retained_min = INT16_MAX, retained_max = INT16_MIN;
	uint32_t lower_bin, upper_bin, cumulative, range;
	uint32_t lower_threshold, upper_threshold;
	float lower_bound, scaling_range;
	float scale;
	uint32_t start = k_cycle_get_32();
	uint32_t next;

	htpa_proc_pixels(data, raw, (uint8_t *)data->pixels);
	next = k_cycle_get_32();
	LOG_DBG("Pixel correction: %u us", k_cyc_to_us_floor32(next - start));
	start = next;
	for (uint32_t y = 0U; y < height; y++) {
		for (uint32_t x = 0U; x < width; x++) {
			int16_t sample = pixels[y * width + x];

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
			uint32_t bin = htpa_proc_bin(pixels[y * width + x], minimum, range);

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
			int16_t sample = pixels[y * width + x];
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
	scaling_range = (float)MAX((int32_t)maximum - minimum,
				   CONFIG_VIDEO_HTPA_PROCESSOR_MIN_GREY_RANGE);
	lower_bound = ((float)minimum + maximum - scaling_range) / 2.0f;
	scale = (float)UINT8_MAX / scaling_range;
	for (uint32_t y = 0U; y < height; y++) {
		for (uint32_t x = 0U; x < width; x++) {
			float pixel = ((float)pixels[y * width + x] - lower_bound) * scale;

			output[y * width + x] = (uint8_t)CLAMP(pixel, 0.0f, (float)UINT8_MAX);
		}
	}
	LOG_DBG("Grayscale scaling: %u us", k_cyc_to_us_floor32(k_cycle_get_32() - start));
}

static int htpa_proc_source_stop(const struct device *dev)
{
	const struct htpa_proc_config *cfg = dev->config;
	struct htpa_proc_data *data = dev->data;
	int ret;
	int flush_ret;

	ret = video_driver_set_stream(cfg->source, false, VIDEO_BUF_TYPE_OUTPUT);
	flush_ret = video_driver_flush(cfg->source, true);
	if (ret != 0 || flush_ret != 0) {
		atomic_set(&data->source_failed, 1);
		return ret != 0 ? ret : flush_ret;
	}
	while (data->source_owned != 0U) {
		struct video_buffer *raw = NULL;
		uint32_t index;

		ret = video_driver_dequeue(cfg->source, &raw, K_NO_WAIT);
		if (ret != 0) {
			atomic_set(&data->source_failed, 1);
			return ret;
		}
		for (index = 0U; index < HTPA_RAW_BUFFERS; index++) {
			if (raw == &data->raw[index]) {
				break;
			}
		}
		if (index == HTPA_RAW_BUFFERS || (data->source_owned & BIT(index)) == 0U) {
			atomic_set(&data->source_failed, 1);
			return -EIO;
		}
		data->source_owned &= ~BIT(index);
	}
	atomic_clear(&data->source_active);
	return 0;
}

static int htpa_proc_source_start(const struct device *dev)
{
	const struct htpa_proc_config *cfg = dev->config;
	struct htpa_proc_data *data = dev->data;
	int ret;

	if (atomic_get(&data->source_failed) != 0) {
		return -EIO;
	}
	atomic_set(&data->source_active, 1);
	for (uint32_t i = 0U; i < HTPA_RAW_BUFFERS; i++) {
		data->raw[i].bytesused = 0U;
		ret = video_driver_enqueue(cfg->source, &data->raw[i]);
		if (ret != 0) {
			goto fail;
		}
		data->source_owned |= BIT(i);
	}
	ret = video_stream_start(cfg->source, VIDEO_BUF_TYPE_OUTPUT);
	if (ret != 0) {
		goto fail;
	}
	return 0;

fail:
	if (htpa_proc_source_stop(dev) != 0) {
		atomic_set(&data->source_failed, 1);
	}
	return ret;
}

static int htpa_proc_capture(const struct device *dev, struct video_buffer *output)
{
	const struct htpa_proc_config *cfg = dev->config;
	struct htpa_proc_data *data = dev->data;
	struct video_buffer *raw = NULL;
	uint32_t index;
	int ret;

	if (atomic_get(&data->source_failed) != 0) {
		return -EIO;
	}
	if (atomic_get(&data->source_active) == 0) {
		ret = htpa_proc_source_start(dev);
		if (ret != 0) {
			return ret;
		}
	}
	while (true) {
		if (atomic_get(&data->canceled) != 0) {
			return -ECANCELED;
		}
		ret = video_driver_dequeue(cfg->source, &raw, K_MSEC(10));
		if (ret != -EAGAIN) {
			break;
		}
	}
	if (ret != 0) {
		return ret;
	}
	for (index = 0U; index < HTPA_RAW_BUFFERS; index++) {
		if (raw == &data->raw[index]) {
			break;
		}
	}
	if (index == HTPA_RAW_BUFFERS || (data->source_owned & BIT(index)) == 0U) {
		if (htpa_proc_source_stop(dev) != 0) {
			LOG_ERR("Failed to stop source after invalid buffer");
		}
		atomic_set(&data->source_failed, 1);
		return -EIO;
	}
	data->source_owned &= ~BIT(index);
	if (raw->bytesused != raw->size) {
		ret = -EIO;
	} else {
		uint32_t start = k_cycle_get_32();

		if (data->format.pixelformat == VIDEO_PIX_FMT_GREY) {
			htpa_proc_grey(data, raw->buffer, output->buffer);
		} else {
			htpa_proc_pixels(data, raw->buffer, output->buffer);
		}
		LOG_INF("Frame processing: %u us", k_cyc_to_us_floor32(k_cycle_get_32() - start));
		output->bytesused = data->format.size;
		output->timestamp = raw->timestamp;
		ret = 0;
	}
	if (atomic_get(&data->canceled) == 0) {
		int enqueue_ret = video_driver_enqueue(cfg->source, raw);

		if (enqueue_ret == 0) {
			data->source_owned |= BIT(index);
		} else {
			ret = enqueue_ret;
		}
	}
	return ret;
}

static void htpa_proc_worker(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct htpa_proc_data *data = dev->data;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	while (true) {
		k_sem_take(&data->worker_sem, K_FOREVER);
		while (true) {
			struct video_buffer *buffer;
			int ret;

			k_mutex_lock(&data->lock, K_FOREVER);
			if (data->canceling || (!data->streaming && !data->flushing)) {
				k_mutex_unlock(&data->lock);
				if (atomic_get(&data->source_active) != 0 &&
				    atomic_get(&data->source_failed) == 0 &&
				    htpa_proc_source_stop(dev) != 0) {
					LOG_ERR("Failed to stop source");
				}
				k_mutex_lock(&data->lock, K_FOREVER);
				k_condvar_broadcast(&data->changed);
				k_mutex_unlock(&data->lock);
				break;
			}
			buffer = k_fifo_get(&data->pending, K_NO_WAIT);
			if (buffer == NULL) {
				/* The source stops acquiring when all raw buffers are full. */
				bool stop_source = !data->streaming;

				k_mutex_unlock(&data->lock);
				if (stop_source && atomic_get(&data->source_active) != 0 &&
				    atomic_get(&data->source_failed) == 0 &&
				    htpa_proc_source_stop(dev) != 0) {
					LOG_ERR("Failed to stop source");
				}
				k_mutex_lock(&data->lock, K_FOREVER);
				k_condvar_broadcast(&data->changed);
				k_mutex_unlock(&data->lock);
				break;
			}
			data->in_flight = buffer;
			atomic_clear(&data->canceled);
			k_mutex_unlock(&data->lock);

			ret = htpa_proc_capture(dev, buffer);
			if (ret != 0 && atomic_get(&data->source_active) != 0 &&
			    atomic_get(&data->source_failed) == 0 &&
			    htpa_proc_source_stop(dev) != 0) {
				LOG_ERR("Failed to stop source after capture error");
			}
			k_mutex_lock(&data->lock, K_FOREVER);
			if (ret != 0 || atomic_get(&data->canceled) != 0) {
				buffer->bytesused = 0U;
				if (ret != 0 && ret != -ECANCELED) {
					LOG_ERR("Frame capture failed: %d", ret);
				}
			}
			k_fifo_put(&data->completed, buffer);
			data->in_flight = NULL;
			k_condvar_broadcast(&data->changed);
			k_mutex_unlock(&data->lock);
		}
	}
}

static int htpa_proc_get_format(const struct device *dev, struct video_format *fmt)
{
	struct htpa_proc_data *data = dev->data;

	*fmt = data->format;
	return 0;
}

static int htpa_proc_set_format(const struct device *dev, struct video_format *fmt)
{
	struct htpa_proc_data *data = dev->data;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT ||
	    (fmt->pixelformat != VIDEO_PIX_FMT_HTPA_S16 &&
	     fmt->pixelformat != VIDEO_PIX_FMT_GREY) ||
	    fmt->width != data->format.width || fmt->height != data->format.height) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->streaming || data->flushing || data->in_flight != NULL ||
	    !k_fifo_is_empty(&data->pending)) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}
	data->format.pixelformat = fmt->pixelformat;
	data->format.pitch = fmt->width *
			     (fmt->pixelformat == VIDEO_PIX_FMT_GREY ? 1U : sizeof(int16_t));
	data->format.size = data->format.pitch * fmt->height;
	*fmt = data->format;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int htpa_proc_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct htpa_proc_data *data = dev->data;

	caps->type = VIDEO_BUF_TYPE_OUTPUT;
	caps->format_caps = data->caps;
	caps->min_vbuf_count = 1U;
	caps->buf_align = 1U;
	return 0;
}

static int htpa_proc_flush(const struct device *dev, bool cancel);

static int htpa_proc_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct htpa_proc_data *data = dev->data;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	data->streaming = enable;
	if (enable) {
		k_sem_give(&data->worker_sem);
	}
	k_mutex_unlock(&data->lock);
	return enable ? 0 : htpa_proc_flush(dev, true);
}

static int htpa_proc_enqueue(const struct device *dev, struct video_buffer *buffer)
{
	struct htpa_proc_data *data = dev->data;

	if (buffer->type != VIDEO_BUF_TYPE_OUTPUT || buffer->buffer == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (buffer->size < data->format.size) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}
	if (data->flushing) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
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

static int htpa_proc_dequeue(const struct device *dev, struct video_buffer **buffer,
			     k_timeout_t timeout)
{
	struct htpa_proc_data *data = dev->data;

	*buffer = k_fifo_get(&data->completed, timeout);
	return *buffer == NULL ? -EAGAIN : 0;
}

static int htpa_proc_flush(const struct device *dev, bool cancel)
{
	struct htpa_proc_data *data = dev->data;
	struct video_buffer *buffer;

	k_mutex_lock(&data->lock, K_FOREVER);
	while (data->flushing) {
		k_condvar_wait(&data->changed, &data->lock, K_FOREVER);
	}
	data->flushing = true;
	data->canceling = cancel;
	if (cancel) {
		if (data->in_flight != NULL) {
			atomic_set(&data->canceled, 1);
		}
		while ((buffer = k_fifo_get(&data->pending, K_NO_WAIT)) != NULL) {
			buffer->bytesused = 0U;
			k_fifo_put(&data->completed, buffer);
		}
	}
	k_sem_give(&data->worker_sem);
	while (data->in_flight != NULL || (!cancel && !k_fifo_is_empty(&data->pending)) ||
	       ((cancel || !data->streaming) && atomic_get(&data->source_active) != 0 &&
		atomic_get(&data->source_failed) == 0)) {
		k_condvar_wait(&data->changed, &data->lock, K_FOREVER);
	}
	data->canceling = false;
	data->flushing = false;
	k_condvar_broadcast(&data->changed);
	k_mutex_unlock(&data->lock);
	return atomic_get(&data->source_failed) != 0 ? -EIO : 0;
}

static int htpa_proc_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct htpa_proc_config *cfg = dev->config;

	return video_set_frmival(cfg->source, frmival);
}

static int htpa_proc_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct htpa_proc_config *cfg = dev->config;

	return video_get_frmival(cfg->source, frmival);
}

static int htpa_proc_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct htpa_proc_config *cfg = dev->config;
	struct htpa_proc_data *data = dev->data;
	struct video_frmival_enum source_fie = *fie;
	struct video_format source_fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	int ret;

	if ((fie->format->pixelformat != VIDEO_PIX_FMT_GREY &&
	     fie->format->pixelformat != VIDEO_PIX_FMT_HTPA_S16) ||
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

static DEVICE_API(video, htpa_proc_api) = {
	.set_format = htpa_proc_set_format,
	.get_format = htpa_proc_get_format,
	.set_frmival = htpa_proc_set_frmival,
	.get_frmival = htpa_proc_get_frmival,
	.enum_frmival = htpa_proc_enum_frmival,
	.get_caps = htpa_proc_get_caps,
	.set_stream = htpa_proc_set_stream,
	.enqueue = htpa_proc_enqueue,
	.dequeue = htpa_proc_dequeue,
	.flush = htpa_proc_flush,
};

static int htpa_proc_init(const struct device *dev)
{
	static const struct video_htpa_raw_layout layout_120x84 = VIDEO_HTPA_120X84_RAW_LAYOUT;
	static const struct video_htpa_raw_layout layout_160x120 = VIDEO_HTPA_160X120_RAW_LAYOUT;
	const struct htpa_proc_config *cfg = dev->config;
	struct htpa_proc_data *data = dev->data;
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	const struct video_htpa_raw_layout *layout;
	size_t index;
	int ret;

	if (!device_is_ready(cfg->source)) {
		return -ENODEV;
	}
	ret = video_get_format(cfg->source, &fmt);
	if (ret != 0) {
		return ret;
	}
	if (fmt.type != VIDEO_BUF_TYPE_OUTPUT || fmt.pixelformat != VIDEO_PIX_FMT_HTPA_RAW) {
		return -ENOTSUP;
	}
	ret = video_get_caps(cfg->source, &caps);
	if (ret != 0) {
		return ret;
	}
	if (caps.type != VIDEO_BUF_TYPE_OUTPUT || caps.format_caps == NULL ||
	    caps.min_vbuf_count > HTPA_RAW_BUFFERS || caps.buf_align > 4U ||
	    (caps.buf_align != 0U && 4U % caps.buf_align != 0U)) {
		return -ENOTSUP;
	}
	ret = video_format_caps_index(caps.format_caps, &fmt, &index);
	if (ret != 0) {
		return ret;
	}
	if (fmt.width == VIDEO_HTPA_160X120_WIDTH && fmt.height == VIDEO_HTPA_160X120_HEIGHT) {
		layout = &layout_160x120;
	} else if (fmt.width == VIDEO_HTPA_120X84_WIDTH && fmt.height == VIDEO_HTPA_120X84_HEIGHT) {
		layout = &layout_120x84;
	} else {
		return -ENOTSUP;
	}
	if (fmt.size > CONFIG_VIDEO_HTPA_PROCESSOR_RAW_BUFFER_SIZE || fmt.width == 0U ||
	    fmt.height == 0U || fmt.height % 2U != 0U || layout->block_count == 0U ||
	    layout->block_count > fmt.size || layout->block_length > fmt.size ||
	    layout->samples_per_block == 0U || layout->samples_per_block > fmt.size ||
	    (uint64_t)layout->data_offset + 2ULL * layout->samples_per_block >
		    layout->block_length ||
	    2ULL * layout->block_count * layout->samples_per_block !=
		    (uint64_t)fmt.width * fmt.height ||
	    2ULL * ((uint64_t)layout->block_count + 1U) * layout->block_length != fmt.size ||
	    (uint64_t)fmt.width * fmt.height > UINT32_MAX / sizeof(int16_t)) {
		return -ENOTSUP;
	}
	data->layout = *layout;
	data->format = (struct video_format) {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_HTPA_S16,
		.width = fmt.width,
		.height = fmt.height,
		.pitch = fmt.width * sizeof(int16_t),
		.size = fmt.width * fmt.height * sizeof(int16_t),
	};
	data->caps[0] = (struct video_format_cap) {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width_min = fmt.width,
		.width_max = fmt.width,
		.height_min = fmt.height,
		.height_max = fmt.height,
	};
	data->caps[1] = data->caps[0];
	data->caps[1].pixelformat = VIDEO_PIX_FMT_HTPA_S16;
	for (uint32_t i = 0U; i < HTPA_RAW_BUFFERS; i++) {
		data->raw[i].type = VIDEO_BUF_TYPE_OUTPUT;
		data->raw[i].buffer = data->raw_data[i];
		data->raw[i].size = fmt.size;
	}
	k_fifo_init(&data->pending);
	k_fifo_init(&data->completed);
	k_sem_init(&data->worker_sem, 0, 1);
	k_mutex_init(&data->lock);
	k_condvar_init(&data->changed);
	k_thread_create(&data->thread, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
			htpa_proc_worker, (void *)dev, NULL, NULL,
			CONFIG_VIDEO_HTPA_PROCESSOR_THREAD_PRIORITY, 0, K_NO_WAIT);
	return 0;
}

#define HTPA_PROC_DEFINE(inst)                                                                     \
	static struct htpa_proc_data htpa_proc_data_##inst;                                        \
	static const struct htpa_proc_config htpa_proc_cfg_##inst = {                              \
		.source = DEVICE_DT_GET(DT_INST_PHANDLE(inst, source)),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, htpa_proc_init, NULL, &htpa_proc_data_##inst,                  \
			      &htpa_proc_cfg_##inst, POST_KERNEL,                                  \
			      CONFIG_VIDEO_HTPA_PROCESSOR_INIT_PRIORITY, &htpa_proc_api);          \
	VIDEO_DEVICE_DEFINE(htpa_proc_##inst, DEVICE_DT_INST_GET(inst),                            \
			    DEVICE_DT_GET(DT_INST_PHANDLE(inst, source)));

DT_INST_FOREACH_STATUS_OKAY(HTPA_PROC_DEFINE)

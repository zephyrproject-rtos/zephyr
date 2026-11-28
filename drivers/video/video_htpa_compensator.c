/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT heimann_htpa_compensator

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/htpa_sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_htpa_compensator, CONFIG_VIDEO_LOG_LEVEL);

struct htpa_comp_config {
	const struct device *source;
};

struct htpa_comp_data {
	struct k_fifo pending;
	struct k_fifo completed;
	struct k_sem worker_sem;
	struct k_mutex lock;
	struct k_condvar changed;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_VIDEO_HTPA_COMPENSATOR_STACK_SIZE);
	uint32_t source_owned;
	uint32_t raw_size;
	uint8_t source_min_buffers;
	size_t source_alignment;
	bool source_active;
	struct video_format format;
	struct video_format_cap caps[3];
	int16_t pixels[VIDEO_HTPA_160X120_WIDTH * VIDEO_HTPA_160X120_HEIGHT];
	struct video_htpa_raw_layout layout;
	struct video_buffer *in_flight;
	bool streaming;
	bool flushing;
	bool canceling;
	bool source_failed;
};

static void htpa_comp_pixels(const struct htpa_comp_data *data, const uint8_t *raw, uint8_t *output)
{
	const struct video_htpa_raw_layout *layout = &data->layout;
	uint32_t width = data->format.width;
	uint32_t height = data->format.height;

	for (uint32_t half = 0U; half < 2U; half++) {
		uint32_t x = 0U;
		uint32_t y = half == 0U ? 0U : height - 1U;
		const uint8_t *offsets = raw + half * layout->block_length + layout->data_offset;
		const uint8_t *blocks =
			raw + (2U + half * layout->block_count) * layout->block_length;

		for (uint32_t block = 0U; block < layout->block_count; block++) {
			for (uint32_t i = 0U; i < layout->samples_per_block; i++) {
				uint16_t pixel =
					sys_get_be16(blocks + block * layout->block_length +
						     layout->data_offset + i * 2U);
				uint16_t offset = sys_get_be16(offsets + i * 2U);
				int32_t difference = (int32_t)pixel - offset;
				int16_t corrected =
					(int16_t)CLAMP(difference, INT16_MIN, INT16_MAX);

				memcpy(output + (y * width + x) * sizeof(corrected), &corrected,
				       sizeof(corrected));
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

/* Only the worker transfers ownership to and from the source. */
static int htpa_comp_source_stop(const struct device *dev)
{
	const struct htpa_comp_config *cfg = dev->config;
	struct htpa_comp_data *data = dev->data;
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

static void htpa_comp_worker(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	const struct htpa_comp_config *cfg = dev->config;
	struct htpa_comp_data *data = dev->data;

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
				ret = htpa_comp_source_stop(dev);
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
				if (htpa_comp_source_stop(dev) != 0) {
					data->source_failed = true;
				}
				LOG_ERR("Source start failed: %d", ret);
			} else {
				data->source_active = true;
			}
		}
		if (data->source_owned == 0U || !data->source_active || data->source_failed) {
			if (data->flushing && !data->streaming && data->source_active) {
				if (htpa_comp_source_stop(dev) != 0) {
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
			if (data->canceling || buffer->bytesused != data->raw_size) {
				buffer->bytesused = 0U;
			} else {
				k_mutex_unlock(&data->lock);
				htpa_comp_pixels(data, buffer->buffer, (uint8_t *)data->pixels);
				memcpy(buffer->buffer, data->pixels,
				       data->format.pitch * data->format.height);
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
			if (htpa_comp_source_stop(dev) != 0) {
				data->source_failed = true;
			}
		}
		k_condvar_broadcast(&data->changed);
		k_mutex_unlock(&data->lock);
	}
}

static int htpa_comp_get_format(const struct device *dev, struct video_format *fmt)
{
	struct htpa_comp_data *data = dev->data;

	*fmt = data->format;
	return 0;
}

static int htpa_comp_set_format(const struct device *dev, struct video_format *fmt)
{
	struct htpa_comp_data *data = dev->data;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT || fmt->pixelformat != VIDEO_PIX_FMT_HTPA_S16 ||
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
	data->format.pitch = fmt->width * sizeof(int16_t);
	data->format.size = data->raw_size;
	*fmt = data->format;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int htpa_comp_get_caps(const struct device *dev, struct video_caps *caps)
{
	struct htpa_comp_data *data = dev->data;

	caps->type = VIDEO_BUF_TYPE_OUTPUT;
	caps->format_caps = data->caps;
	caps->min_vbuf_count = data->source_min_buffers;
	caps->buf_align = data->source_alignment;
	return 0;
}

static int htpa_comp_flush(const struct device *dev, bool cancel);

static int htpa_comp_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct htpa_comp_data *data = dev->data;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	data->streaming = enable;
	if (enable) {
		k_sem_give(&data->worker_sem);
	}
	k_mutex_unlock(&data->lock);
	return enable ? 0 : htpa_comp_flush(dev, true);
}

static int htpa_comp_enqueue(const struct device *dev, struct video_buffer *buffer)
{
	struct htpa_comp_data *data = dev->data;

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

static int htpa_comp_dequeue(const struct device *dev, struct video_buffer **buffer,
			     k_timeout_t timeout)
{
	struct htpa_comp_data *data = dev->data;

	*buffer = k_fifo_get(&data->completed, timeout);
	return *buffer == NULL ? -EAGAIN : 0;
}

static int htpa_comp_flush(const struct device *dev, bool cancel)
{
	struct htpa_comp_data *data = dev->data;
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

static int htpa_comp_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct htpa_comp_config *cfg = dev->config;

	return video_set_frmival(cfg->source, frmival);
}

static int htpa_comp_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct htpa_comp_config *cfg = dev->config;

	return video_get_frmival(cfg->source, frmival);
}

static int htpa_comp_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct htpa_comp_config *cfg = dev->config;
	struct htpa_comp_data *data = dev->data;
	struct video_frmival_enum source_fie = *fie;
	struct video_format source_fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	int ret;

	if (fie->format->pixelformat != VIDEO_PIX_FMT_HTPA_S16 ||
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

static DEVICE_API(video, htpa_comp_api) = {
	.set_format = htpa_comp_set_format,
	.get_format = htpa_comp_get_format,
	.set_frmival = htpa_comp_set_frmival,
	.get_frmival = htpa_comp_get_frmival,
	.enum_frmival = htpa_comp_enum_frmival,
	.get_caps = htpa_comp_get_caps,
	.set_stream = htpa_comp_set_stream,
	.enqueue = htpa_comp_enqueue,
	.dequeue = htpa_comp_dequeue,
	.flush = htpa_comp_flush,
};

static int htpa_comp_init(const struct device *dev)
{
	static const struct video_htpa_raw_layout layout_120x84 = VIDEO_HTPA_120X84_RAW_LAYOUT;
	static const struct video_htpa_raw_layout layout_160x120 = VIDEO_HTPA_160X120_RAW_LAYOUT;
	const struct htpa_comp_config *cfg = dev->config;
	struct htpa_comp_data *data = dev->data;
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
	    caps.min_vbuf_count != 1U) {
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
	if (fmt.width == 0U || fmt.height == 0U || fmt.height % 2U != 0U ||
	    layout->block_count == 0U || layout->block_count > fmt.size ||
	    layout->block_length > fmt.size || layout->samples_per_block == 0U ||
	    layout->samples_per_block > fmt.size ||
	    (uint64_t)layout->data_offset + 2ULL * layout->samples_per_block >
		    layout->block_length ||
	    2ULL * layout->block_count * layout->samples_per_block !=
		    (uint64_t)fmt.width * fmt.height ||
	    2ULL * ((uint64_t)layout->block_count + 1U) * layout->block_length != fmt.size ||
	    (uint64_t)fmt.width * fmt.height > UINT32_MAX / sizeof(int16_t)) {
		return -ENOTSUP;
	}
	data->layout = *layout;
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
		.pixelformat = VIDEO_PIX_FMT_HTPA_S16,
		.width_min = fmt.width,
		.width_max = fmt.width,
		.height_min = fmt.height,
		.height_max = fmt.height,
	};
	k_fifo_init(&data->pending);
	k_fifo_init(&data->completed);
	k_sem_init(&data->worker_sem, 0, 1);
	k_mutex_init(&data->lock);
	k_condvar_init(&data->changed);
	k_thread_create(&data->thread, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
			htpa_comp_worker, (void *)dev, NULL, NULL,
			CONFIG_VIDEO_HTPA_COMPENSATOR_THREAD_PRIORITY, 0, K_NO_WAIT);
	return 0;
}

#define HTPA_COMP_DEFINE(inst)                                                                     \
	static struct htpa_comp_data htpa_comp_data_##inst;                                        \
	static const struct htpa_comp_config htpa_comp_cfg_##inst = {                              \
		.source = DEVICE_DT_GET(DT_INST_PHANDLE(inst, source)),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, htpa_comp_init, NULL, &htpa_comp_data_##inst,                  \
			      &htpa_comp_cfg_##inst, POST_KERNEL,                                  \
			      CONFIG_VIDEO_HTPA_COMPENSATOR_INIT_PRIORITY, &htpa_comp_api);        \
	VIDEO_DEVICE_DEFINE(htpa_comp_##inst, DEVICE_DT_INST_GET(inst),                            \
			    DEVICE_DT_GET(DT_INST_PHANDLE(inst, source)));

DT_INST_FOREACH_STATUS_OKAY(HTPA_COMP_DEFINE)

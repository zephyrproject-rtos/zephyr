/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Zephyr (top) half of the native simulator host FIFO video source.
 *
 * It exposes a host named pipe (FIFO) fed with raw frames as a regular Zephyr
 * video capture device. All the host C library accesses are delegated to the
 * bottom half, which is built in the native simulator runner context.
 */

#define DT_DRV_COMPAT zephyr_native_sim_video_fifo

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>

#include <nsi_errno.h>
#include <nsi_host_trampolines.h>

#include "cmdline.h"
#include "soc.h"
#include "video_common.h"
#include "video_native_sim_fifo_bottom.h"

LOG_MODULE_REGISTER(video_native_sim_fifo, CONFIG_VIDEO_LOG_LEVEL);

/* Any size up to 1920x1080, the width being a multiple of the pixels per macropixel */
#define VIDEO_NSI_FIFO_FORMAT_CAP(pixfmt, step)                                                    \
	{                                                                                          \
		.pixelformat = pixfmt,                                                             \
		.width_min = (step),                                                               \
		.width_max = 1920,                                                                 \
		.height_min = 1,                                                                   \
		.height_max = 1080,                                                                \
		.width_step = (step),                                                              \
		.height_step = 1,                                                                  \
	}

/* Uncompressed formats only: every frame is exactly pitch * height bytes */
static const struct video_format_cap video_nsi_fifo_fmts[] = {
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_RGB565, 1),
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_RGB24, 1),
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_BGR24, 1),
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_XRGB32, 1),
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_BGRX32, 1),
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_YUYV, 2),
	VIDEO_NSI_FIFO_FORMAT_CAP(VIDEO_PIX_FMT_GREY, 1),
	{0},
};

/* Format used until the application selects another one */
static const struct video_format video_nsi_fifo_default_fmt = {
	.type = VIDEO_BUF_TYPE_OUTPUT,
	.pixelformat = VIDEO_PIX_FMT_RGB565,
	.width = 320,
	.height = 240,
};

struct video_nsi_fifo_config {
	/** FIFO path coming from the devicetree, NULL when the property is not set */
	const char *fifo_path;
	/** Buffer for the default FIFO path, used when no path is given */
	char *default_path;
	size_t default_path_size;
	k_thread_stack_t *stack;
	size_t stack_size;
	/** Delay before polling the FIFO again when the host provided no complete frame */
	uint32_t poll_interval_ms;
};

struct video_nsi_fifo_data {
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_thread thread;
	/** Protects streaming, fmt, fd, frame_offset, discard and the head of fifo_in */
	struct k_mutex lock;
	/** Wakes the thread up when a buffer is queued or the stream is started */
	struct k_sem wake_sem;
	struct k_poll_signal *sig;
	/** FIFO path given on the command line, NULL when the option was not used */
	const char *fifo_path_arg;
	/** FIFO path in use, resolved at init */
	const char *path;
	/** Format of the frames the host writer is expected to provide */
	struct video_format fmt;
	/** Number of bytes of the frame currently being read */
	size_t frame_offset;
	int fd;
	bool streaming;
	/** The frame being read was cancelled by a flush, drop it once complete */
	bool discard;
	/** The driver created the FIFO, so it removes it on exit */
	bool created;
};

static void video_nsi_fifo_close(struct video_nsi_fifo_data *data)
{
	if (data->fd >= 0) {
		(void)nsi_host_close(data->fd);
		data->fd = -1;
	}

	data->frame_offset = 0;
	data->discard = false;
}

static void video_nsi_fifo_cleanup(struct video_nsi_fifo_data *data)
{
	if (data->created) {
		video_nsi_fifo_unlink_bottom(data->path);
	}
}

static int video_nsi_fifo_open(struct video_nsi_fifo_data *data)
{
	int fd;

	fd = video_nsi_fifo_open_bottom(data->path, &data->created);
	if (fd < 0) {
		return -nsi_errno_from_mid(-fd);
	}

	data->fd = fd;
	data->frame_offset = 0;
	data->discard = false;

	return 0;
}

static void video_nsi_fifo_deliver(struct video_nsi_fifo_data *data, struct video_buffer *vbuf)
{
	struct video_buffer *popped = k_fifo_get(&data->fifo_in, K_NO_WAIT);

	/* Only flush(true) takes buffers out of fifo_in, and it holds the lock */
	__ASSERT_NO_MSG(popped == vbuf);
	ARG_UNUSED(popped);

	vbuf->bytesused = data->fmt.size;
	vbuf->timestamp = k_uptime_get_32();
	vbuf->line_offset = 0;

	k_fifo_put(&data->fifo_out, vbuf);

	if (IS_ENABLED(CONFIG_POLL) && (data->sig != NULL)) {
		k_poll_signal_raise(data->sig, VIDEO_BUF_DONE);
	}
}

static void video_nsi_fifo_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct video_nsi_fifo_config *cfg = dev->config;
	struct video_nsi_fifo_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct video_buffer *vbuf;
		int ret;

		k_mutex_lock(&data->lock, K_FOREVER);

		/* The buffer stays queued until it is filled, so stop and flush never lose it */
		vbuf = k_fifo_peek_head(&data->fifo_in);
		if (!data->streaming || (vbuf == NULL)) {
			k_mutex_unlock(&data->lock);
			k_sem_take(&data->wake_sem, K_FOREVER);
			continue;
		}

		/* A larger format may have been set since the buffer was queued */
		if (vbuf->size < data->fmt.size) {
			LOG_ERR("Buffer too small: %u bytes provided, %u needed", vbuf->size,
				data->fmt.size);
			(void)k_fifo_get(&data->fifo_in, K_NO_WAIT);
			vbuf->bytesused = 0;
			/* As for a cancelled buffer, skip the rest of the frame it was receiving */
			data->discard = (data->frame_offset > 0);
			k_fifo_put(&data->fifo_out, vbuf);
			if (IS_ENABLED(CONFIG_POLL) && (data->sig != NULL)) {
				k_poll_signal_raise(data->sig, VIDEO_BUF_ERROR);
			}
			k_mutex_unlock(&data->lock);
			continue;
		}

		if (data->fd < 0) {
			ret = video_nsi_fifo_open(data);
			if (ret < 0) {
				k_mutex_unlock(&data->lock);
				k_sleep(K_MSEC(cfg->poll_interval_ms));
				continue;
			}
			LOG_DBG("Reopened the host FIFO %s", data->path);
		}

		ret = video_nsi_fifo_read_bottom(data->fd, vbuf->buffer, data->fmt.size,
						 &data->frame_offset);
		if (ret == 0) {
			data->frame_offset = 0;
			if (data->discard) {
				data->discard = false;
			} else {
				video_nsi_fifo_deliver(data, vbuf);
			}
			k_mutex_unlock(&data->lock);
			continue;
		}

		/* -EAGAIN: no more data for now, -EPIPE: all the writers closed the FIFO */
		ret = -nsi_errno_from_mid(-ret);
		if (ret == -EPIPE) {
			/*
			 * Drop the partial frame, if any, so that the next writer to attach is
			 * guaranteed to be frame aligned. The file descriptor is kept open: a
			 * new writer can attach to it.
			 */
			if (data->frame_offset > 0) {
				LOG_WRN("Incomplete frame discarded, is the host writing %ux%u %s "
					"frames?",
					data->fmt.width, data->fmt.height,
					VIDEO_FOURCC_TO_STR(data->fmt.pixelformat));
			}
			data->frame_offset = 0;
			data->discard = false;
		} else if (ret != -EAGAIN) {
			LOG_ERR("Error %d while reading the host FIFO, it will be reopened", ret);
			video_nsi_fifo_close(data);
		}

		k_mutex_unlock(&data->lock);

		/* Host reads must not block, or simulated time would stop: poll instead */
		k_sleep(K_MSEC(cfg->poll_interval_ms));
	}
}

static int video_nsi_fifo_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_nsi_fifo_data *data = dev->data;
	size_t idx;
	int ret;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	/* Scaling and pixel format conversion are up to the host writer (e.g. ffmpeg) */
	ret = video_format_caps_index(video_nsi_fifo_fmts, fmt, &idx);
	/* video_format_caps_index() ignores the steps, and YUYV needs an even width */
	if ((ret < 0) || ((fmt->width % video_nsi_fifo_fmts[idx].width_step) != 0)) {
		LOG_ERR("Unsupported format %s %ux%u", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
			fmt->width, fmt->height);
		return -ENOTSUP;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->fmt = *fmt;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int video_nsi_fifo_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct video_nsi_fifo_data *data = dev->data;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	*fmt = data->fmt;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int video_nsi_fifo_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (caps->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	caps->format_caps = video_nsi_fifo_fmts;
	caps->min_vbuf_count = 1;

	return 0;
}

static int video_nsi_fifo_set_stream(const struct device *dev, bool enable,
				     enum video_buf_type type)
{
	struct video_nsi_fifo_data *data = dev->data;
	int ret = 0;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (enable == data->streaming) {
		goto unlock;
	}

	if (!enable) {
		data->streaming = false;
		video_nsi_fifo_close(data);
		LOG_INF("Stream stopped");
		goto unlock;
	}

	ret = video_nsi_fifo_open(data);
	if (ret < 0) {
		goto unlock;
	}

	LOG_INF("Reading frames from the host FIFO %s", data->path);

	data->streaming = true;
	k_sem_give(&data->wake_sem);

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int video_nsi_fifo_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	struct video_nsi_fifo_data *data = dev->data;
	int ret = 0;

	if (vbuf->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (vbuf->size < data->fmt.size) {
		LOG_ERR("Buffer too small: %u bytes provided, %u needed", vbuf->size,
			data->fmt.size);
		ret = -EINVAL;
	} else {
		vbuf->bytesused = 0;
		k_fifo_put(&data->fifo_in, vbuf);
		k_sem_give(&data->wake_sem);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int video_nsi_fifo_dequeue(const struct device *dev, struct video_buffer **vbuf,
				  k_timeout_t timeout)
{
	struct video_nsi_fifo_data *data = dev->data;

	*vbuf = k_fifo_get(&data->fifo_out, timeout);
	if (*vbuf == NULL) {
		return -EAGAIN;
	}

	return 0;
}

static int video_nsi_fifo_flush(const struct device *dev, bool cancel)
{
	struct video_nsi_fifo_data *data = dev->data;
	struct video_buffer *vbuf;

	if (!cancel) {
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}
		return 0;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Resetting frame_offset would misalign every later frame, so skip the rest instead */
	data->discard = (data->frame_offset > 0);

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		k_fifo_put(&data->fifo_out, vbuf);
		if (IS_ENABLED(CONFIG_POLL) && (data->sig != NULL)) {
			k_poll_signal_raise(data->sig, VIDEO_BUF_ABORTED);
		}
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

#ifdef CONFIG_POLL
static int video_nsi_fifo_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	struct video_nsi_fifo_data *data = dev->data;

	if ((data->sig != NULL) && (sig != NULL)) {
		return -EALREADY;
	}

	data->sig = sig;

	return 0;
}
#endif

static DEVICE_API(video, video_nsi_fifo_driver_api) = {
	.set_format = video_nsi_fifo_set_fmt,
	.get_format = video_nsi_fifo_get_fmt,
	.get_caps = video_nsi_fifo_get_caps,
	.set_stream = video_nsi_fifo_set_stream,
	.enqueue = video_nsi_fifo_enqueue,
	.dequeue = video_nsi_fifo_dequeue,
	.flush = video_nsi_fifo_flush,
#ifdef CONFIG_POLL
	.set_signal = video_nsi_fifo_set_signal,
#endif
};

static int video_nsi_fifo_init(const struct device *dev)
{
	const struct video_nsi_fifo_config *cfg = dev->config;
	struct video_nsi_fifo_data *data = dev->data;
	int ret;

	data->fmt = video_nsi_fifo_default_fmt;
	ret = video_estimate_fmt_size(&data->fmt);
	if (ret < 0) {
		return ret;
	}

	/* The native simulator resets unset string options to NULL before parsing */
	if (data->fifo_path_arg != NULL) {
		data->path = data->fifo_path_arg;
	} else if (cfg->fifo_path != NULL) {
		data->path = cfg->fifo_path;
	} else {
		if (video_nsi_fifo_default_path_bottom(dev->name, cfg->default_path,
						       cfg->default_path_size) < 0) {
			return -ENAMETOOLONG;
		}
		data->path = cfg->default_path;
	}

	data->fd = -1;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_mutex_init(&data->lock);
	k_sem_init(&data->wake_sem, 0, 1);

	k_thread_create(&data->thread, cfg->stack, cfg->stack_size, video_nsi_fifo_thread,
			(void *)dev, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);
	(void)k_thread_name_set(&data->thread, dev->name);

	return 0;
}

/* "/tmp/zephyr-<device>-<pid>.fifo", 20 digits fit any pid */
#define VIDEO_NSI_FIFO_DEFAULT_PATH_SIZE(inst)                                                     \
	(sizeof("/tmp/zephyr--.fifo") + sizeof(DEVICE_DT_NAME(DT_DRV_INST(inst))) + 20)

#define VIDEO_NSI_FIFO_DEFINE(inst)                                                                \
	BUILD_ASSERT(DT_INST_PROP(inst, poll_interval_ms) > 0,                                     \
		     "poll-interval-ms must not be zero, it would busy loop the worker thread");   \
                                                                                                   \
	K_KERNEL_STACK_DEFINE(video_nsi_fifo_stack_##inst,                                         \
			      CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);                           \
                                                                                                   \
	static char video_nsi_fifo_default_path_##inst[VIDEO_NSI_FIFO_DEFAULT_PATH_SIZE(inst)];    \
                                                                                                   \
	static const struct video_nsi_fifo_config video_nsi_fifo_config_##inst = {                 \
		.fifo_path = DT_INST_PROP_OR(inst, fifo_path, NULL),                               \
		.default_path = video_nsi_fifo_default_path_##inst,                                \
		.default_path_size = sizeof(video_nsi_fifo_default_path_##inst),                   \
		.stack = video_nsi_fifo_stack_##inst,                                              \
		.stack_size = K_KERNEL_STACK_SIZEOF(video_nsi_fifo_stack_##inst),                  \
		.poll_interval_ms = DT_INST_PROP(inst, poll_interval_ms),                          \
	};                                                                                         \
                                                                                                   \
	static struct video_nsi_fifo_data video_nsi_fifo_data_##inst;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, video_nsi_fifo_init, NULL, &video_nsi_fifo_data_##inst,        \
			      &video_nsi_fifo_config_##inst, POST_KERNEL,                          \
			      CONFIG_VIDEO_INIT_PRIORITY, &video_nsi_fifo_driver_api);             \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(video_nsi_fifo_##inst, DEVICE_DT_INST_GET(inst), NULL);                \
                                                                                                   \
	static void video_nsi_fifo_cleanup_##inst(void)                                            \
	{                                                                                          \
		video_nsi_fifo_cleanup(&video_nsi_fifo_data_##inst);                               \
	}                                                                                          \
	NATIVE_TASK(video_nsi_fifo_cleanup_##inst, ON_EXIT, 1);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_NSI_FIFO_DEFINE)

#define VIDEO_NSI_FIFO_INST_NAME(inst) DEVICE_DT_NAME(DT_DRV_INST(inst))

#define VIDEO_NSI_FIFO_PATH_OPT(inst)                                                              \
	{                                                                                          \
		.option = VIDEO_NSI_FIFO_INST_NAME(inst),                                          \
		.name = "path",                                                                    \
		.type = 's',                                                                       \
		.dest = (void *)&video_nsi_fifo_data_##inst.fifo_path_arg,                         \
		.descript = "Path of the host FIFO to read the video frames from. "                \
			    "Overrides the fifo-path devicetree property",                         \
	},

static void video_nsi_fifo_options(void)
{
	static struct args_struct_t video_nsi_fifo_opts[] = {
		DT_INST_FOREACH_STATUS_OKAY(VIDEO_NSI_FIFO_PATH_OPT) ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(video_nsi_fifo_opts);
}

NATIVE_TASK(video_nsi_fifo_options, PRE_BOOT_1, 1);

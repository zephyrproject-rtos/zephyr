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

#include "cmdline.h"
#include "soc.h"
#include "video_common.h"
#include "video_native_sim_fifo_bottom.h"

LOG_MODULE_REGISTER(video_native_sim_fifo, CONFIG_VIDEO_LOG_LEVEL);

struct video_nsi_fifo_config {
	/** FIFO path coming from the devicetree, used unless overridden on the command line */
	const char *fifo_path;
	/** Staging buffer holding the frame being assembled, of frame_size bytes */
	uint8_t *staging;
	/** Size of a complete frame in bytes */
	uint32_t frame_size;
	/** Delay between two polls of the FIFO when no frame was completed */
	uint32_t poll_interval_ms;
	/** Fixed format of the frames the host writer is expected to provide */
	struct video_format fmt;
	/** Single entry (plus terminator) format capability list */
	const struct video_format_cap *format_caps;
};

struct video_nsi_fifo_data {
	const struct device *dev;
	struct k_fifo fifo_in;
	struct k_fifo fifo_out;
	struct k_work_delayable work;
	struct k_poll_signal *sig;
	/** FIFO path given on the command line, NULL when the option was not used */
	const char *fifo_path_arg;
	/** Number of bytes of the frame currently being assembled */
	size_t staging_offset;
	uint32_t frames_delivered;
	uint32_t frames_dropped;
	int fd;
};

static const char *video_nsi_fifo_path(const struct device *dev)
{
	const struct video_nsi_fifo_config *cfg = dev->config;
	struct video_nsi_fifo_data *data = dev->data;

	/* The native simulator resets unset string options to NULL before parsing */
	return (data->fifo_path_arg != NULL) ? data->fifo_path_arg : cfg->fifo_path;
}

static void video_nsi_fifo_close(struct video_nsi_fifo_data *data)
{
	if (data->fd >= 0) {
		(void)video_nsi_fifo_close_bottom(data->fd);
		data->fd = -1;
	}

	data->staging_offset = 0;
}

static int video_nsi_fifo_open(const struct device *dev)
{
	struct video_nsi_fifo_data *data = dev->data;
	const char *path = video_nsi_fifo_path(dev);
	int fd;

	fd = video_nsi_fifo_open_bottom(path);
	if (fd < 0) {
		return -nsi_errno_from_mid(-fd);
	}

	LOG_INF("Reading frames from the host FIFO %s", path);

	data->fd = fd;
	data->staging_offset = 0;

	return 0;
}

static void video_nsi_fifo_deliver(const struct device *dev)
{
	const struct video_nsi_fifo_config *cfg = dev->config;
	struct video_nsi_fifo_data *data = dev->data;
	struct video_buffer *vbuf;

	vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT);
	if (vbuf == NULL) {
		/*
		 * The application holds every buffer. Recycle the oldest frame it has not
		 * dequeued yet so that it is handed the freshest frames instead of a
		 * backlog of stale ones. The frame is consumed from the host FIFO either
		 * way, so that the pipe never builds up a backlog of its own.
		 */
		vbuf = k_fifo_get(&data->fifo_out, K_NO_WAIT);
		if (vbuf == NULL) {
			data->frames_dropped++;
			LOG_DBG("No buffer available, frame dropped (%u so far)",
				data->frames_dropped);
			return;
		}

		/*
		 * The frame this buffer held is overwritten below, so account for it as
		 * dropped. A buffer flushed by a previous stream holds no frame, hence
		 * the guard keeping the counters consistent.
		 */
		data->frames_dropped++;
		if (data->frames_delivered > 0) {
			data->frames_delivered--;
		}
	}

	memcpy(vbuf->buffer, cfg->staging, cfg->frame_size);
	vbuf->bytesused = cfg->frame_size;
	vbuf->timestamp = k_uptime_get_32();
	vbuf->line_offset = 0;
	data->frames_delivered++;

	k_fifo_put(&data->fifo_out, vbuf);

	if (IS_ENABLED(CONFIG_POLL) && (data->sig != NULL)) {
		k_poll_signal_raise(data->sig, VIDEO_BUF_DONE);
	}
}

static void video_nsi_fifo_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct video_nsi_fifo_data *data = CONTAINER_OF(dwork, struct video_nsi_fifo_data, work);
	const struct device *dev = data->dev;
	const struct video_nsi_fifo_config *cfg = dev->config;
	int ret;

	if (data->fd < 0) {
		/* A previous error closed the FIFO, or it could not be opened yet */
		if (video_nsi_fifo_open(dev) < 0) {
			goto reschedule;
		}
	}

	ret = video_nsi_fifo_read_bottom(data->fd, cfg->staging, cfg->frame_size,
					 &data->staging_offset);
	if (ret < 0) {
		LOG_ERR("Error %d while reading the host FIFO, it will be reopened",
			nsi_errno_from_mid(-ret));
		video_nsi_fifo_close(data);
		goto reschedule;
	}

	switch (ret) {
	case VIDEO_NSI_FIFO_FRAME:
		video_nsi_fifo_deliver(dev);
		if (!k_fifo_is_empty(&data->fifo_in)) {
			/*
			 * A buffer is still free and more data may already be pending: poll
			 * again without waiting so that the application always gets the
			 * freshest frame the host produced.
			 */
			k_work_reschedule(&data->work, K_NO_WAIT);
			return;
		}
		/*
		 * Every buffer has been filled. The system workqueue runs at a cooperative
		 * priority, so polling again without waiting would starve the preemptible
		 * application threads and prevent them from ever giving a buffer back.
		 */
		break;
	case VIDEO_NSI_FIFO_WRITER_GONE_PARTIAL:
		LOG_WRN("The host writer disconnected mid-frame, incomplete frame discarded");
		break;
	case VIDEO_NSI_FIFO_WRITER_GONE:
	case VIDEO_NSI_FIFO_NO_FRAME:
	default:
		break;
	}

reschedule:
	k_work_reschedule(&data->work, K_MSEC(cfg->poll_interval_ms));
}

static int video_nsi_fifo_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_nsi_fifo_config *cfg = dev->config;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	/*
	 * The host writer decides the format: scaling and pixel format conversion are
	 * done by the host feeder (ffmpeg), not by the simulated device.
	 */
	if ((fmt->pixelformat != cfg->fmt.pixelformat) || (fmt->width != cfg->fmt.width) ||
	    (fmt->height != cfg->fmt.height)) {
		LOG_ERR("Only %ux%u %s is supported, adjust the host writer instead",
			cfg->fmt.width, cfg->fmt.height, VIDEO_FOURCC_TO_STR(cfg->fmt.pixelformat));
		return -ENOTSUP;
	}

	fmt->pitch = cfg->fmt.pitch;
	fmt->size = cfg->fmt.size;

	return 0;
}

static int video_nsi_fifo_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct video_nsi_fifo_config *cfg = dev->config;

	if (fmt->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	*fmt = cfg->fmt;

	return 0;
}

static int video_nsi_fifo_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_nsi_fifo_config *cfg = dev->config;

	if (caps->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	caps->format_caps = cfg->format_caps;
	caps->min_vbuf_count = 1;

	return 0;
}

static int video_nsi_fifo_set_stream(const struct device *dev, bool enable,
				     enum video_buf_type type)
{
	const struct video_nsi_fifo_config *cfg = dev->config;
	struct video_nsi_fifo_data *data = dev->data;
	struct k_work_sync work_sync = {0};
	int ret;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	if (!enable) {
		(void)k_work_cancel_delayable_sync(&data->work, &work_sync);
		LOG_INF("Stream stopped: %u frames delivered, %u frames dropped",
			data->frames_delivered, data->frames_dropped);
		video_nsi_fifo_close(data);
		return 0;
	}

	data->frames_delivered = 0;
	data->frames_dropped = 0;

	if (data->fd < 0) {
		ret = video_nsi_fifo_open(dev);
		if (ret < 0) {
			return ret;
		}
	}

	k_work_reschedule(&data->work, K_MSEC(cfg->poll_interval_ms));

	return 0;
}

static int video_nsi_fifo_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	const struct video_nsi_fifo_config *cfg = dev->config;
	struct video_nsi_fifo_data *data = dev->data;

	if (vbuf->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	if (vbuf->size < cfg->frame_size) {
		LOG_ERR("Buffer too small: %u bytes provided, %u needed", vbuf->size,
			cfg->frame_size);
		return -EINVAL;
	}

	vbuf->bytesused = 0;
	k_fifo_put(&data->fifo_in, vbuf);

	return 0;
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

	/*
	 * Only the poll work item can fill the pending buffers, and it only runs while
	 * the stream is started. Waiting for them on a stopped stream would block
	 * forever, so the buffers are cancelled instead.
	 */
	if (!cancel && (data->fd < 0)) {
		LOG_DBG("Stream not started, cancelling the pending buffers instead");
		cancel = true;
	}

	if (!cancel) {
		/* Wait for all the buffers to be filled with a frame from the host */
		while (!k_fifo_is_empty(&data->fifo_in)) {
			k_sleep(K_MSEC(1));
		}

		return 0;
	}

	while ((vbuf = k_fifo_get(&data->fifo_in, K_NO_WAIT)) != NULL) {
		k_fifo_put(&data->fifo_out, vbuf);
		if (IS_ENABLED(CONFIG_POLL) && (data->sig != NULL)) {
			k_poll_signal_raise(data->sig, VIDEO_BUF_ABORTED);
		}
	}

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
	unsigned int bits_per_pixel = video_bits_per_pixel(cfg->fmt.pixelformat);

	if ((bits_per_pixel == 0) ||
	    (cfg->fmt.width * cfg->fmt.height * bits_per_pixel / BITS_PER_BYTE !=
	     cfg->frame_size)) {
		LOG_ERR("Unsupported pixel format %s for a raw frame video source",
			VIDEO_FOURCC_TO_STR(cfg->fmt.pixelformat));
		return -EINVAL;
	}

	data->dev = dev;
	data->fd = -1;

	k_fifo_init(&data->fifo_in);
	k_fifo_init(&data->fifo_out);
	k_work_init_delayable(&data->work, video_nsi_fifo_work);

	return 0;
}

/* Bytes per pixel of every pixel format the binding accepts */
#define VIDEO_NSI_FIFO_BYTES_PER_PIXEL_RGB565 2
#define VIDEO_NSI_FIFO_BYTES_PER_PIXEL_YUYV   2
#define VIDEO_NSI_FIFO_BYTES_PER_PIXEL_GREY   1

/* Whether a pixel format packs several columns in a single macropixel */
#define VIDEO_NSI_FIFO_EVEN_WIDTH_RGB565 0
#define VIDEO_NSI_FIFO_EVEN_WIDTH_YUYV   1
#define VIDEO_NSI_FIFO_EVEN_WIDTH_GREY   0

#define VIDEO_NSI_FIFO_PIXFMT(inst)                                                                \
	UTIL_CAT(VIDEO_PIX_FMT_, DT_INST_STRING_UPPER_TOKEN(inst, pixel_format))

#define VIDEO_NSI_FIFO_BPP(inst)                                                                   \
	UTIL_CAT(VIDEO_NSI_FIFO_BYTES_PER_PIXEL_, DT_INST_STRING_UPPER_TOKEN(inst, pixel_format))

#define VIDEO_NSI_FIFO_EVEN_WIDTH(inst)                                                            \
	UTIL_CAT(VIDEO_NSI_FIFO_EVEN_WIDTH_, DT_INST_STRING_UPPER_TOKEN(inst, pixel_format))

#define VIDEO_NSI_FIFO_PITCH(inst) (DT_INST_PROP(inst, width) * VIDEO_NSI_FIFO_BPP(inst))

#define VIDEO_NSI_FIFO_FRAME_SIZE(inst) (VIDEO_NSI_FIFO_PITCH(inst) * DT_INST_PROP(inst, height))

#define VIDEO_NSI_FIFO_DEFINE(inst)                                                                \
	BUILD_ASSERT(DT_INST_PROP(inst, width) > 0, "width must not be zero");                     \
	BUILD_ASSERT(DT_INST_PROP(inst, height) > 0, "height must not be zero");                   \
	BUILD_ASSERT(DT_INST_PROP(inst, poll_interval_ms) > 0,                                     \
		     "poll-interval-ms must not be zero, it would busy loop the workqueue");       \
	BUILD_ASSERT(!VIDEO_NSI_FIFO_EVEN_WIDTH(inst) || (DT_INST_PROP(inst, width) % 2 == 0),     \
		     "this pixel format packs two columns per macropixel, width must be even");    \
                                                                                                   \
	static uint8_t video_nsi_fifo_staging_##inst[VIDEO_NSI_FIFO_FRAME_SIZE(inst)];             \
                                                                                                   \
	static const struct video_format_cap video_nsi_fifo_caps_##inst[] = {                      \
		{                                                                                  \
			.pixelformat = VIDEO_NSI_FIFO_PIXFMT(inst),                                \
			.width_min = DT_INST_PROP(inst, width),                                    \
			.width_max = DT_INST_PROP(inst, width),                                    \
			.height_min = DT_INST_PROP(inst, height),                                  \
			.height_max = DT_INST_PROP(inst, height),                                  \
			.width_step = 1,                                                           \
			.height_step = 1,                                                          \
		},                                                                                 \
		{0},                                                                               \
	};                                                                                         \
                                                                                                   \
	static const struct video_nsi_fifo_config video_nsi_fifo_config_##inst = {                 \
		.fifo_path = DT_INST_PROP(inst, fifo_path),                                        \
		.staging = video_nsi_fifo_staging_##inst,                                          \
		.frame_size = VIDEO_NSI_FIFO_FRAME_SIZE(inst),                                     \
		.poll_interval_ms = DT_INST_PROP(inst, poll_interval_ms),                          \
		.fmt =                                                                             \
			{                                                                          \
				.type = VIDEO_BUF_TYPE_OUTPUT,                                     \
				.pixelformat = VIDEO_NSI_FIFO_PIXFMT(inst),                        \
				.width = DT_INST_PROP(inst, width),                                \
				.height = DT_INST_PROP(inst, height),                              \
				.pitch = VIDEO_NSI_FIFO_PITCH(inst),                               \
				.size = VIDEO_NSI_FIFO_FRAME_SIZE(inst),                           \
			},                                                                         \
		.format_caps = video_nsi_fifo_caps_##inst,                                         \
	};                                                                                         \
                                                                                                   \
	static struct video_nsi_fifo_data video_nsi_fifo_data_##inst;                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, video_nsi_fifo_init, NULL, &video_nsi_fifo_data_##inst,        \
			      &video_nsi_fifo_config_##inst, POST_KERNEL,                          \
			      CONFIG_VIDEO_INIT_PRIORITY, &video_nsi_fifo_driver_api);             \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(video_nsi_fifo_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(VIDEO_NSI_FIFO_DEFINE)

#define VIDEO_NSI_FIFO_INST_NAME(inst) DEVICE_DT_NAME(DT_DRV_INST(inst))

#define VIDEO_NSI_FIFO_COMMAND_LINE_OPTS(inst)                                                     \
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
		DT_INST_FOREACH_STATUS_OKAY(VIDEO_NSI_FIFO_COMMAND_LINE_OPTS) ARG_TABLE_ENDMARKER,
	};

	native_add_command_line_opts(video_nsi_fifo_opts);
}

NATIVE_TASK(video_nsi_fifo_options, PRE_BOOT_1, 1);

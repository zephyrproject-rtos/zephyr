/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/htpa_sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

#define BLOCK_SIZE 1606U
#define RAW_SIZE   (26U * BLOCK_SIZE)
#define OUT_SIZE   (160U * 120U * 2U)

struct source_data {
	struct k_fifo pending;
	struct k_fifo completed;
	bool streaming;
	uint32_t start_count;
	uint32_t stop_count;
	uint32_t fill_count;
	uint32_t fail_enqueue_countdown;
	struct k_sem started;
	bool hold;
	bool short_frame;
	bool fail_start;
	bool constant_frame;
	bool narrow_frame;
	bool extreme_frame;
};

struct source_config {
	const struct video_format_cap *caps;
	const struct video_htpa_raw_layout *layout;
	uint32_t raw_size;
};

static const struct video_htpa_raw_layout source_layout = {
	.block_count = 12U,
	.block_length = BLOCK_SIZE,
	.data_offset = 6U,
	.samples_per_block = 800U,
};

static const struct video_format_cap source_caps[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_HTPA_RAW,
		.width_min = 160U,
		.width_max = 160U,
		.height_min = 120U,
		.height_max = 120U,
	},
	{0},
};

static const struct video_htpa_raw_layout layout_120 = VIDEO_HTPA_120X84_RAW_LAYOUT;
static const struct video_format_cap caps_120[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_HTPA_RAW,
		.width_min = VIDEO_HTPA_120X84_WIDTH,
		.width_max = VIDEO_HTPA_120X84_WIDTH,
		.height_min = VIDEO_HTPA_120X84_HEIGHT,
		.height_max = VIDEO_HTPA_120X84_HEIGHT,
	},
	{0},
};

static const struct video_format_cap small_caps[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_HTPA_RAW,
		.width_min = 4U,
		.width_max = 4U,
		.height_min = 4U,
		.height_max = 4U,
	},
	{0},
};

static int source_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct source_config *cfg = dev->config;

	caps->type = VIDEO_BUF_TYPE_OUTPUT;
	caps->format_caps = cfg->caps;
	caps->min_vbuf_count = 1U;
	caps->buf_align = 4U;
	return 0;
}

static int source_format(const struct device *dev, struct video_format *fmt)
{
	const struct source_config *cfg = dev->config;

	*fmt = (struct video_format) {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_HTPA_RAW,
		.width = cfg->caps[0].width_min,
		.height = cfg->caps[0].height_min,
		.size = cfg->raw_size,
	};
	return 0;
}

static void source_fill(const struct device *dev, struct video_buffer *buf)
{
	const struct source_config *cfg = dev->config;
	const struct video_htpa_raw_layout *layout = cfg->layout;
	struct source_data *data = dev->data;

	memset(buf->buffer, 0, cfg->raw_size);
	for (uint32_t half = 0U; half < 2U; half++) {
		for (uint32_t i = 0U; i < layout->samples_per_block; i++) {
			uint16_t offset = data->extreme_frame ?
				(i % 2U == 0U ? 0U : UINT16_MAX) :
				(uint16_t)(30000U + i % 31U + 20U);

			if (data->extreme_frame && i < 2U) {
				offset = i == 0U ? 32760U : 32780U;
			}

			sys_put_be16(offset,
				     buf->buffer + half * layout->block_length +
					     layout->data_offset + 2U * i);
			for (uint32_t block = 0U; block < layout->block_count; block++) {
				int16_t value = (int16_t)(block * layout->samples_per_block +
							  i - 4000);

				if (data->narrow_frame) {
					value = (int16_t)(i % 2U == 0U ? 0 : 100);
				}
				uint16_t sample = (uint16_t)((data->constant_frame ? 42 : value) +
							    (int32_t)offset);

				if (data->extreme_frame) {
					sample = i % 2U == 0U ? UINT16_MAX : 0U;
					if (i < 2U) {
						sample = i == 0U ? 32780U : 32760U;
					}
				}
				sys_put_be16(sample, buf->buffer +
					     (2U + half * layout->block_count + block) *
						     layout->block_length +
					     layout->data_offset + 2U * i);
			}
		}
	}
	data->fill_count++;
	buf->bytesused = data->short_frame ? cfg->raw_size - 1U : cfg->raw_size;
	buf->timestamp = 123U;
	k_fifo_put(&data->completed, buf);
}

static void source_run(const struct device *dev)
{
	struct source_data *data = dev->data;
	struct video_buffer *buf;

	if (!data->streaming || data->hold) {
		return;
	}
	while ((buf = k_fifo_get(&data->pending, K_NO_WAIT)) != NULL) {
		source_fill(dev, buf);
	}
}

static int source_enqueue(const struct device *dev, struct video_buffer *buf)
{
	struct source_data *data = dev->data;

	if (data->fail_enqueue_countdown > 0U) {
		data->fail_enqueue_countdown--;
		if (data->fail_enqueue_countdown == 0U) {
			return -EIO;
		}
	}
	k_fifo_put(&data->pending, buf);
	source_run(dev);
	return 0;
}

static int source_dequeue(const struct device *dev, struct video_buffer **buf, k_timeout_t timeout)
{
	struct source_data *data = dev->data;

	*buf = k_fifo_get(&data->completed, timeout);
	return *buf == NULL ? -EAGAIN : 0;
}

static int source_flush(const struct device *dev, bool cancel)
{
	struct source_data *data = dev->data;
	struct video_buffer *buf;

	ARG_UNUSED(cancel);
	while ((buf = k_fifo_get(&data->pending, K_NO_WAIT)) != NULL) {
		buf->bytesused = 0U;
		k_fifo_put(&data->completed, buf);
	}
	return 0;
}

static int source_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct source_data *data = dev->data;

	ARG_UNUSED(type);
	if (!enable) {
		data->streaming = false;
		data->stop_count++;
		return 0;
	}
	data->start_count++;
	k_sem_give(&data->started);
	if (data->fail_start) {
		return -EIO;
	}
	data->streaming = true;
	source_run(dev);
	return 0;
}

static int source_init(const struct device *dev)
{
	struct source_data *data = dev->data;

	k_fifo_init(&data->pending);
	k_fifo_init(&data->completed);
	k_sem_init(&data->started, 0, 1);
	return 0;
}

static int source_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	ARG_UNUSED(dev);
	*frmival = (struct video_frmival){.numerator = 1U, .denominator = 7U};
	return 0;
}

static int source_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	if (frmival->denominator == 0U ||
	    video_frmival_nsec(frmival) < NSEC_PER_SEC / 7U - 100U ||
	    video_frmival_nsec(frmival) > NSEC_PER_SEC / 7U + 100U) {
		return -ENOTSUP;
	}
	return source_get_frmival(dev, frmival);
}

static int source_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (fie->index != 0U || fie->format->pixelformat != VIDEO_PIX_FMT_HTPA_RAW) {
		return -EINVAL;
	}
	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	return source_get_frmival(dev, &fie->discrete);
}

static DEVICE_API(video, source_api) = {
	.get_format = source_format,
	.get_frmival = source_get_frmival,
	.set_frmival = source_set_frmival,
	.enum_frmival = source_enum_frmival,
	.get_caps = source_get_caps,
	.enqueue = source_enqueue,
	.dequeue = source_dequeue,
	.flush = source_flush,
	.set_stream = source_stream,
};
static struct source_data source_state;
static const struct source_config source_cfg = {
	.caps = source_caps, .layout = &source_layout, .raw_size = RAW_SIZE,
};
DEVICE_DT_DEFINE(DT_NODELABEL(htpa_test_source), source_init, NULL, &source_state, &source_cfg,
		 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &source_api);
static struct source_data state_120;
static const struct source_config cfg_120 = {
	.caps = caps_120,
	.layout = &layout_120,
	.raw_size = 2U * (VIDEO_HTPA_120X84_BLOCK_COUNT + 1U) * VIDEO_HTPA_120X84_BLOCK_LENGTH,
};
DEVICE_DT_DEFINE(DT_NODELABEL(htpa_120_source), source_init, NULL, &state_120, &cfg_120,
		 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &source_api);
static struct source_data small_state;
static const struct source_config small_cfg = {.caps = small_caps, .raw_size = 60U};
DEVICE_DT_DEFINE(DT_NODELABEL(htpa_small_source), source_init, NULL, &small_state, &small_cfg,
		 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &source_api);
static struct source_data bad_state;
static const struct source_config bad_cfg = {.caps = source_caps, .raw_size = 60U};
DEVICE_DT_DEFINE(DT_NODELABEL(htpa_bad_source), source_init, NULL, &bad_state, &bad_cfg,
		 POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &source_api);

static const struct device *const processor = DEVICE_DT_GET(DT_NODELABEL(htpa_test_processor));

ZTEST(htpa_processor, test_frame_interval)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_frmival_enum fie = {.format = &fmt};
	struct video_frmival interval = {.numerator = 1428571U, .denominator = 10000000U};

	zassert_ok(video_get_format(processor, &fmt));
	fmt.pixelformat = VIDEO_PIX_FMT_GREY;
	zassert_ok(video_enum_frmival(processor, &fie));
	zassert_equal(fie.type, VIDEO_FRMIVAL_TYPE_DISCRETE);
	zassert_equal(fie.discrete.numerator, 1U);
	zassert_equal(fie.discrete.denominator, 7U);
	fie.index = 1U;
	zassert_equal(video_enum_frmival(processor, &fie), -EINVAL);
	zassert_ok(video_set_frmival(processor, &interval));
	zassert_equal(interval.denominator, 7U);
	zassert_ok(video_get_frmival(processor, &interval));
	zassert_equal(interval.denominator, 7U);
	interval = (struct video_frmival){.numerator = 1U, .denominator = 30U};
	zassert_equal(video_set_frmival(processor, &interval), -ENOTSUP);
}

ZTEST(htpa_processor, test_unsupported_source)
{
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(htpa_small_processor))));
}

ZTEST(htpa_processor, test_invalid_raw_size)
{
	zassert_false(device_is_ready(DEVICE_DT_GET(DT_NODELABEL(htpa_bad_processor))));
}

ZTEST(htpa_processor, test_120x84_frame)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(htpa_120_processor));
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_buffer *out = NULL;
	struct video_buffer *buf;

	zassert_true(device_is_ready(dev));
	zassert_ok(video_get_format(dev, &fmt));
	zassert_equal(fmt.width, 120U);
	zassert_equal(fmt.height, 84U);
	zassert_equal(fmt.size, 120U * 84U * 2U);
	zassert_ok(video_get_caps(dev, &caps));
	zassert_equal(caps.format_caps[0].width_min, fmt.width);
	zassert_equal(caps.format_caps[0].height_min, fmt.height);
	buf = video_buffer_alloc(fmt.size, K_NO_WAIT);
	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(dev, buf));
	zassert_ok(video_stream_start(dev, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(dev, &out, K_SECONDS(3)));
	zassert_equal_ptr(out, buf);
	zassert_equal(out->bytesused, fmt.size);
	for (uint32_t y = 0U; y < fmt.height; y++) {
		for (uint32_t x = 0U; x < fmt.width; x++) {
			uint32_t row = y < 42U ? y : 83U - y;
			int16_t actual;

			memcpy(&actual, out->buffer + 2U * (y * fmt.width + x), sizeof(actual));
			zassert_equal(actual, (int16_t)(row * fmt.width + x - 4000),
				      "x=%u y=%u", x, y);
		}
	}
	zassert_ok(video_stream_stop(dev, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_frame_and_format)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_buffer *buf;
	struct video_buffer *out = NULL;

	zassert_true(device_is_ready(processor));
	zassert_ok(video_get_format(processor, &fmt));
	zassert_equal(fmt.pixelformat, VIDEO_PIX_FMT_HTPA_S16);
	zassert_equal(fmt.pitch, fmt.width * sizeof(int16_t));
	zassert_equal(fmt.size, OUT_SIZE);
	zassert_ok(video_get_caps(processor, &caps));
	zassert_equal(caps.format_caps[0].width_min, fmt.width);
	zassert_equal(caps.format_caps[0].height_min, fmt.height);
	zassert_equal(caps.format_caps[0].pixelformat, VIDEO_PIX_FMT_GREY);
	zassert_equal(caps.format_caps[1].pixelformat, VIDEO_PIX_FMT_HTPA_S16);
	zassert_ok(video_set_format(processor, &fmt));
	zassert_equal(fmt.pitch, fmt.width * sizeof(int16_t));
	buf = video_buffer_alloc(fmt.size, K_NO_WAIT);
	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal_ptr(out, buf);
	zassert_equal(out->bytesused, OUT_SIZE);
	zassert_equal(out->timestamp, 123U);
	for (uint32_t y = 0U; y < 120U; y++) {
		for (uint32_t x = 0U; x < 160U; x++) {
			uint32_t row = y < 60U ? y : 119U - y;
			int16_t expected = (int16_t)(row * 160U + x - 4000);
			int16_t actual;

			memcpy(&actual, out->buffer + 2U * (y * 160U + x), sizeof(actual));
			zassert_equal(actual, expected, "x=%u y=%u", x, y);
		}
	}
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_compensation_saturates)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_buffer *out = NULL;
	struct video_buffer *buf;
	int16_t actual;

	zassert_ok(video_get_format(processor, &fmt));
	fmt.pixelformat = VIDEO_PIX_FMT_HTPA_S16;
	zassert_ok(video_set_format(processor, &fmt));
	buf = video_buffer_alloc(fmt.size, K_NO_WAIT);
	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	source_state.extreme_frame = true;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	memcpy(&actual, out->buffer, sizeof(actual));
	zassert_equal(actual, 20);
	memcpy(&actual, out->buffer + sizeof(actual), sizeof(actual));
	zassert_equal(actual, -20);
	memcpy(&actual, out->buffer + 2U * sizeof(actual), sizeof(actual));
	zassert_equal(actual, INT16_MAX);
	memcpy(&actual, out->buffer + 3U * sizeof(actual), sizeof(actual));
	zassert_equal(actual, INT16_MIN);
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	source_state.extreme_frame = false;
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_grey_histogram)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_buffer *out = NULL;
	struct video_buffer *buf;

	zassert_ok(video_get_format(processor, &fmt));
	zassert_ok(video_get_caps(processor, &caps));
	zassert_equal(caps.format_caps[0].pixelformat, VIDEO_PIX_FMT_GREY);
	fmt.pixelformat = VIDEO_PIX_FMT_GREY;
	zassert_ok(video_set_format(processor, &fmt));
	zassert_equal(fmt.pitch, fmt.width);
	zassert_ok(video_get_format(processor, &fmt));
	zassert_equal(fmt.pitch, fmt.width);
	zassert_equal(fmt.size, 160U * 120U);
	buf = video_buffer_alloc(fmt.size, K_NO_WAIT);
	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_equal(video_set_format(processor, &fmt), -EBUSY);
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->bytesused, fmt.size);
	zassert_equal(out->timestamp, 123U);
	/* The retained range is samples 76 through 9523 after percentile clipping. */
	zassert_equal(out->buffer[0], 0U);
	zassert_equal(out->buffer[159], 2U);
	zassert_equal(out->buffer[30U * 160U], 127U);
	zassert_equal(out->buffer[59U * 160U + 159U], 255U);
	zassert_equal(out->buffer[119U * 160U + 159U], 2U);

	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	source_state.constant_frame = true;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	for (uint32_t i = 0U; i < fmt.size; i++) {
		zassert_equal(out->buffer[i], 127U);
	}
	source_state.constant_frame = false;
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	source_state.narrow_frame = true;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->buffer[0], (uint8_t)(127.5f - 12750.0f /
						 CONFIG_VIDEO_HTPA_PROCESSOR_MIN_GREY_RANGE));
	zassert_equal(out->buffer[1], (uint8_t)(127.5f + 12750.0f /
						 CONFIG_VIDEO_HTPA_PROCESSOR_MIN_GREY_RANGE));
	source_state.narrow_frame = false;
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_buffer_release(buf));
	fmt.pixelformat = VIDEO_PIX_FMT_HTPA_S16;
	zassert_ok(video_set_format(processor, &fmt));
}

ZTEST(htpa_processor, test_continuous_source)
{
	struct video_buffer *buf = video_buffer_alloc(OUT_SIZE, K_NO_WAIT);
	struct video_buffer *out = NULL;
	uint32_t starts = source_state.start_count;
	uint32_t stops = source_state.stop_count;
	uint32_t fills = source_state.fill_count;

	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	for (uint32_t i = 0U; i < 3U; i++) {
		zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
		zassert_equal_ptr(out, buf);
		zassert_equal(out->bytesused, OUT_SIZE);
		if (i < 2U) {
			zassert_ok(video_enqueue(processor, buf));
		}
	}
	zassert_equal(source_state.start_count, starts + 1U);
	zassert_equal(source_state.stop_count, stops);
	zassert_true(source_state.fill_count >=
		     fills + CONFIG_VIDEO_HTPA_PROCESSOR_RAW_BUFFER_COUNT + 3U);
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_equal(source_state.stop_count, stops + 1U);
	zassert_true(k_fifo_is_empty(&source_state.pending));
	zassert_true(k_fifo_is_empty(&source_state.completed));
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_cancel_and_restart)
{
	struct video_buffer *buf = video_buffer_alloc(OUT_SIZE, K_NO_WAIT);
	struct video_buffer *out = NULL;

	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	k_sem_reset(&source_state.started);
	source_state.hold = true;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(k_sem_take(&source_state.started, K_SECONDS(3)));
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal_ptr(out, buf);
	zassert_equal(out->bytesused, 0U);
	source_state.hold = false;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->bytesused, OUT_SIZE);
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_partial_source_enqueue_failure)
{
	struct video_buffer *buf = video_buffer_alloc(OUT_SIZE, K_NO_WAIT);
	struct video_buffer *out = NULL;

	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	source_state.fail_enqueue_countdown = CONFIG_VIDEO_HTPA_PROCESSOR_RAW_BUFFER_COUNT;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->bytesused, 0U);
	zassert_true(k_fifo_is_empty(&source_state.pending));
	zassert_true(k_fifo_is_empty(&source_state.completed));
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->bytesused, OUT_SIZE);
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_short_frame)
{
	struct video_buffer *buf = video_buffer_alloc(OUT_SIZE, K_NO_WAIT);
	struct video_buffer *out = NULL;

	zassert_not_null(buf);
	buf->type = VIDEO_BUF_TYPE_OUTPUT;
	source_state.short_frame = true;
	zassert_ok(video_enqueue(processor, buf));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->bytesused, 0U);
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	source_state.short_frame = false;
	zassert_ok(video_buffer_release(buf));
}

ZTEST(htpa_processor, test_flush_and_start_failure)
{
	struct video_buffer *first = video_buffer_alloc(OUT_SIZE, K_NO_WAIT);
	struct video_buffer *second = video_buffer_alloc(OUT_SIZE, K_NO_WAIT);
	struct video_buffer *out = NULL;

	zassert_not_null(first);
	zassert_not_null(second);
	first->type = VIDEO_BUF_TYPE_OUTPUT;
	second->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(processor, first));
	zassert_ok(video_enqueue(processor, second));
	zassert_ok(video_driver_flush(processor, false));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal_ptr(out, first);
	zassert_equal(out->bytesused, OUT_SIZE);
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal_ptr(out, second);
	zassert_equal(out->bytesused, OUT_SIZE);

	source_state.fail_start = true;
	zassert_ok(video_enqueue(processor, first));
	zassert_ok(video_stream_start(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal_ptr(out, first);
	zassert_equal(out->bytesused, 0U);
	source_state.fail_start = false;
	zassert_ok(video_enqueue(processor, first));
	zassert_ok(video_dequeue(processor, &out, K_SECONDS(3)));
	zassert_equal(out->bytesused, OUT_SIZE);
	zassert_ok(video_stream_stop(processor, VIDEO_BUF_TYPE_OUTPUT));
	zassert_ok(video_buffer_release(first));
	zassert_ok(video_buffer_release(second));
}

ZTEST_SUITE(htpa_processor, NULL, NULL, NULL, NULL, NULL);

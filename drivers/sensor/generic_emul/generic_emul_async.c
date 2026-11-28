/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/util.h>

#include "generic_emul.h"

/* The shift applies to every frame of the channel */
struct generic_emul_rtio_channel {
	uint16_t chan_type;
	int8_t shift;
};

struct generic_emul_rtio_header {
	uint64_t timestamp_ns;
	uint32_t period_ns;
	uint16_t frames;
	uint8_t num_channels;
	bool streamed;
	struct generic_emul_rtio_channel channels[];
};

static size_t buf_size(uint8_t num_channels, uint16_t frames)
{
	return sizeof(struct generic_emul_rtio_header) +
	       num_channels * sizeof(struct generic_emul_rtio_channel) +
	       (size_t)frames * num_channels * sizeof(q31_t);
}

static const q31_t *buf_samples(const struct generic_emul_rtio_header *h)
{
	return (const void *)&h->channels[h->num_channels];
}

/* Fails only with an external sensor clock whose counter cannot be read */
static int get_timestamp_ns(uint64_t *ns)
{
	uint64_t cycles;
	int ret;

	ret = sensor_clock_get_cycles(&cycles);
	if (ret != 0) {
		return ret;
	}

	*ns = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

static bool channel_supported(enum sensor_channel chan)
{
	return chan < SENSOR_CHAN_ALL && !SENSOR_CHANNEL_3_AXIS(chan) &&
	       chan != SENSOR_CHAN_PROX && chan != SENSOR_CHAN_GAUGE_CYCLE_COUNT;
}

static int find_channel(const struct generic_emul_rtio_header *h, enum sensor_channel chan)
{
	for (int i = 0; i < h->num_channels; i++) {
		if (h->channels[i].chan_type == chan) {
			return i;
		}
	}

	return -ENOTSUP;
}

static int find_spec(const struct generic_emul_rtio_header *h, struct sensor_chan_spec spec)
{
	int x;

	if (spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (!SENSOR_CHANNEL_3_AXIS(spec.chan_type)) {
		return channel_supported(spec.chan_type) ? find_channel(h, spec.chan_type)
							 : -ENOTSUP;
	}

	/*
	 * A three-axis channel is stored as its X, Y and Z channels in three consecutive entries:
	 * find X and check that Y and Z follow it.
	 */
	x = find_channel(h, generic_emul_axis_channel(spec.chan_type, 0));
	if (x < 0 || x + 2 >= h->num_channels) {
		return -ENOTSUP;
	}

	for (int i = 1; i < 3; i++) {
		if (h->channels[x + i].chan_type != generic_emul_axis_channel(spec.chan_type, i)) {
			return -ENOTSUP;
		}
	}

	return x;
}

static int decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec spec,
				   uint16_t *frame_count)
{
	const struct generic_emul_rtio_header *h = (const void *)buffer;
	int ret = find_spec(h, spec);

	if (ret < 0) {
		return ret;
	}

	*frame_count = h->frames;

	return 0;
}

static int decoder_get_size_info(struct sensor_chan_spec spec, size_t *base_size,
				 size_t *frame_size)
{
	if (!SENSOR_CHANNEL_3_AXIS(spec.chan_type) && !channel_supported(spec.chan_type)) {
		return -ENOTSUP;
	}

	return sensor_natively_supported_channel_size_info(spec, base_size, frame_size);
}

static int8_t spec_shift(const struct generic_emul_rtio_header *h, int first, int axes)
{
	int8_t shift = h->channels[first].shift;

	for (int i = 1; i < axes; i++) {
		shift = MAX(shift, h->channels[first + i].shift);
	}

	return shift;
}

/* A batch ends at a timestamp delta beyond 32 bits */
static int decoder_decode(const uint8_t *buffer, struct sensor_chan_spec spec, uint32_t *fit,
			  uint16_t max_count, void *data_out)
{
	const struct generic_emul_rtio_header *h = (const void *)buffer;
	const q31_t *samples = buf_samples(h);
	struct sensor_three_axis_data *axis_out = data_out;
	struct sensor_q31_data *q31_out = data_out;
	int axes = SENSOR_CHANNEL_3_AXIS(spec.chan_type) ? 3 : 1;
	int first = find_spec(h, spec);
	uint64_t base_ns;
	int8_t shift;
	int count;

	if (first < 0) {
		return first;
	}

	if (max_count == 0) {
		return -EINVAL;
	}

	if (*fit >= h->frames) {
		return 0;
	}

	base_ns = h->timestamp_ns + (uint64_t)*fit * h->period_ns;
	shift = spec_shift(h, first, axes);

	for (count = 0; count < max_count && *fit + count < h->frames; count++) {
		const q31_t *s = &samples[(*fit + count) * h->num_channels + first];
		uint64_t delta = (uint64_t)count * h->period_ns;

		if (delta > UINT32_MAX) {
			break;
		}

		if (axes == 1) {
			q31_out->readings[count].timestamp_delta = delta;
			q31_out->readings[count].value = s[0];
			continue;
		}

		axis_out->readings[count].timestamp_delta = delta;
		for (int i = 0; i < 3; i++) {
			int8_t diff = (int8_t)MIN(shift - h->channels[first + i].shift, 31);

			axis_out->readings[count].values[i] = s[i] >> diff;
		}
	}

	if (axes == 1) {
		q31_out->header.base_timestamp_ns = base_ns;
		q31_out->header.reading_count = count;
		q31_out->shift = shift;
	} else {
		axis_out->header.base_timestamp_ns = base_ns;
		axis_out->header.reading_count = count;
		axis_out->shift = shift;
	}

	*fit += count;

	return count;
}

static bool decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct generic_emul_rtio_header *h = (const void *)buffer;

	return h->streamed && trigger == SENSOR_TRIG_FIFO_WATERMARK;
}

static const struct sensor_decoder_api generic_emul_decoder = {
	.get_frame_count = decoder_get_frame_count,
	.get_size_info = decoder_get_size_info,
	.decode = decoder_decode,
	.has_trigger = decoder_has_trigger,
};

int generic_emul_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &generic_emul_decoder;

	return 0;
}

static uint8_t count_channels(const atomic_t *channels)
{
	uint8_t count = 0;

	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		if (channel_supported(i) && atomic_test_bit(channels, i)) {
			count++;
		}
	}

	return count;
}

/* In channel order, so the axes of a three-axis channel are consecutive */
static void add_channels(struct generic_emul_rtio_header *h, const atomic_t *channels)
{
	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		if (channel_supported(i) && atomic_test_bit(channels, i)) {
			h->channels[h->num_channels++].chan_type = (uint16_t)i;
		}
	}
}

/* Each frame starts as the set values; a streamed frame then comes from the sample source */
static int fill_frames(const struct device *dev, struct generic_emul_rtio_header *h, uint32_t base)
{
	struct generic_emul_data *data = dev->data;
	struct generic_emul_stream *stream = data->stream;
	const struct generic_emul_cfg *cfg = dev->config;
	uint8_t n = h->num_channels;
	q31_t *s = (void *)&h->channels[n];
	generic_emul_sample_fn sample_fn;
	void *user_data;
	int ret;

	K_SPINLOCK(&data->lock) {
		sample_fn = h->streamed ? stream->sample_fn : NULL;
		user_data = stream->sample_user_data;

		for (int c = 0; c < n; c++) {
			h->channels[c].shift = stream->shift[h->channels[c].chan_type];
			s[c] = stream->q31[h->channels[c].chan_type];
		}
	}

	for (uint16_t f = 1; f < h->frames; f++) {
		memcpy(&s[f * n], s, n * sizeof(q31_t));
	}

	if (sample_fn == NULL) {
		return 0;
	}

	for (uint16_t f = 0; f < h->frames; f++) {
		for (int c = 0; c < n; c++) {
			struct sensor_chan_spec ch = {h->channels[c].chan_type, 0};

			ret = sample_fn(cfg->emul, ch, base + f, &s[f * n + c], user_data);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static void submit_oneshot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	struct generic_emul_data *data = dev->data;
	ATOMIC_DEFINE(channels, SENSOR_CHAN_ALL) = {0};
	struct generic_emul_rtio_header *h;
	uint8_t num_channels;
	uint32_t buf_len;
	uint8_t *buf;
	int ret;

	if (data->fetch_rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, data->fetch_rc);
		return;
	}

	/* Unset channels are left out, so decoding them fails */
	K_SPINLOCK(&data->lock) {
		for (size_t i = 0; i < read_cfg->count; i++) {
			enum sensor_channel type = read_cfg->channels[i].chan_type;
			int axes = SENSOR_CHANNEL_3_AXIS(type) ? 3 : 1;
			bool set = read_cfg->channels[i].chan_idx == 0;

			for (int a = 0; set && a < axes; a++) {
				enum sensor_channel chan = generic_emul_axis_channel(type, a);

				set = channel_supported(chan) &&
				      atomic_test_bit(data->channels_set, chan);
			}

			for (int a = 0; set && a < axes; a++) {
				atomic_set_bit(channels, generic_emul_axis_channel(type, a));
			}
		}
	}

	num_channels = count_channels(channels);

	ret = rtio_sqe_rx_buf(iodev_sqe, buf_size(num_channels, 1), buf_size(num_channels, 1),
			      &buf, &buf_len);
	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	h = (void *)buf;
	*h = (struct generic_emul_rtio_header){.frames = 1};
	add_channels(h, channels);

	ret = get_timestamp_ns(&h->timestamp_ns);
	if (ret == 0) {
		ret = fill_frames(dev, h, 0);
	}

	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/* Called with the data lock held, once no read is pending */
static void stream_stop_locked(struct generic_emul_stream *stream)
{
	k_timer_stop(&stream->timer);
	stream->active = false;
}

/* Serves one read from the FIFO. Only the timer drains the FIFO, so it is not re-checked. */
static void stream_serve(struct generic_emul_stream *stream, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct device *dev = stream->dev;
	struct generic_emul_data *data = dev->data;
	const struct generic_emul_cfg *cfg = dev->config;
	ATOMIC_DEFINE(channels_set, SENSOR_CHAN_ALL);
	struct generic_emul_rtio_header *h = NULL;
	uint32_t base = 0, buf_len, min_len;
	uint16_t frames = 0, fit = 0;
	bool multishot, mempool;
	uint8_t num_channels;
	uint8_t *buf;
	int ret;

	/* One snapshot, so the buffer is sized for the channels written to it */
	K_SPINLOCK(&data->lock) {
		frames = stream->fifo;
		base = stream->seq;
		for (size_t w = 0; w < ARRAY_SIZE(channels_set); w++) {
			atomic_set(&channels_set[w], atomic_get(&data->channels_set[w]));
		}
	}

	/* Read before completion, which frees a plain read */
	multishot = (iodev_sqe->sqe.flags & RTIO_SQE_MULTISHOT) != 0;
	mempool = (iodev_sqe->sqe.flags & RTIO_SQE_MEMPOOL_BUFFER) != 0;

	num_channels = count_channels(channels_set);

	/* A caller's buffer need only hold a watermark; what does not fit stays in the FIFO */
	min_len = buf_size(num_channels, mempool ? frames : cfg->stream->fifo_watermark);
	ret = rtio_sqe_rx_buf(iodev_sqe, min_len, buf_size(num_channels, frames), &buf, &buf_len);
	if (ret == 0) {
		fit = frames;
		if (num_channels > 0) {
			fit = (uint16_t)MIN(frames, (buf_len - buf_size(num_channels, 0)) /
							    (num_channels * sizeof(q31_t)));
		}

		h = (void *)buf;
		*h = (struct generic_emul_rtio_header){
			.period_ns = k_ticks_to_ns_near32(cfg->stream->period_ticks),
			.frames = fit,
			.streamed = true,
		};
		add_channels(h, channels_set);

		ret = get_timestamp_ns(&h->timestamp_ns);
	}

	if (ret == 0) {
		/* The newest sample in the FIFO is taken now */
		h->timestamp_ns -=
			(uint64_t)(frames - 1) * k_ticks_to_ns_near64(cfg->stream->period_ticks);
		ret = fill_frames(dev, h, base);
	}

	/* An error ends the read and loses its samples, but keeps the session */
	K_SPINLOCK(&data->lock) {
		if (ret != 0) {
			fit = frames;
			stream->overruns += frames;
		}
		stream->fifo -= fit;
		stream->seq += fit;
	}

	if (ret != 0) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	/* A multishot read canceled during completion is not re-submitted */
	if (multishot) {
		K_SPINLOCK(&data->lock) {
			if (stream->pending == 0) {
				stream_stop_locked(stream);
			}
		}
	}
}

/* Serves pending reads, oldest first, while the FIFO holds a watermark */
static void stream_deliver(struct generic_emul_stream *stream)
{
	const struct generic_emul_cfg *cfg = stream->dev->config;
	struct generic_emul_data *data = stream->dev->data;
	struct rtio_iodev_sqe *iodev_sqe;
	bool canceled;

	do {
		iodev_sqe = NULL;
		canceled = false;

		K_SPINLOCK(&data->lock) {
			if (stream->pending == 0 || stream->fifo < cfg->stream->fifo_watermark) {
				K_SPINLOCK_BREAK;
			}

			/* Dequeued first: a completed multishot read re-submits behind others */
			iodev_sqe =
				CONTAINER_OF(mpsc_pop(&stream->reads), struct rtio_iodev_sqe, q);
			stream->pending--;

			/* The FIFO is left for the next read */
			canceled = rtio_iodev_sqe_is_canceled(iodev_sqe);
			if (canceled && stream->pending == 0) {
				stream_stop_locked(stream);
			}
		}

		if (canceled) {
			rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
		} else if (iodev_sqe != NULL) {
			stream_serve(stream, iodev_sqe);
		}
	} while (iodev_sqe != NULL);
}

/* Each tick is a watermark interrupt, adding fifo_watermark samples */
static void stream_tick(struct k_timer *timer)
{
	struct generic_emul_stream *stream = k_timer_user_data_get(timer);
	const struct generic_emul_cfg *cfg = stream->dev->config;
	struct generic_emul_data *data = stream->dev->data;
	uint16_t watermark = cfg->stream->fifo_watermark;

	K_SPINLOCK(&data->lock) {
		uint16_t room = cfg->stream->fifo_depth - stream->fifo;

		if (watermark <= room) {
			stream->fifo += watermark;
		} else {
			/* Full: the oldest samples are lost */
			stream->fifo = cfg->stream->fifo_depth;
			stream->seq += watermark - room;
			stream->overruns += watermark - room;
		}
	}

	stream_deliver(stream);
}

/* Multishot and plain reads queue alike, and are served at the next watermark in order */
static void submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;
	const struct generic_emul_cfg *cfg = dev->config;
	struct generic_emul_data *data = dev->data;
	struct generic_emul_stream *stream = data->stream;

	for (size_t i = 0; i < read_cfg->count; i++) {
		if (read_cfg->triggers[i].trigger != SENSOR_TRIG_FIFO_WATERMARK) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	K_SPINLOCK(&data->lock) {
		mpsc_push(&stream->reads, &iodev_sqe->q);
		stream->pending++;

		/* A stream that ended with an error resumes where it left off */
		if (!stream->active) {
			k_timeout_t interval = K_TICKS(cfg->stream->period_ticks *
						       cfg->stream->fifo_watermark);

			stream->active = true;
			stream->seq = 0;
			stream->fifo = 0;
			stream->overruns = 0;
			k_timer_start(&stream->timer, interval, interval);
		}
	}
}

void generic_emul_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;

	if (read_cfg->is_streaming) {
		submit_stream(dev, iodev_sqe);
	} else {
		submit_oneshot(dev, iodev_sqe);
	}
}

/* The stream is not found through the SQE, whose sensor may be a wrapper */
void generic_emul_stream_init(const struct device *dev)
{
	struct generic_emul_data *data = dev->data;
	struct generic_emul_stream *stream = data->stream;

	stream->dev = dev;
	mpsc_init(&stream->reads);
	k_timer_init(&stream->timer, stream_tick, NULL);
	k_timer_user_data_set(&stream->timer, stream);
}

void generic_emul_set_sample_source(const struct emul *target, generic_emul_sample_fn fn,
				    void *user_data)
{
	struct generic_emul_data *data = target->data;

	__ASSERT_NO_MSG(data->stream != NULL);

	K_SPINLOCK(&data->lock) {
		data->stream->sample_fn = fn;
		data->stream->sample_user_data = user_data;
	}
}

uint32_t generic_emul_stream_overruns(const struct emul *target)
{
	struct generic_emul_data *data = target->data;
	uint32_t overruns = 0;

	__ASSERT_NO_MSG(data->stream != NULL);

	K_SPINLOCK(&data->lock) {
		overruns = data->stream->overruns;
	}

	return overruns;
}

/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/generic_emul.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#define STREAM_NODE DT_NODELABEL(stream_sensor)
#define WATERMARK   DT_PROP(STREAM_NODE, fifo_watermark)
#define DEPTH       DT_PROP(STREAM_NODE, fifo_depth)
#define PERIOD_MS   (MSEC_PER_SEC / DT_PROP(STREAM_NODE, sampling_frequency))
#define SHIFT       16

/* Decoder output for up to n frames, extending the trailing readings[1] */
#define THREE_AXIS_DATA(n)                                                                         \
	union {                                                                                    \
		struct sensor_three_axis_data data;                                                \
		uint8_t size[sizeof(struct sensor_three_axis_data) +                               \
			     ((n) - 1) * sizeof(struct sensor_three_axis_sample_data)];            \
	}

static const struct device *dev = DEVICE_DT_GET(STREAM_NODE);
static const struct emul *emul = EMUL_DT_GET(STREAM_NODE);

/* Axes requested before, and again within, their three-axis channel */
SENSOR_DT_READ_IODEV(oneshot_iodev, STREAM_NODE, {SENSOR_CHAN_ACCEL_Y, 0}, {SENSOR_CHAN_ACCEL_X, 0},
		     {SENSOR_CHAN_ACCEL_XYZ, 0}, {SENSOR_CHAN_DIE_TEMP, 0});
SENSOR_DT_STREAM_IODEV(stream_iodev, STREAM_NODE,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_STREAM_IODEV(drdy_iodev, STREAM_NODE,
		       {SENSOR_TRIG_DATA_READY, SENSOR_STREAM_DATA_INCLUDE});
RTIO_DEFINE(oneshot_rtio, 1, 1);
RTIO_DEFINE_WITH_MEMPOOL(stream_rtio, 4, 4, 4, 128, 4);

static rtio_sqe_handle_t handle, second;

/* Userdata telling two streams' completions apart */
static int tags[2];

/*
 * A plain read's buffer for n frames of the three accel axes, as the driver lays it out: a
 * 16-byte header, 4 bytes per channel and 4 bytes per sample
 */
#define READ_BUF_SIZE(n) (16 + 3 * 4 + (n) * 3 * 4)

/* Sample n of axis a reads n * 16 + a, in whole units at the shift set_xyz() sets */
static int ramp(const struct emul *target, struct sensor_chan_spec ch, uint32_t seq, q31_t *value,
		void *user_data)
{
	ARG_UNUSED(target);
	ARG_UNUSED(user_data);

	*value = ((int64_t)(seq * 16 + (ch.chan_type - SENSOR_CHAN_ACCEL_X)) << 31) >> SHIFT;

	return 0;
}

static void set_xyz(void)
{
	struct sensor_chan_spec chan = {SENSOR_CHAN_ACCEL_X, 0};
	q31_t sample;

	for (int i = 0; i < 3; i++) {
		chan.chan_type = SENSOR_CHAN_ACCEL_X + i;
		sample = ((int64_t)(i + 1) << 31) >> SHIFT;
		zassert_ok(emul_sensor_backend_set_channel(emul, chan, &sample, SHIFT));
	}
}

static int64_t whole(q31_t value, int8_t shift)
{
	return ((int64_t)value << shift) >> 31;
}

/* Consume one stream completion, returning its buffer and userdata */
static int consume_tagged(uint8_t **buf, uint32_t *len, void **userdata)
{
	struct rtio_cqe *cqe = rtio_cqe_consume_block(&stream_rtio);
	int res = cqe->result;

	*userdata = cqe->userdata;
	if (res == 0) {
		zassert_ok(rtio_cqe_get_mempool_buffer(&stream_rtio, cqe, buf, len));
	}
	rtio_cqe_release(&stream_rtio, cqe);

	return res;
}

/* Consume one stream completion, returning its buffer */
static int consume(uint8_t **buf, uint32_t *len)
{
	void *userdata;

	return consume_tagged(buf, len, &userdata);
}

/* Queue a plain read into @p buf, which is also its userdata */
static void submit_read(const struct rtio_iodev *iodev, uint8_t *buf, uint32_t len)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&stream_rtio);

	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM, buf, len, buf);
	zassert_ok(rtio_submit(&stream_rtio, 0));
}

/* Consume the completion of a plain read, returning its buffer */
static int consume_read(uint8_t **buf)
{
	struct rtio_cqe *cqe = rtio_cqe_consume_block(&stream_rtio);
	int res = cqe->result;

	*buf = cqe->userdata;
	rtio_cqe_release(&stream_rtio, cqe);

	return res;
}

/* The ramp index of the first frame of @p buf */
static uint32_t first_seq(const uint8_t *buf)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_q31_data data;
	uint32_t fit = 0;

	zassert_ok(sensor_get_decoder(dev, &decoder));
	zassert_equal(1, decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ACCEL_X, 0},
					 &fit, 1, &data));

	return whole(data.readings[0].value, data.shift) / 16;
}

/* Check every frame of @p buf continues the ramp from @p next */
static uint32_t check_ramp(const uint8_t *buf, uint32_t next)
{
	THREE_AXIS_DATA(DEPTH) out;
	const struct sensor_decoder_api *decoder;
	struct sensor_chan_spec xyz = {SENSOR_CHAN_ACCEL_XYZ, 0};
	uint16_t frames;
	uint32_t fit = 0;
	int count;

	zassert_ok(sensor_get_decoder(dev, &decoder));
	zassert_true(decoder->has_trigger(buf, SENSOR_TRIG_FIFO_WATERMARK));
	zassert_ok(decoder->get_frame_count(buf, xyz, &frames));
	zassert_true(frames >= WATERMARK, "%u frames", frames);

	count = decoder->decode(buf, xyz, &fit, DEPTH, &out.data);
	zassert_equal(count, frames);
	zassert_equal(out.data.header.reading_count, frames);

	for (int f = 0; f < frames; f++, next++) {
		for (int a = 0; a < 3; a++) {
			zassert_equal(whole(out.data.readings[f].values[a], out.data.shift),
				      next * 16 + a, "frame %d axis %d", f, a);
		}
	}

	zassert_equal(decoder->decode(buf, xyz, &fit, DEPTH, &out.data), 0, "past the last frame");

	return next;
}

ZTEST(drivers_sensor_generic_emul_stream, test_oneshot_read)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_three_axis_data xyz;
	struct sensor_q31_data q31;
	uint8_t buf[128];
	uint32_t fit = 0;

	set_xyz();
	zassert_ok(sensor_read(&oneshot_iodev, &oneshot_rtio, buf, sizeof(buf)));
	zassert_ok(sensor_get_decoder(dev, &decoder));
	zassert_false(decoder->has_trigger(buf, SENSOR_TRIG_FIFO_WATERMARK));

	zassert_equal(1, decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ACCEL_XYZ, 0},
					 &fit, 1, &xyz));
	for (int a = 0; a < 3; a++) {
		zassert_equal(whole(xyz.readings[0].values[a], xyz.shift), a + 1);
	}

	fit = 0;
	zassert_equal(1, decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ACCEL_Y, 0},
					 &fit, 1, &q31));
	zassert_equal(whole(q31.readings[0].value, q31.shift), 2);

	/* Requested but never set */
	fit = 0;
	zassert_equal(-ENOTSUP, decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_DIE_TEMP,
										0},
						&fit, 1, &q31));
}

ZTEST(drivers_sensor_generic_emul_stream, test_oneshot_fetch_rc)
{
	uint8_t buf[128];

	set_xyz();
	generic_emul_func_rc(emul, 0, 0, -EIO);
	zassert_equal(-EIO, sensor_read(&oneshot_iodev, &oneshot_rtio, buf, sizeof(buf)));
}

ZTEST(drivers_sensor_generic_emul_stream, test_stream_ramp)
{
	uint32_t next = 0, len;
	uint8_t *buf;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));

	for (int i = 0; i < 4; i++) {
		zassert_ok(consume(&buf, &len));
		next = check_ramp(buf, next);
		rtio_release_buffer(&stream_rtio, buf, len);
	}

	zassert_equal(generic_emul_stream_overruns(emul), 0);
}

ZTEST(drivers_sensor_generic_emul_stream, test_stream_set_values)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_q31_data data;
	uint32_t fit = 0, len;
	uint8_t *buf;

	set_xyz();
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));
	zassert_ok(consume(&buf, &len));

	zassert_ok(sensor_get_decoder(dev, &decoder));
	zassert_equal(1, decoder->decode(buf, (struct sensor_chan_spec){SENSOR_CHAN_ACCEL_Z, 0},
					 &fit, 1, &data));
	zassert_equal(whole(data.readings[0].value, data.shift), 3);
	rtio_release_buffer(&stream_rtio, buf, len);
}

/* Consume @p n completions alternating between the tagged streams, starting with @p first */
static uint32_t check_alternating(int n, int first, uint32_t next)
{
	void *userdata;
	uint32_t len;
	uint8_t *buf;

	for (int i = 0; i < n; i++) {
		zassert_ok(consume_tagged(&buf, &len, &userdata));
		zassert_equal_ptr(userdata, &tags[(first + i) % 2], "completion %d", i);
		next = check_ramp(buf, next);
		rtio_release_buffer(&stream_rtio, buf, len);
	}

	return next;
}

ZTEST(drivers_sensor_generic_emul_stream, test_two_streams_alternate)
{
	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, &tags[0], &handle));
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, &tags[1], &second));

	check_alternating(4, 0, 0);
	zassert_equal(generic_emul_stream_overruns(emul), 0);
}

ZTEST(drivers_sensor_generic_emul_stream, test_cancel_one_of_two_streams)
{
	uint32_t next, len;
	void *userdata;
	int canceled = 0;
	uint8_t *buf;
	int res;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, &tags[0], &handle));
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, &tags[1], &second));
	next = check_alternating(1, 0, 0);

	/*
	 * The cancel completes when the first is re-submitted or dequeued, either before or
	 * after the second's next completion. It loses no samples, and only the second is left.
	 */
	zassert_ok(rtio_sqe_cancel(&stream_rtio, handle));
	for (int i = 0; i < 3; i++) {
		res = consume_tagged(&buf, &len, &userdata);
		if (res == -ECANCELED) {
			zassert_equal_ptr(userdata, &tags[0]);
			canceled++;
			continue;
		}

		zassert_ok(res);
		zassert_equal_ptr(userdata, &tags[1], "completion %d", i);
		next = check_ramp(buf, next);
		rtio_release_buffer(&stream_rtio, buf, len);
	}

	zassert_equal(canceled, 1);
}

/* As the sensing subsystem restarts a stream */
ZTEST(drivers_sensor_generic_emul_stream, test_cancel_then_restart)
{
	uint32_t next, len;
	void *userdata;
	uint8_t *buf;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, &tags[0], &handle));
	next = check_alternating(1, 0, 0);

	zassert_ok(rtio_sqe_cancel(&stream_rtio, handle));
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, &tags[1], &second));
	zassert_equal(-ECANCELED, consume_tagged(&buf, &len, &userdata));
	zassert_equal_ptr(userdata, &tags[0]);

	/* The restart queues before the cancel completes, so the session and the ramp go on */
	next = check_alternating(1, 1, next);
	check_alternating(1, 1, next);
}

/* Two pre-allocated buffers read in one submission */
ZTEST(drivers_sensor_generic_emul_stream, test_reads_into_static_buffers)
{
	static uint8_t bufs[2][READ_BUF_SIZE(DEPTH)] __aligned(8);
	uint32_t next = 0;
	uint8_t *buf;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	for (int i = 0; i < 2; i++) {
		struct rtio_sqe *sqe = rtio_sqe_acquire(&stream_rtio);

		zassert_not_null(sqe);
		rtio_sqe_prep_read(sqe, &stream_iodev, RTIO_PRIO_NORM, bufs[i], sizeof(bufs[i]),
				   bufs[i]);
	}
	zassert_ok(rtio_submit(&stream_rtio, 0));

	for (int i = 0; i < 2; i++) {
		zassert_ok(consume_read(&buf));
		zassert_equal_ptr(buf, bufs[i], "completion %d", i);
		next = check_ramp(buf, next);
	}

	/* A buffer handed back continues the ramp */
	submit_read(&stream_iodev, bufs[0], sizeof(bufs[0]));
	zassert_ok(consume_read(&buf));
	zassert_equal_ptr(buf, bufs[0]);
	check_ramp(buf, next);
	zassert_equal(generic_emul_stream_overruns(emul), 0);
}

ZTEST(drivers_sensor_generic_emul_stream, test_static_buffer_too_small)
{
	static uint8_t small[READ_BUF_SIZE(WATERMARK) - 1] __aligned(8);
	uint8_t *buf;

	set_xyz();
	submit_read(&stream_iodev, small, sizeof(small));
	zassert_equal(-ENOMEM, consume_read(&buf));
	zassert_equal(generic_emul_stream_overruns(emul), WATERMARK);
}

ZTEST(drivers_sensor_generic_emul_stream, test_drained_queue_keeps_session)
{
	static uint8_t read_buf[READ_BUF_SIZE(DEPTH)] __aligned(8);
	uint32_t next, first, overruns;
	uint8_t *buf;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	submit_read(&stream_iodev, read_buf, sizeof(read_buf));
	zassert_ok(consume_read(&buf));
	next = check_ramp(buf, 0);

	/* With no read pending, sampling goes on and overflows the FIFO */
	k_msleep(4 * DEPTH * PERIOD_MS);

	/* The next read resumes the session past a gap of exactly the overruns */
	submit_read(&stream_iodev, read_buf, sizeof(read_buf));
	zassert_ok(consume_read(&buf));
	overruns = generic_emul_stream_overruns(emul);
	first = first_seq(buf);
	zassert_true(first > next, "no gap after %u", next);
	zassert_equal(first - next, overruns, "%u lost, %u dropped", first - next, overruns);
	check_ramp(buf, first);
}

ZTEST(drivers_sensor_generic_emul_stream, test_unsupported_trigger)
{
	uint32_t len;
	uint8_t *buf;

	zassert_ok(sensor_stream(&drdy_iodev, &stream_rtio, NULL, NULL));
	zassert_equal(-ENOTSUP, consume(&buf, &len));
}

ZTEST(drivers_sensor_generic_emul_stream, test_cancel_stops_sampling)
{
	uint32_t overruns, len;
	uint8_t *buf;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));
	zassert_ok(consume(&buf, &len));
	rtio_release_buffer(&stream_rtio, buf, len);

	zassert_ok(rtio_sqe_cancel(&stream_rtio, handle));
	zassert_equal(-ECANCELED, consume(&buf, &len));

	overruns = generic_emul_stream_overruns(emul);
	k_msleep(4 * DEPTH * PERIOD_MS);
	zassert_equal(overruns, generic_emul_stream_overruns(emul), "still sampling");

	/* A new stream starts the ramp again */
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));
	zassert_ok(consume(&buf, &len));
	check_ramp(buf, 0);
	rtio_release_buffer(&stream_rtio, buf, len);
}

/* Each completion holds one 128-byte block of four; holding them all fills the pool */
#define HELD 4

ZTEST(drivers_sensor_generic_emul_stream, test_full_pool_ends_stream)
{
	uint8_t *held[HELD], *buf;
	uint32_t held_len[HELD], len, next = 0, first;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));

	for (int i = 0; i < HELD; i++) {
		zassert_ok(consume(&held[i], &held_len[i]));
		next = check_ramp(held[i], next);
	}

	zassert_equal(-ENOMEM, consume(&buf, &len));
	zassert_equal(generic_emul_stream_overruns(emul), WATERMARK);

	for (int i = 0; i < HELD; i++) {
		rtio_release_buffer(&stream_rtio, held[i], held_len[i]);
	}

	/* The session is kept, so a re-armed stream resumes past the lost samples */
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));
	zassert_ok(consume(&buf, &len));
	first = first_seq(buf);
	zassert_equal(first - next, generic_emul_stream_overruns(emul), "%u lost, %u dropped",
		      first - next, generic_emul_stream_overruns(emul));
	check_ramp(buf, first);
	rtio_release_buffer(&stream_rtio, buf, len);
}

#define FAIL_SEQ (2 * WATERMARK)

/* Fails the first sample from FAIL_SEQ on, then ramps again */
static int ramp_fail_once(const struct emul *target, struct sensor_chan_spec ch, uint32_t seq,
			  q31_t *value, void *user_data)
{
	bool *failed = user_data;

	if (seq >= FAIL_SEQ && !*failed) {
		*failed = true;
		return -EIO;
	}

	return ramp(target, ch, seq, value, NULL);
}

ZTEST(drivers_sensor_generic_emul_stream, test_sample_error_ends_stream)
{
	/* The source outlives the test until stream_after() removes it */
	static bool failed;
	uint32_t next = 0, len, first;
	uint8_t *buf;

	failed = false;
	set_xyz();
	generic_emul_set_sample_source(emul, ramp_fail_once, &failed);
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));

	while (next < FAIL_SEQ) {
		zassert_ok(consume(&buf, &len));
		next = check_ramp(buf, next);
		rtio_release_buffer(&stream_rtio, buf, len);
	}

	zassert_equal(-EIO, consume(&buf, &len));
	zassert_true(generic_emul_stream_overruns(emul) >= WATERMARK, "%u dropped",
		     generic_emul_stream_overruns(emul));

	/* The session is kept, so a re-armed stream resumes past the failed samples */
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));
	zassert_ok(consume(&buf, &len));
	first = first_seq(buf);
	zassert_true(first >= next + WATERMARK, "resumed at %u after %u", first, next);
	check_ramp(buf, first);
	rtio_release_buffer(&stream_rtio, buf, len);
}

/* Forwards submissions to the emulator, leaving itself as the iodev's sensor */
static void wrapper_submit(const struct device *wrapper, struct rtio_iodev_sqe *iodev_sqe)
{
	ARG_UNUSED(wrapper);

	((const struct sensor_driver_api *)dev->api)->submit(dev, iodev_sqe);
}

static DEVICE_API(sensor, wrapper_api) = {
	.submit = wrapper_submit,
};

DEVICE_DEFINE(wrapper, "wrapper", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_SENSOR_INIT_PRIORITY, &wrapper_api);

static struct sensor_stream_trigger wrapper_triggers[] = {
	{SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE},
};
static struct sensor_read_config wrapper_read_config = {
	.sensor = DEVICE_GET(wrapper),
	.is_streaming = true,
	.triggers = wrapper_triggers,
	.count = ARRAY_SIZE(wrapper_triggers),
	.max = ARRAY_SIZE(wrapper_triggers),
};
RTIO_IODEV_DEFINE(wrapper_iodev, &__sensor_iodev_api, &wrapper_read_config);

ZTEST(drivers_sensor_generic_emul_stream, test_stream_through_a_wrapper)
{
	uint32_t next = 0, len;
	uint8_t *buf;

	set_xyz();
	generic_emul_set_sample_source(emul, ramp, NULL);
	zassert_ok(sensor_stream(&wrapper_iodev, &stream_rtio, NULL, &handle));

	for (int i = 0; i < 2; i++) {
		zassert_ok(consume(&buf, &len));
		next = check_ramp(buf, next);
		rtio_release_buffer(&stream_rtio, buf, len);
	}
}

static K_SEM_DEFINE(workq_release, 0, 1);

static void block_workq(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)k_sem_take(&workq_release, K_FOREVER);
}

/* Code under test may wait for stream data from the system workqueue */
ZTEST(drivers_sensor_generic_emul_stream, test_stream_with_system_workqueue_blocked)
{
	static K_WORK_DEFINE(blocker, block_workq);
	int64_t deadline = k_uptime_get() + 4 * WATERMARK * PERIOD_MS;
	struct rtio_cqe *cqe = NULL;
	uint32_t len = 0;
	uint8_t *buf = NULL;
	int res;

	set_xyz();
	zassert_equal(1, k_work_submit(&blocker));
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));

	while (cqe == NULL && k_uptime_get() < deadline) {
		cqe = rtio_cqe_consume(&stream_rtio);
		if (cqe == NULL) {
			k_msleep(PERIOD_MS);
		}
	}

	k_sem_give(&workq_release);
	zassert_not_null(cqe, "no completion while the system workqueue is blocked");

	res = cqe->result;
	if (res == 0) {
		zassert_ok(rtio_cqe_get_mempool_buffer(&stream_rtio, cqe, &buf, &len));
	}
	rtio_cqe_release(&stream_rtio, cqe);
	zassert_ok(res);
	rtio_release_buffer(&stream_rtio, buf, len);
}

static void drain(void)
{
	while (true) {
		struct rtio_cqe *cqe = rtio_cqe_consume(&stream_rtio);
		uint8_t *buf;
		uint32_t len;

		if (cqe == NULL) {
			break;
		}

		if (cqe->result == 0 &&
		    rtio_cqe_get_mempool_buffer(&stream_rtio, cqe, &buf, &len) == 0) {
			rtio_release_buffer(&stream_rtio, buf, len);
		}
		rtio_cqe_release(&stream_rtio, cqe);
	}
}

static void stream_after(void *unused)
{
	ARG_UNUSED(unused);

	/* Wait for the cancels to complete, and drop what is left */
	(void)rtio_sqe_cancel(&stream_rtio, handle);
	(void)rtio_sqe_cancel(&stream_rtio, second);
	k_msleep(2 * DEPTH * PERIOD_MS);
	drain();

	/* A stream ended by an error keeps its session until re-armed and cancelled */
	generic_emul_reset(emul, true);
	set_xyz();
	zassert_ok(sensor_stream(&stream_iodev, &stream_rtio, NULL, &handle));
	(void)rtio_sqe_cancel(&stream_rtio, handle);
	k_msleep(2 * DEPTH * PERIOD_MS);
	drain();

	generic_emul_set_sample_source(emul, NULL, NULL);
	generic_emul_reset(emul, true);
}

ZTEST_SUITE(drivers_sensor_generic_emul_stream, NULL, NULL, NULL, stream_after, NULL);

#define WM_NODE      DT_NODELABEL(wm_sensor)
#define WM_WATERMARK DT_PROP(WM_NODE, fifo_watermark)
#define WM_DEPTH     DT_PROP(WM_NODE, fifo_depth)
#define WM_PERIOD_MS (MSEC_PER_SEC / DT_PROP(WM_NODE, sampling_frequency))

static const struct device *wm_dev = DEVICE_DT_GET(WM_NODE);
static const struct emul *wm_emul = EMUL_DT_GET(WM_NODE);

SENSOR_DT_STREAM_IODEV(wm_iodev, WM_NODE, {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});

static void wm_set_xyz(void)
{
	struct sensor_chan_spec chan = {SENSOR_CHAN_ACCEL_X, 0};
	q31_t sample = 0;

	for (int i = 0; i < 3; i++) {
		chan.chan_type = SENSOR_CHAN_ACCEL_X + i;
		zassert_ok(emul_sensor_backend_set_channel(wm_emul, chan, &sample, SHIFT));
	}
}

ZTEST(drivers_sensor_generic_emul_watermark, test_completion_holds_watermark)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_chan_spec xyz = {SENSOR_CHAN_ACCEL_XYZ, 0};
	int on_time = 0;
	uint32_t len;
	uint16_t frames;
	uint8_t *buf;

	wm_set_xyz();
	zassert_ok(sensor_get_decoder(wm_dev, &decoder));
	zassert_ok(sensor_stream(&wm_iodev, &stream_rtio, NULL, &handle));

	for (int i = 0; i < 8 && on_time < 2; i++) {
		zassert_ok(consume(&buf, &len));
		zassert_ok(decoder->get_frame_count(buf, xyz, &frames));
		/* A late delivery, as on a loaded qemu, finds the FIFO full */
		zassert_true(frames == WM_WATERMARK || frames == WM_DEPTH,
			     "completion %d has %u frames", i, frames);
		on_time += (frames == WM_WATERMARK);
		rtio_release_buffer(&stream_rtio, buf, len);
	}

	zassert_equal(on_time, 2, "too few completions hold a watermark");
}

/*
 * A watermark that does not divide the FIFO overflows it by part of a watermark, and a buffer
 * holding a watermark leaves the rest of the FIFO for the next read
 */
ZTEST(drivers_sensor_generic_emul_watermark, test_static_buffer_leaves_rest_in_fifo)
{
	static uint8_t full_buf[READ_BUF_SIZE(WM_DEPTH)] __aligned(8);
	static uint8_t short_buf[READ_BUF_SIZE(WM_WATERMARK)] __aligned(8);
	const struct sensor_decoder_api *decoder;
	struct sensor_chan_spec xyz = {SENSOR_CHAN_ACCEL_XYZ, 0};
	uint32_t overruns, next, first;
	uint16_t frames;
	uint8_t *buf;

	wm_set_xyz();
	generic_emul_set_sample_source(wm_emul, ramp, NULL);
	zassert_ok(sensor_get_decoder(wm_dev, &decoder));
	submit_read(&wm_iodev, full_buf, sizeof(full_buf));
	zassert_ok(consume_read(&buf));
	zassert_ok(decoder->get_frame_count(buf, xyz, &frames));
	next = first_seq(buf) + frames;
	overruns = generic_emul_stream_overruns(wm_emul);

	/* Overflow the FIFO */
	k_msleep(4 * WM_DEPTH * WM_PERIOD_MS);

	submit_read(&wm_iodev, short_buf, sizeof(short_buf));
	zassert_ok(consume_read(&buf));
	zassert_ok(decoder->get_frame_count(buf, xyz, &frames));
	zassert_equal(frames, WM_WATERMARK);
	first = first_seq(buf);
	zassert_equal(first - next, generic_emul_stream_overruns(wm_emul) - overruns,
		      "%u lost, %u dropped", first - next,
		      generic_emul_stream_overruns(wm_emul) - overruns);

	submit_read(&wm_iodev, full_buf, sizeof(full_buf));
	zassert_ok(consume_read(&buf));
	zassert_equal(first_seq(buf), first + WM_WATERMARK);
}

static void wm_after(void *unused)
{
	ARG_UNUSED(unused);

	(void)rtio_sqe_cancel(&stream_rtio, handle);
	k_msleep(2 * WM_WATERMARK * WM_PERIOD_MS);
	drain();

	generic_emul_reset(wm_emul, true);
	wm_set_xyz();
	zassert_ok(sensor_stream(&wm_iodev, &stream_rtio, NULL, &handle));
	(void)rtio_sqe_cancel(&stream_rtio, handle);
	k_msleep(2 * WM_WATERMARK * WM_PERIOD_MS);
	drain();

	generic_emul_set_sample_source(wm_emul, NULL, NULL);
	generic_emul_reset(wm_emul, true);
}

ZTEST_SUITE(drivers_sensor_generic_emul_watermark, NULL, NULL, NULL, wm_after, NULL);

#define SLOW_NODE      DT_NODELABEL(slow_sensor)
#define SLOW_WATERMARK DT_PROP(SLOW_NODE, fifo_watermark)
#define SLOW_PERIOD_NS (NSEC_PER_SEC / DT_PROP(SLOW_NODE, sampling_frequency))

static const struct emul *slow_emul = EMUL_DT_GET(SLOW_NODE);

SENSOR_DT_STREAM_IODEV(slow_iodev, SLOW_NODE,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE});
RTIO_DEFINE_WITH_MEMPOOL(slow_rtio, 2, 2, 2, 256, 4);

static rtio_sqe_handle_t slow_handle;

/* Six frames a second apart span more than a 32-bit timestamp delta */
ZTEST(drivers_sensor_generic_emul_slow, test_long_completion_timestamps)
{
	THREE_AXIS_DATA(SLOW_WATERMARK) out;
	const struct sensor_decoder_api *decoder;
	struct sensor_chan_spec xyz = {SENSOR_CHAN_ACCEL_XYZ, 0};
	struct rtio_cqe *cqe;
	uint64_t base_ns;
	uint32_t fit = 0, len;
	q31_t sample = 0;
	uint8_t *buf;

	zassert_ok(emul_sensor_backend_set_channel(slow_emul, xyz, &sample, SHIFT));
	zassert_ok(sensor_get_decoder(DEVICE_DT_GET(SLOW_NODE), &decoder));
	zassert_ok(sensor_stream(&slow_iodev, &slow_rtio, NULL, &slow_handle));

	cqe = rtio_cqe_consume_block(&slow_rtio);
	zassert_ok(cqe->result);
	zassert_ok(rtio_cqe_get_mempool_buffer(&slow_rtio, cqe, &buf, &len));
	rtio_cqe_release(&slow_rtio, cqe);

	zassert_equal(SLOW_WATERMARK - 1,
		      decoder->decode(buf, xyz, &fit, SLOW_WATERMARK, &out.data));
	zassert_equal(out.data.readings[SLOW_WATERMARK - 2].timestamp_delta,
		      (SLOW_WATERMARK - 2) * SLOW_PERIOD_NS);
	base_ns = out.data.header.base_timestamp_ns;

	zassert_equal(1, decoder->decode(buf, xyz, &fit, SLOW_WATERMARK, &out.data));
	zassert_equal(out.data.header.base_timestamp_ns,
		      base_ns + (uint64_t)(SLOW_WATERMARK - 1) * SLOW_PERIOD_NS);
	rtio_release_buffer(&slow_rtio, buf, len);
}

static void slow_after(void *unused)
{
	ARG_UNUSED(unused);

	/* The cancel completes with the next watermark */
	zassert_ok(rtio_sqe_cancel(&slow_rtio, slow_handle));
	while (true) {
		struct rtio_cqe *cqe = rtio_cqe_consume_block(&slow_rtio);
		int res = cqe->result;
		uint8_t *buf;
		uint32_t len;

		if (res == 0 && rtio_cqe_get_mempool_buffer(&slow_rtio, cqe, &buf, &len) == 0) {
			rtio_release_buffer(&slow_rtio, buf, len);
		}
		rtio_cqe_release(&slow_rtio, cqe);

		if (res != 0) {
			break;
		}
	}

	generic_emul_reset(slow_emul, true);
}

ZTEST_SUITE(drivers_sensor_generic_emul_slow, NULL, NULL, NULL, slow_after, NULL);

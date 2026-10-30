/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RTIO stream tests for the accel0 sensor. Expected rate, tolerances and
 * timings come from the zephyr,user node (see README.rst).
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#define ACCEL DT_ALIAS(accel0)
#define USER  DT_PATH(zephyr_user)

#define RATE_HZ            DT_PROP(USER, stream_rate_hz)
#define RATE_TOL_PCT       DT_PROP_OR(USER, rate_tolerance_percent, 5)
#define ONE_G_TOL_MG       DT_PROP_OR(USER, one_g_tolerance_mg, 200)
#define CAPTURE_TIMEOUT_MS DT_PROP_OR(USER, capture_timeout_ms, 1000)
#define CAPTURE_TIMEOUT    K_MSEC(CAPTURE_TIMEOUT_MS)
#define STOP_QUIET_MS      DT_PROP_OR(USER, stop_quiet_ms, 300)
#define IDLE_MS            DT_PROP_OR(USER, idle_ms, 1000)
#define RESTART_CYCLES     DT_PROP_OR(USER, restart_cycles, 10)

#define MEASURE_MS 3000

/* One completion per block, so no completion is dropped while a buffer is free. */
#define POOL_BLOCKS 64
#define BLOCK_SIZE  64

SENSOR_DT_STREAM_IODEV(accel_stream, ACCEL,
		       {SENSOR_TRIG_FIFO_WATERMARK, SENSOR_STREAM_DATA_INCLUDE},
		       {SENSOR_TRIG_FIFO_FULL, SENSOR_STREAM_DATA_INCLUDE});
SENSOR_DT_READ_IODEV(accel_read, ACCEL, {SENSOR_CHAN_ACCEL_XYZ, 0});

RTIO_DEFINE_WITH_MEMPOOL(stream_ctx, 32, POOL_BLOCKS, POOL_BLOCKS, BLOCK_SIZE, sizeof(void *));
RTIO_DEFINE(read_ctx, 1, 1);

static const struct device *const accel = DEVICE_DT_GET(ACCEL);
static const struct sensor_chan_spec xyz = {SENSOR_CHAN_ACCEL_XYZ, 0};

static rtio_sqe_handle_t stream_handle;
static bool streaming;

static struct {
	uint8_t *buf;
	uint32_t len;
} held[POOL_BLOCKS];
static size_t nheld;

/* A q31 reading with the decoder's shift, in mg. */
static int64_t q31_to_mg(q31_t v, int8_t shift)
{
	int64_t ums2;

	if (shift <= 31) {
		ums2 = ((int64_t)v * 1000000LL) >> (31 - shift);
	} else {
		ums2 = ((int64_t)v * 1000000LL) << (shift - 31);
	}
	return ums2 * 1000 / 9806650;
}

static bool sum_frames;
static struct {
	int64_t x, y, z;
	uint32_t n;
} frame_sum;

static void sum_capture(const struct sensor_decoder_api *decoder, const uint8_t *buf,
			uint16_t frames)
{
	struct sensor_three_axis_data data;
	uint32_t fit = 0;

	for (uint16_t i = 0; i < frames; i++) {
		if (decoder->decode(buf, xyz, &fit, 1, &data) != 1) {
			break;
		}
		frame_sum.x += q31_to_mg(data.readings[0].x, data.shift);
		frame_sum.y += q31_to_mg(data.readings[0].y, data.shift);
		frame_sum.z += q31_to_mg(data.readings[0].z, data.shift);
		frame_sum.n++;
	}
}

static void release_held(void)
{
	for (size_t i = 0; i < nheld; i++) {
		rtio_release_buffer(&stream_ctx, held[i].buf, held[i].len);
	}
	nheld = 0;
}

static void start_stream(void)
{
	zassert_ok(sensor_stream(&accel_stream, &stream_ctx, NULL, &stream_handle),
		   "sensor_stream() failed");
	streaming = true;
}

/* Cancel, then drain completions until none has come for STOP_QUIET_MS. */
static void stop_stream(void)
{
	k_timepoint_t quiet_end;

	if (!streaming) {
		return;
	}
	rtio_sqe_cancel(&stream_ctx, stream_handle);
	streaming = false;

	quiet_end = sys_timepoint_calc(K_MSEC(STOP_QUIET_MS));
	while (!sys_timepoint_expired(quiet_end)) {
		struct rtio_cqe *cqe = rtio_cqe_consume(&stream_ctx);
		uint8_t *buf;
		uint32_t len;

		if (cqe == NULL) {
			k_msleep(1);
			continue;
		}
		if (rtio_cqe_get_mempool_buffer(&stream_ctx, cqe, &buf, &len) == 0) {
			rtio_release_buffer(&stream_ctx, buf, len);
		}
		rtio_cqe_release(&stream_ctx, cqe);
		quiet_end = sys_timepoint_calc(K_MSEC(STOP_QUIET_MS));
	}
}

/* Returns the next capture's frame count, 0 on timeout, or its error. */
static int next_capture(k_timeout_t timeout, bool keep)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	const struct sensor_decoder_api *decoder;
	struct rtio_cqe *cqe;
	uint16_t frames = 0;
	uint8_t *buf;
	uint32_t len;
	int result;
	int rc;

	for (;;) {
		cqe = rtio_cqe_consume(&stream_ctx);
		if (cqe == NULL) {
			if (sys_timepoint_expired(end)) {
				return 0;
			}
			k_msleep(1);
			continue;
		}

		result = cqe->result;
		if (rtio_cqe_get_mempool_buffer(&stream_ctx, cqe, &buf, &len) != 0) {
			buf = NULL;
			len = 0;
		}
		rtio_cqe_release(&stream_ctx, cqe);

		if (result == -ECANCELED) {
			if (buf != NULL) {
				rtio_release_buffer(&stream_ctx, buf, len);
			}
			continue;
		}
		break;
	}

	if (result < 0) {
		if (buf != NULL) {
			rtio_release_buffer(&stream_ctx, buf, len);
		}
		return result;
	}
	zassert_not_null(buf, "the completion carried no buffer");

	rc = sensor_get_decoder(accel, &decoder);
	if (rc == 0) {
		rc = decoder->get_frame_count(buf, xyz, &frames);
	}
	if (rc == 0 && sum_frames) {
		sum_capture(decoder, buf, frames);
	}

	if (keep && nheld < ARRAY_SIZE(held)) {
		held[nheld].buf = buf;
		held[nheld].len = len;
		nheld++;
	} else {
		rtio_release_buffer(&stream_ctx, buf, len);
	}

	zassert_ok(rc, "counting the capture's frames failed: %d", rc);
	zassert_true(frames > 0, "a capture with no frames");
	return frames;
}

/* The sensor must be at rest. */
static void assert_reads_1g(const char *what, int64_t x, int64_t y, int64_t z)
{
	const int64_t lo = (1000 - ONE_G_TOL_MG) * (1000 - ONE_G_TOL_MG);
	const int64_t hi = (1000 + ONE_G_TOL_MG) * (1000 + ONE_G_TOL_MG);
	int64_t mag2 = x * x + y * y + z * z;

	TC_PRINT("%s: x=%lld y=%lld z=%lld mg\n", what, x, y, z);
	zassert_between_inclusive(mag2, lo, hi,
				  "%s: |a|^2 = %lld mg^2, expected (1000 +/- %d mg)^2 at rest",
				  what, mag2, ONE_G_TOL_MG);
}

static void assert_one_shot_reads_1g(void)
{
	const struct sensor_decoder_api *decoder;
	struct sensor_three_axis_data data;
	uint8_t buf[256];
	uint16_t frames = 0;
	uint32_t fit = 0;
	int rc;

	zassert_ok(sensor_read(&accel_read, &read_ctx, buf, sizeof(buf)), "sensor_read() failed");
	zassert_ok(sensor_get_decoder(accel, &decoder));
	zassert_ok(decoder->get_frame_count(buf, xyz, &frames));
	zassert_equal(frames, 1, "a one-shot read reports %u frames", frames);

	rc = decoder->decode(buf, xyz, &fit, 1, &data);
	zassert_equal(rc, 1, "decode() of a one-shot read returned %d, expected 1 frame", rc);

	assert_reads_1g("one-shot read", q31_to_mg(data.readings[0].x, data.shift),
			q31_to_mg(data.readings[0].y, data.shift),
			q31_to_mg(data.readings[0].z, data.shift));
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(accel), "%s is not ready", accel->name);
	TC_PRINT("%s: expecting %d Hz +/- %d %%\n", accel->name, RATE_HZ, RATE_TOL_PCT);
	return NULL;
}

static void after_each(void *fixture)
{
	ARG_UNUSED(fixture);
	release_held();
	stop_stream();
}

/* Frames are decoded one at a time, so the decoder's fit cursor is exercised. */
ZTEST(rtio_stream, test_1_stream_rate_and_data)
{
	int64_t t0, elapsed;
	uint32_t total = 0;
	int frames;

	start_stream();
	zassert_true(next_capture(CAPTURE_TIMEOUT, false) > 0, "no first capture");

	memset(&frame_sum, 0, sizeof(frame_sum));
	sum_frames = true;
	t0 = k_uptime_get();
	do {
		frames = next_capture(CAPTURE_TIMEOUT, false);
		zassert_true(frames > 0, "the stream stopped after %u frames (%d)", total, frames);
		total += frames;
		elapsed = k_uptime_get() - t0;
	} while (elapsed < MEASURE_MS);
	sum_frames = false;

	uint32_t rate = (uint32_t)(total * 1000LL / elapsed);

	TC_PRINT("%u frames in %lld ms: %u Hz (expected %d)\n", total, elapsed, rate, RATE_HZ);
	zassert_within(rate, RATE_HZ, RATE_HZ * RATE_TOL_PCT / 100,
		       "%u Hz is more than %d %% away from %d Hz", rate, RATE_TOL_PCT, RATE_HZ);

	zassert_equal(frame_sum.n, total, "decoded %u of %u frames", frame_sum.n, total);
	assert_reads_1g("stream mean", frame_sum.x / frame_sum.n, frame_sum.y / frame_sum.n,
			frame_sum.z / frame_sum.n);
}

/* While stopped, the FIFO fills and its interrupt fires with no stream to serve. */
ZTEST(rtio_stream, test_2_restart_after_idle)
{
	int frames;

	start_stream();
	zassert_true(next_capture(CAPTURE_TIMEOUT, false) > 0, "no capture before the stop");
	stop_stream();

	k_msleep(IDLE_MS);

	start_stream();
	frames = next_capture(CAPTURE_TIMEOUT, false);
	zassert_true(frames > 0, "no capture within %d ms of restarting after %d ms stopped (%d)",
		     CAPTURE_TIMEOUT_MS, IDLE_MS, frames);
}

/* Vary the stopped time so the stop lands at different points of the FIFO fill. */
ZTEST(rtio_stream, test_3_restart_cycles)
{
	for (int i = 0; i < RESTART_CYCLES; i++) {
		int frames;

		start_stream();
		frames = next_capture(CAPTURE_TIMEOUT, false);
		zassert_true(frames > 0, "cycle %d: no capture after the restart (%d)", i, frames);
		stop_stream();
		k_msleep((i * 37) % 250);
	}
}

ZTEST(rtio_stream, test_4_one_shot_read)
{
	assert_one_shot_reads_1g();
}

ZTEST(rtio_stream, test_5_one_shot_after_stream)
{
	start_stream();
	zassert_true(next_capture(CAPTURE_TIMEOUT, false) > 0, "no capture");
	stop_stream();

	assert_one_shot_reads_1g();
}

/*
 * Hold captures until the mempool runs dry, then release and restart.
 * Runs last: a driver that fails it streams no more until reset.
 */
ZTEST(rtio_stream, test_6_restart_after_pool_exhausted)
{
	int frames;
	int rc;

	start_stream();
	do {
		rc = next_capture(CAPTURE_TIMEOUT, true);
	} while (rc > 0 && nheld < ARRAY_SIZE(held));

	if (rc < 0) {
		TC_PRINT("held %zu captures; the stream then ended with %d\n", nheld, rc);
	} else {
		TC_PRINT("held %zu captures; the stream then stopped completing\n", nheld);
	}
	zassert_true(nheld > 0, "no capture before the pool ran dry");

	release_held();
	stop_stream();

	start_stream();
	frames = next_capture(CAPTURE_TIMEOUT, false);
	zassert_true(frames > 0, "no capture after the mempool ran dry and was released (%d)",
		     frames);
}

ZTEST_SUITE(rtio_stream, NULL, suite_setup, NULL, after_each, NULL);

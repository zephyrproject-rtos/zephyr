/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for the native_sim host FIFO video source. The driver must expose the
 * fixed devicetree format, reject any other format, keep the simulation running
 * while no writer is attached, and hand over frame aligned buffers once a host
 * writer feeds the pipe.
 *
 * The host writer is provided by the test itself, in the native simulator
 * runner context, so that the suite needs no external tool and stays
 * deterministic.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

#include "fifo_writer_bottom.h"

#define VIDEO_FIFO_NODE DT_NODELABEL(video_fifo)

#define FIFO_WIDTH  DT_PROP(VIDEO_FIFO_NODE, width)
#define FIFO_HEIGHT DT_PROP(VIDEO_FIFO_NODE, height)
#define FIFO_PITCH  (FIFO_WIDTH * 2)
#define FIFO_SIZE   (FIFO_PITCH * FIFO_HEIGHT)
#define FIFO_PATH   DT_PROP(VIDEO_FIFO_NODE, fifo_path)

static const struct device *const fifo_dev = DEVICE_DT_GET(VIDEO_FIFO_NODE);

/* Frames are staged here rather than on a thread stack: they are too large */
static uint8_t test_frame[FIFO_SIZE];

/* Writer left open by the running test, closed by the teardown hook */
static int test_writer_fd = -1;

static void test_frame_fill(uint8_t seed)
{
	for (size_t i = 0; i < FIFO_SIZE; i++) {
		test_frame[i] = (uint8_t)(seed + i * 7U);
	}
}

/* Enqueue a single frame sized buffer and start the stream, creating the FIFO */
static void test_stream_start_with_one_buffer(void)
{
	struct video_buffer *vbuf = video_buffer_alloc(FIFO_SIZE, K_NO_WAIT);

	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

	zassert_ok(video_enqueue(fifo_dev, vbuf));
	zassert_ok(video_stream_start(fifo_dev, VIDEO_BUF_TYPE_OUTPUT));
}

ZTEST(video_native_sim_fifo, test_device_ready)
{
	zassert_true(device_is_ready(fifo_dev), "the FIFO video device is not ready");
}

ZTEST(video_native_sim_fifo, test_get_format)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};

	zassert_ok(video_get_format(fifo_dev, &fmt));
	zassert_equal(fmt.pixelformat, VIDEO_PIX_FMT_RGB565);
	zassert_equal(fmt.width, FIFO_WIDTH);
	zassert_equal(fmt.height, FIFO_HEIGHT);
	zassert_equal(fmt.pitch, FIFO_PITCH);
	zassert_equal(fmt.size, FIFO_SIZE);
}

ZTEST(video_native_sim_fifo, test_get_caps)
{
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};

	zassert_ok(video_get_caps(fifo_dev, &caps));
	zassert_not_null(caps.format_caps);

	/* The host writer decides the format, so exactly one format is advertised. */
	zassert_equal(caps.format_caps[0].pixelformat, VIDEO_PIX_FMT_RGB565);
	zassert_equal(caps.format_caps[0].width_min, FIFO_WIDTH);
	zassert_equal(caps.format_caps[0].width_max, FIFO_WIDTH);
	zassert_equal(caps.format_caps[0].height_min, FIFO_HEIGHT);
	zassert_equal(caps.format_caps[0].height_max, FIFO_HEIGHT);
	zassert_equal(caps.format_caps[1].pixelformat, 0, "the format list is not terminated");
}

ZTEST(video_native_sim_fifo, test_set_format_accepts_the_devicetree_format)
{
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = FIFO_WIDTH,
		.height = FIFO_HEIGHT,
	};

	zassert_ok(video_set_format(fifo_dev, &fmt));
	zassert_equal(fmt.pitch, FIFO_PITCH, "set_format must fill the pitch in");
	zassert_equal(fmt.size, FIFO_SIZE, "set_format must fill the size in");
}

ZTEST(video_native_sim_fifo, test_set_format_rejects_other_formats)
{
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width = FIFO_WIDTH,
		.height = FIFO_HEIGHT,
	};

	fmt.width = FIFO_WIDTH * 2;
	zassert_equal(video_set_format(fifo_dev, &fmt), -ENOTSUP, "a wider format was accepted");

	fmt.width = FIFO_WIDTH;
	fmt.height = FIFO_HEIGHT * 2;
	zassert_equal(video_set_format(fifo_dev, &fmt), -ENOTSUP, "a taller format was accepted");

	fmt.height = FIFO_HEIGHT;
	fmt.pixelformat = VIDEO_PIX_FMT_YUYV;
	zassert_equal(video_set_format(fifo_dev, &fmt), -ENOTSUP,
		      "another pixel format was accepted");
}

ZTEST(video_native_sim_fifo, test_enqueue_rejects_small_buffers)
{
	/*
	 * video_enqueue() hands the driver the pool entry matching the buffer index,
	 * not the buffer passed in, so the buffer has to come from the pool for the
	 * driver to see the truncated size at all.
	 */
	struct video_buffer *vbuf = video_buffer_alloc(FIFO_SIZE - 1, K_NO_WAIT);

	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

	zassert_equal(video_enqueue(fifo_dev, vbuf), -EINVAL,
		      "a buffer smaller than a frame was accepted");

	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_no_frame_without_a_host_writer)
{
	struct video_buffer *vbuf = video_buffer_alloc(FIFO_SIZE, K_NO_WAIT);
	int64_t uptime;

	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

	zassert_ok(video_enqueue(fifo_dev, vbuf));

	/* Opening and polling the FIFO must succeed even though nothing writes to it. */
	zassert_ok(video_stream_start(fifo_dev, VIDEO_BUF_TYPE_OUTPUT));

	/*
	 * The driver polls the FIFO without ever blocking, so the simulated time
	 * must keep advancing and no frame must be delivered.
	 */
	uptime = k_uptime_get();
	k_sleep(K_MSEC(100));
	zassert_true(k_uptime_get() - uptime >= 100, "the simulation did not advance");

	vbuf = NULL;
	zassert_equal(video_dequeue(fifo_dev, &vbuf, K_MSEC(100)), -EAGAIN,
		      "a frame was delivered without a host writer");

	zassert_ok(video_stream_stop(fifo_dev, VIDEO_BUF_TYPE_OUTPUT));

	/* Stopping the stream cancels the pending buffer, which is ours to release */
	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_NO_WAIT), "the buffer was not flushed");
	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_frame_from_a_host_writer)
{
	struct video_buffer *vbuf = NULL;

	/* The driver creates the FIFO, so a writer can only attach once streaming */
	test_stream_start_with_one_buffer();

	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	test_frame_fill(0x11);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not write a whole frame");

	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");
	zassert_equal(vbuf->bytesused, FIFO_SIZE, "the frame is not a whole frame");
	zassert_mem_equal(vbuf->buffer, test_frame, FIFO_SIZE,
			  "the delivered frame does not match what the host wrote");

	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_partial_frame_is_discarded)
{
	struct video_buffer *vbuf = NULL;

	test_stream_start_with_one_buffer();

	/* A writer that disconnects in the middle of a frame */
	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	memset(test_frame, 0xAA, FIFO_SIZE / 2);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE / 2),
		      FIFO_SIZE / 2, "the host writer could not write a partial frame");

	zassert_ok(video_fifo_test_close(test_writer_fd));
	test_writer_fd = -1;

	/* Let the driver observe the end of file and drop the incomplete frame */
	k_sleep(K_MSEC(50));

	zassert_equal(video_dequeue(fifo_dev, &vbuf, K_NO_WAIT), -EAGAIN,
		      "an incomplete frame was delivered");

	/* The next writer to attach must be picked up frame aligned */
	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not re-attach a host writer to %s", FIFO_PATH);

	test_frame_fill(0x5A);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not write a whole frame");

	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");
	zassert_equal(vbuf->bytesused, FIFO_SIZE, "the frame is not a whole frame");
	zassert_mem_equal(vbuf->buffer, test_frame, FIFO_SIZE,
			  "the frame is misaligned, the partial frame was not discarded");

	zassert_ok(video_buffer_release(vbuf));
}

static void video_native_sim_fifo_after(void *fixture)
{
	struct video_buffer *vbuf;

	ARG_UNUSED(fixture);

	if (test_writer_fd >= 0) {
		(void)video_fifo_test_close(test_writer_fd);
		test_writer_fd = -1;
	}

	/* Stopping an idle stream is harmless, and it cancels any pending buffer */
	(void)video_stream_stop(fifo_dev, VIDEO_BUF_TYPE_OUTPUT);

	while (video_dequeue(fifo_dev, &vbuf, K_NO_WAIT) == 0) {
		(void)video_buffer_release(vbuf);
	}

	/* Leave no named pipe behind on the host filesystem */
	(void)video_fifo_test_unlink(FIFO_PATH);
}

ZTEST_SUITE(video_native_sim_fifo, NULL, NULL, NULL, video_native_sim_fifo_after, NULL);

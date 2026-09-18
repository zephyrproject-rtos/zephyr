/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for the native_sim host FIFO video source. The driver must start with
 * its default format, accept every format it advertises and reject the others,
 * keep the simulation running while no writer is attached, and hand over frame
 * aligned buffers once a host writer feeds the pipe.
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

#define FIFO_PATH DT_PROP(VIDEO_FIFO_NODE, fifo_path)

/* Default format of the driver, RGB565 */
#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 240

/* Smaller RGB565 format selected by the streaming tests */
#define FIFO_WIDTH  64
#define FIFO_HEIGHT 32
#define FIFO_PITCH  (FIFO_WIDTH * 2)
#define FIFO_SIZE   (FIFO_PITCH * FIFO_HEIGHT)

/* Another format, with another number of bytes per pixel */
#define RGB24_WIDTH  40
#define RGB24_HEIGHT 24
#define RGB24_SIZE   (RGB24_WIDTH * 3 * RGB24_HEIGHT)

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

static int test_set_format(uint32_t pixelformat, uint32_t width, uint32_t height)
{
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = pixelformat,
		.width = width,
		.height = height,
	};

	return video_set_format(fifo_dev, &fmt);
}

/* Enqueue a single frame sized buffer and start the stream, creating the FIFO */
static void test_stream_start_with_one_buffer(void)
{
	struct video_buffer *vbuf;

	zassert_ok(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, FIFO_HEIGHT));

	vbuf = video_buffer_alloc(FIFO_SIZE, K_NO_WAIT);
	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

	zassert_ok(video_enqueue(fifo_dev, vbuf));
	zassert_ok(video_stream_start(fifo_dev, VIDEO_BUF_TYPE_OUTPUT));
}

/* Select a format, and check that it is filled in and reported back */
static void test_check_set_format(uint32_t pixelformat, uint32_t width, uint32_t height)
{
	uint32_t pitch = width * video_bits_per_pixel(pixelformat) / BITS_PER_BYTE;
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = pixelformat,
		.width = width,
		.height = height,
	};
	struct video_format cur = {.type = VIDEO_BUF_TYPE_OUTPUT};

	zassert_ok(video_set_format(fifo_dev, &fmt), "%s %ux%u was rejected",
		   VIDEO_FOURCC_TO_STR(pixelformat), width, height);
	zassert_equal(fmt.pitch, pitch, "set_format must fill the pitch in");
	zassert_equal(fmt.size, pitch * height, "set_format must fill the size in");

	zassert_ok(video_get_format(fifo_dev, &cur));
	zassert_true((cur.pixelformat == pixelformat) && (cur.width == width) &&
			     (cur.height == height) && (cur.pitch == fmt.pitch) &&
			     (cur.size == fmt.size),
		     "get_format does not report the format just set");
}

ZTEST(video_native_sim_fifo, test_device_ready)
{
	zassert_true(device_is_ready(fifo_dev), "the FIFO video device is not ready");
}

ZTEST(video_native_sim_fifo, test_default_format)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};

	zassert_ok(video_get_format(fifo_dev, &fmt));
	zassert_equal(fmt.pixelformat, VIDEO_PIX_FMT_RGB565);
	zassert_equal(fmt.width, DEFAULT_WIDTH);
	zassert_equal(fmt.height, DEFAULT_HEIGHT);
	zassert_equal(fmt.pitch, DEFAULT_WIDTH * 2);
	zassert_equal(fmt.size, DEFAULT_WIDTH * 2 * DEFAULT_HEIGHT);
}

ZTEST(video_native_sim_fifo, test_get_caps)
{
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	const struct video_format_cap *rgb565 = NULL;
	const struct video_format_cap *yuyv = NULL;

	zassert_ok(video_get_caps(fifo_dev, &caps));
	zassert_not_null(caps.format_caps);

	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *cap = &caps.format_caps[i];

		zassert_equal(cap->width_max, 1920);
		zassert_equal(cap->height_max, 1080);

		if (cap->pixelformat == VIDEO_PIX_FMT_RGB565) {
			rgb565 = cap;
		} else if (cap->pixelformat == VIDEO_PIX_FMT_YUYV) {
			yuyv = cap;
		}
	}

	zassert_not_null(rgb565, "the default pixel format is not advertised");
	zassert_not_null(yuyv, "YUYV is not advertised");
	zassert_equal(yuyv->width_step, 2, "YUYV packs two pixels per macropixel");
}

ZTEST(video_native_sim_fifo, test_set_format_accepts_every_advertised_format)
{
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};

	zassert_ok(video_get_caps(fifo_dev, &caps));

	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *cap = &caps.format_caps[i];

		test_check_set_format(cap->pixelformat, cap->width_min, cap->height_min);
		test_check_set_format(cap->pixelformat, cap->width_max, cap->height_max);
	}
}

ZTEST(video_native_sim_fifo, test_set_format_rejects_unsupported_formats)
{
	struct video_format fmt = {.type = VIDEO_BUF_TYPE_OUTPUT};

	zassert_ok(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, FIFO_HEIGHT));

	zassert_equal(test_set_format(VIDEO_PIX_FMT_JPEG, FIFO_WIDTH, FIFO_HEIGHT), -ENOTSUP,
		      "a compressed format was accepted");
	zassert_equal(test_set_format(VIDEO_PIX_FMT_NV12, FIFO_WIDTH, FIFO_HEIGHT), -ENOTSUP,
		      "a planar format was accepted");
	zassert_equal(test_set_format(VIDEO_PIX_FMT_RGB565, 1921, FIFO_HEIGHT), -ENOTSUP,
		      "a width above 1920 was accepted");
	zassert_equal(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, 1081), -ENOTSUP,
		      "a height above 1080 was accepted");
	zassert_equal(test_set_format(VIDEO_PIX_FMT_RGB565, 0, FIFO_HEIGHT), -ENOTSUP,
		      "a zero width was accepted");
	zassert_equal(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, 0), -ENOTSUP,
		      "a zero height was accepted");
	zassert_equal(test_set_format(VIDEO_PIX_FMT_YUYV, FIFO_WIDTH + 1, FIFO_HEIGHT), -ENOTSUP,
		      "an odd YUYV width was accepted");

	/* The format in use is left unchanged */
	zassert_ok(video_get_format(fifo_dev, &fmt));
	zassert_equal(fmt.pixelformat, VIDEO_PIX_FMT_RGB565);
	zassert_equal(fmt.width, FIFO_WIDTH);
	zassert_equal(fmt.height, FIFO_HEIGHT);
}

ZTEST(video_native_sim_fifo, test_enqueue_rejects_small_buffers)
{
	struct video_buffer *vbuf;

	zassert_ok(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, FIFO_HEIGHT));

	/*
	 * video_enqueue() hands the driver the pool entry matching the buffer index,
	 * not the buffer passed in, so the buffer has to come from the pool for the
	 * driver to see the truncated size at all.
	 */
	vbuf = video_buffer_alloc(FIFO_SIZE - 1, K_NO_WAIT);
	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

	zassert_equal(video_enqueue(fifo_dev, vbuf), -EINVAL,
		      "a buffer smaller than a frame was accepted");

	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_no_frame_without_a_host_writer)
{
	struct video_buffer *vbuf;
	int64_t uptime;

	zassert_ok(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, FIFO_HEIGHT));

	vbuf = video_buffer_alloc(FIFO_SIZE, K_NO_WAIT);
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

ZTEST(video_native_sim_fifo, test_flush_waits_for_the_pending_frame)
{
	struct video_buffer *vbuf = NULL;

	test_stream_start_with_one_buffer();

	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	/* A non cancelling flush waits for the host, so write the frame first */
	test_frame_fill(0x33);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not write a whole frame");

	zassert_ok(video_driver_flush(fifo_dev, false));

	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_NO_WAIT), "the buffer was not filled");
	zassert_equal(vbuf->bytesused, FIFO_SIZE, "the frame is not a whole frame");
	zassert_mem_equal(vbuf->buffer, test_frame, FIFO_SIZE,
			  "the delivered frame does not match what the host wrote");

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

ZTEST(video_native_sim_fifo, test_frame_in_the_selected_format)
{
	struct video_buffer *vbuf;

	zassert_ok(test_set_format(VIDEO_PIX_FMT_RGB24, RGB24_WIDTH, RGB24_HEIGHT));

	vbuf = video_buffer_alloc(RGB24_SIZE, K_NO_WAIT);
	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

	zassert_ok(video_enqueue(fifo_dev, vbuf));
	zassert_ok(video_stream_start(fifo_dev, VIDEO_BUF_TYPE_OUTPUT));

	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	test_frame_fill(0x77);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, RGB24_SIZE), RGB24_SIZE,
		      "the host writer could not write a whole frame");

	vbuf = NULL;
	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");
	zassert_equal(vbuf->bytesused, RGB24_SIZE, "the frame does not have the selected size");
	zassert_mem_equal(vbuf->buffer, test_frame, RGB24_SIZE,
			  "the delivered frame does not match what the host wrote");

	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_host_waits_while_buffers_are_held)
{
	struct video_buffer *vbuf = NULL;
	struct video_buffer *none = NULL;

	test_stream_start_with_one_buffer();

	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	test_frame_fill(0x11);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not write a whole frame");
	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");

	/* The application holds the only buffer, so the next frame stays in the pipe */
	test_frame_fill(0x22);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not write a second frame");
	k_sleep(K_MSEC(50));
	zassert_equal(video_dequeue(fifo_dev, &none, K_NO_WAIT), -EAGAIN,
		      "a frame was delivered while the application held every buffer");

	zassert_ok(video_enqueue(fifo_dev, vbuf));
	vbuf = NULL;

	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");
	zassert_equal(vbuf->bytesused, FIFO_SIZE, "the frame is not a whole frame");
	zassert_mem_equal(vbuf->buffer, test_frame, FIFO_SIZE,
			  "the second frame was lost or corrupted");

	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_cancelling_mid_frame_keeps_frames_aligned)
{
	struct video_buffer *vbuf = NULL;

	test_stream_start_with_one_buffer();

	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	memset(test_frame, 0xAA, FIFO_SIZE);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE / 2),
		      FIFO_SIZE / 2, "the host writer could not write a partial frame");
	k_sleep(K_MSEC(50));

	zassert_ok(video_driver_flush(fifo_dev, true));
	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_NO_WAIT), "the buffer was not returned");
	zassert_equal(vbuf->bytesused, 0, "a cancelled buffer holds a frame");

	/* The rest of the cancelled frame, then a whole one */
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE / 2),
		      FIFO_SIZE / 2, "the host writer could not finish the frame");
	test_frame_fill(0x5A);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not write a whole frame");

	zassert_ok(video_enqueue(fifo_dev, vbuf));
	vbuf = NULL;

	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");
	zassert_mem_equal(vbuf->buffer, test_frame, FIFO_SIZE,
			  "the frame is misaligned, the cancelled frame was not skipped");

	zassert_ok(video_buffer_release(vbuf));
}

ZTEST(video_native_sim_fifo, test_larger_format_mid_frame_keeps_frames_aligned)
{
	struct video_buffer *vbuf = NULL;

	test_stream_start_with_one_buffer();

	test_writer_fd = video_fifo_test_open_writer(FIFO_PATH);
	zassert_true(test_writer_fd >= 0, "could not attach a host writer to %s", FIFO_PATH);

	memset(test_frame, 0xAA, FIFO_SIZE);
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE / 2),
		      FIFO_SIZE / 2, "the host writer could not write a partial frame");
	k_sleep(K_MSEC(50));

	/* The queued buffer is too small for the new format, so it comes back unfilled */
	zassert_ok(test_set_format(VIDEO_PIX_FMT_RGB565, FIFO_WIDTH, FIFO_HEIGHT * 2));
	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "the buffer was not returned");
	zassert_equal(vbuf->bytesused, 0, "a buffer too small for a frame was filled");
	zassert_ok(video_buffer_release(vbuf));

	vbuf = video_buffer_alloc(FIFO_SIZE * 2, K_NO_WAIT);
	zassert_not_null(vbuf, "could not allocate a video buffer");
	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(fifo_dev, vbuf));

	/* The rest of the interrupted frame at the new size, then a whole one */
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE), FIFO_SIZE,
		      "the host writer could not finish the frame");
	zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE / 2),
		      FIFO_SIZE / 2, "the host writer could not finish the frame");
	test_frame_fill(0x5A);
	for (int i = 0; i < 2; i++) {
		zassert_equal(video_fifo_test_write(test_writer_fd, test_frame, FIFO_SIZE),
			      FIFO_SIZE, "the host writer could not write a whole frame");
	}

	vbuf = NULL;
	zassert_ok(video_dequeue(fifo_dev, &vbuf, K_MSEC(1000)), "no frame was delivered");
	zassert_equal(vbuf->bytesused, FIFO_SIZE * 2, "the frame is not a whole frame");
	zassert_mem_equal(vbuf->buffer, test_frame, FIFO_SIZE,
			  "the frame is misaligned, the interrupted frame was not skipped");
	zassert_mem_equal(vbuf->buffer + FIFO_SIZE, test_frame, FIFO_SIZE,
			  "the frame is misaligned, the interrupted frame was not skipped");

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

	/* Every test starts from the default format */
	(void)test_set_format(VIDEO_PIX_FMT_RGB565, DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

ZTEST_SUITE(video_native_sim_fifo, NULL, NULL, NULL, video_native_sim_fifo_after, NULL);

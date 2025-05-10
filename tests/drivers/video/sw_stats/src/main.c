/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/pixel/print.h>

#define WIDTH 640
#define HEIGHT 480

uint8_t rggb8frame[WIDTH * HEIGHT * 1];
uint16_t rgb24hist[(1 << 4) * 3];

ZTEST(video_sw_stats, test_video_sw_stats)
{
	const struct device *dev = device_get_binding("VIDEO_SW_STATS");
	struct video_buffer *vbufp;
	struct video_stats_channels chan = {
		.base.flags = VIDEO_STATS_CHANNELS_RGB
	};
	struct video_stats_histogram hist = {
		.base.flags = VIDEO_STATS_HISTOGRAM_RGB,
		.buckets = rgb24hist,
		.num_buckets = ARRAY_SIZE(rgb24hist),
	};
	struct video_buffer vbuf = {
		.buffer = rggb8frame,
		.size = sizeof(rggb8frame),
		.bytesused = sizeof(rggb8frame),
	};
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGGB8,
		.width = WIDTH,
		.pitch = WIDTH * video_bits_per_pixel(VIDEO_PIX_FMT_RGGB8) / BITS_PER_BYTE,
		.height = HEIGHT,
	};

	zexpect_not_null(dev);
	zexpect_true(device_is_ready(dev));

	/* Load test data on the buffer: upper half completely white */

	memset(rggb8frame, 0xff, sizeof(rggb8frame) / 2);

	zexpect_ok(video_set_format(dev, VIDEO_EP_IN, &fmt));

	/* Test loading a buffer into the device */

	zexpect_ok(video_enqueue(dev, VIDEO_EP_IN, &vbuf));

	/* Test collecting image statistics out of this buffer */

	zexpect_ok(video_get_stats(dev, VIDEO_EP_IN, &chan.base));
	zexpect_ok(video_get_stats(dev, VIDEO_EP_IN, &hist.base));

	/* Test collecting image statistics out of this buffer */

	zexpect_ok(video_dequeue(dev, VIDEO_EP_IN, &vbufp, K_NO_WAIT));
	zexpect_not_null(vbufp);

	/* Check the statistics content of channel averages */

	zexpect_equal(chan.base.flags & VIDEO_STATS_CHANNELS_RGB, VIDEO_STATS_CHANNELS_RGB);
	zexpect_within(chan.rgb[0], 0xff / 2, 10, "%u is average", chan.rgb[0]);
	zexpect_within(chan.rgb[1], 0xff / 2, 10, "%u is average", chan.rgb[1]);
	zexpect_within(chan.rgb[2], 0xff / 2, 10, "%u is average", chan.rgb[2]);

	/* Check the channel averagres */

	zexpect_equal(hist.base.flags & VIDEO_STATS_HISTOGRAM_RGB, VIDEO_STATS_HISTOGRAM_RGB);
	zexpect_not_equal(hist.num_buckets, 0, "The histogram size must not be zero.");
	zexpect_not_equal(hist.num_values, 0, "The histogram must not be empty.");

	/* Check the histograms */

	zexpect_within(hist.buckets[0], hist.num_values / 2, 10,
		       "Half of the image is filled with 0x00");

	zexpect_within(hist.buckets[hist.num_buckets - 1], hist.num_values / 2, 10,
		       "Half of the image is filled with 0xff");

	for (size_t i = hist.num_buckets * 0 / 3 + 1; i < hist.num_buckets * 1 / 3 - 1; i++) {
		zexpect_equal(hist.buckets[i], 0, "%u: Only 0x00 or 0xff for the red channel", i);
	}

	for (size_t i = hist.num_buckets * 1 / 3 + 1; i < hist.num_buckets * 2 / 3 - 1; i++) {
		zexpect_equal(hist.buckets[i], 0, "%u: Only 0x00 or 0xff in the green channel", i);
	}

	for (size_t i = hist.num_buckets * 2 / 3 + 1; i < hist.num_buckets * 3 / 3 - 1; i++) {
		zexpect_equal(hist.buckets[i], 0, "%u: Only 0x00 or 0xff in the blue channel", i);
	}
}

ZTEST_SUITE(video_sw_stats, NULL, NULL, NULL, NULL, NULL);

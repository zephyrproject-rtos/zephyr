/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

enum {
	RGB565,
	YUYV_A,
	YUYV_B,
};

static const struct video_format_cap fmts[] = {
	[RGB565] = {.pixelformat = VIDEO_PIX_FMT_RGB565,
		    .width_min  = 1280, .width_max  = 1280, .width_step  = 50,
		    .height_min = 720,  .height_max = 720,  .height_step = 50},
	[YUYV_A] = {.pixelformat = VIDEO_PIX_FMT_YUYV,
		    .width_min  = 100,  .width_max  = 1000, .width_step  = 50,
		    .height_min = 100,  .height_max = 1000, .height_step = 50},
	[YUYV_B] = {.pixelformat = VIDEO_PIX_FMT_YUYV,
		    .width_min  = 1920, .width_max  = 1920, .width_step  = 0,
		    .height_min = 1080, .height_max = 1080, .height_step = 0},
	{0},
};

ZTEST(video_common, test_video_format_caps_index)
{
	struct video_format fmt = {0};
	size_t idx;
	int ret;

	fmt.pixelformat = VIDEO_PIX_FMT_YUYV;

	fmt.width = 100;
	fmt.height = 100;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting minimum value to match");
	zassert_equal(idx, YUYV_A);

	fmt.width = 1000;
	fmt.height = 1000;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting maximum value to match");
	zassert_equal(idx, YUYV_A);

	fmt.width = 1920;
	fmt.height = 1080;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting exact match to work");
	zassert_equal(idx, YUYV_B);

	fmt.width = 1001;
	fmt.height = 1000;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting 1 above maximum width to mismatch");

	fmt.width = 1000;
	fmt.height = 1001;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting 1 above maximum height to mismatch");

	fmt.width = 1280;
	fmt.height = 720;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret);
	zassert_not_ok(ret, "expecting wrong format to mismatch");

	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;

	fmt.width = 1000;
	fmt.height = 1000;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting wrong format to mismatch");

	fmt.width = 1280;
	fmt.height = 720;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting exact match to work");
	zassert_equal(idx, RGB565);
}

ZTEST(video_common, test_video_frmival_usec)
{
	zassert_equal(
		video_frmival_usec(&(struct video_frmival){.usec = 66666}),
		66666U);

	zassert_equal(
		video_frmival_usec(&(struct video_frmival){.usec = 33333}),
		33333U);

	zassert_equal(
		video_frmival_usec(&(struct video_frmival){.usec = 5000000}),
		5000000U);

	zassert_equal(
		video_frmival_usec(&(struct video_frmival){.usec = 1}),
		1U);
}

ZTEST(video_common, test_video_frmival_nsec)
{
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.usec = 66666}),
		66666000ULL);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.usec = 33333}),
		33333000ULL);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.usec = 5000000}),
		5000000000ULL);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.usec = 1}),
		1000ULL);

	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
}

ZTEST(video_common, test_video_closest_frmival_stepwise)
{
	struct video_frmival_stepwise stepwise;
	uint32_t desired;
	uint32_t expected;
	uint32_t match;
	int ret;

	stepwise.min = USEC_PER_SEC / 30;
	stepwise.max = 30 * (USEC_PER_SEC / 30);
	stepwise.step = USEC_PER_SEC / 30;

	desired = USEC_PER_SEC;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.max, "1 / 1");

	desired = 3 * (USEC_PER_SEC / 30);
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, desired, "3 / 30");

	desired = (uint32_t)(USEC_PER_SEC * 7U / 80U);
	expected = 3 * (USEC_PER_SEC / 30);
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, expected, "7 / 80");

	desired = USEC_PER_SEC / 120;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.min, "1 / 120");

	desired = 100U * USEC_PER_SEC;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.max, "100 / 1");

	/* Fine 1ms step test with large max */
	stepwise.min = USEC_PER_SEC / 60;
	stepwise.max = UINT32_MAX;
	stepwise.step = USEC_PER_MSEC;

	desired = 16667U;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.min, "16667 / 1000000");

	desired = 33333U;
	expected = stepwise.min +
		   DIV_ROUND_CLOSEST(desired - stepwise.min, stepwise.step) * stepwise.step;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, expected, "33333 / 1000000");
}

ZTEST(video_common, test_video_buffer_release_null)
{
	int ret;

	ret = video_buffer_release(NULL);
	zassert_equal(ret, -EINVAL, "expecting -EINVAL when releasing a NULL buffer");
}

ZTEST(video_common, test_video_buffer_alloc_release)
{
	struct video_buffer *vbuf;
	int ret;

	vbuf = video_buffer_alloc(64, K_NO_WAIT);
	zassert_not_null(vbuf, "expecting buffer allocation to succeed");

	ret = video_buffer_release(vbuf);
	zassert_ok(ret, "expecting buffer release to succeed");

	ret = video_buffer_release(vbuf);
	zassert_equal(ret, -EINVAL, "expecting -EINVAL when releasing a buffer twice");
}

ZTEST(video_common, test_video_buffer_release_bad_index)
{
	struct video_buffer vbuf = {.index = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX};
	int ret;

	ret = video_buffer_release(&vbuf);
	zassert_equal(ret, -EINVAL, "expecting -EINVAL for an out-of-range buffer index");
}

ZTEST_SUITE(video_common, NULL, NULL, NULL, NULL, NULL);

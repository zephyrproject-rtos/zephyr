/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/video.h>

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
	fmt.pitch = 100 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting minimum value to match");
	zassert_equal(idx, YUYV_A);

	fmt.width = 1000;
	fmt.height = 1000;
	fmt.pitch = 1000 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting maximum value to match");
	zassert_equal(idx, YUYV_A);

	fmt.width = 1920;
	fmt.height = 1080;
	fmt.pitch = 1920 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting exact match to work");
	zassert_equal(idx, YUYV_B);

	fmt.width = 1001;
	fmt.height = 1000;
	fmt.pitch = 1001 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting 1 above maximum width to mismatch");

	fmt.width = 1000;
	fmt.height = 1001;
	fmt.pitch = 1000 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting 1 above maximum height to mismatch");

	fmt.width = 1280;
	fmt.height = 720;
	fmt.pitch = 1280 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret);
	zassert_not_ok(ret, "expecting wrong format to mismatch");

	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;

	fmt.width = 1000;
	fmt.height = 1000;
	fmt.pitch = 1000 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting wrong format to mismatch");

	fmt.width = 1280;
	fmt.height = 720;
	fmt.pitch = 1280 * 2;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting exact match to work");
	zassert_equal(idx, RGB565);
}

ZTEST(video_common, test_video_frmival_nsec)
{
	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 1, .denominator = 15}),
		66666666);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 1, .denominator = 30}),
		33333333);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 5, .denominator = 1}),
		5000000000);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 1, .denominator = 1750000}),
		571);
}

ZTEST(video_common, test_video_closest_frmival_stepwise)
{
	struct video_frmival_stepwise stepwise;
	struct video_frmival desired;
	struct video_frmival expected;
	struct video_frmival match;

	stepwise.min.numerator = 1;
	stepwise.min.denominator = 30;
	stepwise.max.numerator = 30;
	stepwise.max.denominator = 30;
	stepwise.step.numerator = 1;
	stepwise.step.denominator = 30;

	desired.numerator = 1;
	desired.denominator = 1;
	video_closest_frmival_stepwise(&stepwise, &desired, &match);
	zassert_equal(video_frmival_nsec(&match), video_frmival_nsec(&desired), "1 / 1");

	desired.numerator = 3;
	desired.denominator = 30;
	video_closest_frmival_stepwise(&stepwise, &desired, &match);
	zassert_equal(video_frmival_nsec(&match), video_frmival_nsec(&desired), "3 / 30");

	desired.numerator = 7;
	desired.denominator = 80;
	expected.numerator = 3;
	expected.denominator = 30;
	video_closest_frmival_stepwise(&stepwise, &desired, &match);
	zassert_equal(video_frmival_nsec(&match), video_frmival_nsec(&expected), "7 / 80");

	desired.numerator = 1;
	desired.denominator = 120;
	video_closest_frmival_stepwise(&stepwise, &desired, &match);
	zassert_equal(video_frmival_nsec(&match), video_frmival_nsec(&stepwise.min), "1 / 120");

	desired.numerator = 100;
	desired.denominator = 1;
	video_closest_frmival_stepwise(&stepwise, &desired, &match);
	zassert_equal(video_frmival_nsec(&match), video_frmival_nsec(&stepwise.max), "100 / 1");
}

ZTEST_SUITE(video_common, NULL, NULL, NULL, NULL, NULL);

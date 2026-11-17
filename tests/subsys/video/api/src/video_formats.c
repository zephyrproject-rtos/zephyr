/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/video/video.h>

ZTEST(video_formats, test_video_formats)
{
	zassert(VIDEO_FMT_IS_GRAYSCALE(VIDEO_PIX_FMT_GREY), "is GRAYSCALE");
	zassert(VIDEO_FMT_IS_GRAYSCALE(VIDEO_PIX_FMT_Y14), "is GRAYSCALE");
	zassert(VIDEO_FMT_IS_GRAYSCALE(VIDEO_PIX_FMT_Y14P), "is GRAYSCALE");
	zassert(!VIDEO_FMT_IS_GRAYSCALE(VIDEO_PIX_FMT_RGB24), "is not GRAYSCALE");

	zassert(VIDEO_FMT_IS_BAYER(VIDEO_PIX_FMT_SRGGB8), "is BAYER");
	zassert(VIDEO_FMT_IS_BAYER(VIDEO_PIX_FMT_SBGGR8P16), "is BAYER");
	zassert(VIDEO_FMT_IS_BAYER(VIDEO_PIX_FMT_SGRBG12P), "is BAYER");
	zassert(!VIDEO_FMT_IS_BAYER(VIDEO_PIX_FMT_NV12), "is not BAYER");

	zassert(VIDEO_FMT_IS_RGB(VIDEO_PIX_FMT_RGB24), "is RGB");
	zassert(VIDEO_FMT_IS_RGB(VIDEO_PIX_FMT_ABGR32), "is RGB");
	zassert(VIDEO_FMT_IS_RGB(VIDEO_PIX_FMT_RGB565), "is RGB");
	zassert(!VIDEO_FMT_IS_RGB(VIDEO_PIX_FMT_XYUV32), "is not RGB");

	zassert(VIDEO_FMT_IS_YUV(VIDEO_PIX_FMT_YUYV), "is YUV");
	zassert(VIDEO_FMT_IS_YUV(VIDEO_PIX_FMT_NV42), "is YUV");
	zassert(VIDEO_FMT_IS_YUV(VIDEO_PIX_FMT_YUV420), "is YUV");
	zassert(!VIDEO_FMT_IS_YUV(VIDEO_PIX_FMT_BGRA32), "is not YUV");

	zassert(VIDEO_FMT_IS_MIPI_PACKED(VIDEO_PIX_FMT_Y12P), "is MIPI_PACKED");
	zassert(VIDEO_FMT_IS_MIPI_PACKED(VIDEO_PIX_FMT_SBGGR12P), "is MIPI_PACKED");
	zassert(VIDEO_FMT_IS_MIPI_PACKED(VIDEO_PIX_FMT_SGBRG10P), "is MIPI_PACKED");
	zassert(!VIDEO_FMT_IS_MIPI_PACKED(VIDEO_PIX_FMT_Y12), "is not MIPI_PACKED");

	zassert(VIDEO_FMT_IS_PADDED(VIDEO_PIX_FMT_Y8P16), "is PADDED");
	zassert(VIDEO_FMT_IS_PADDED(VIDEO_PIX_FMT_SRGGB8P16), "is PADDED");
	zassert(!VIDEO_FMT_IS_PADDED(VIDEO_PIX_FMT_GREY), "is not PADDED");

	zassert(VIDEO_FMT_IS_SEMI_PLANAR(VIDEO_PIX_FMT_NV61), "is SEMI_PLANAR");
	zassert(VIDEO_FMT_IS_SEMI_PLANAR(VIDEO_PIX_FMT_NV16), "is SEMI_PLANAR");
	zassert(VIDEO_FMT_IS_SEMI_PLANAR(VIDEO_PIX_FMT_NV12), "is SEMI_PLANAR");
	zassert(!VIDEO_FMT_IS_SEMI_PLANAR(VIDEO_PIX_FMT_YUV420), "is not SEMI_PLANAR");

	zassert(VIDEO_FMT_IS_FULL_PLANAR(VIDEO_PIX_FMT_YUV420), "is FULLY_PLANAR");
	zassert(VIDEO_FMT_IS_FULL_PLANAR(VIDEO_PIX_FMT_YVU420), "is FULLY_PLANAR");
	zassert(!VIDEO_FMT_IS_FULL_PLANAR(VIDEO_PIX_FMT_NV12), "is not FULLY_PLANAR");
}

ZTEST(video_formats, test_video_num_planes)
{
	const struct {
		uint32_t pixelformat;
		unsigned int planes;
	} cases[] = {
		{VIDEO_PIX_FMT_RGB565, 1U},
		{VIDEO_PIX_FMT_BGRX32, 1U},
		{VIDEO_PIX_FMT_RGBA32, 1U},
		{VIDEO_PIX_FMT_GREY, 1U},
		{VIDEO_PIX_FMT_SRGGB8, 1U},
		{VIDEO_PIX_FMT_JPEG, 1U},
		{VIDEO_PIX_FMT_NV12, 2U},
		{VIDEO_PIX_FMT_NV21, 2U},
		{VIDEO_PIX_FMT_NV16, 2U},
		{VIDEO_PIX_FMT_NV61, 2U},
		{VIDEO_PIX_FMT_NV24, 2U},
		{VIDEO_PIX_FMT_NV42, 2U},
		{VIDEO_PIX_FMT_RGB24PLANAR, 3U},
		{VIDEO_PIX_FMT_YUV420, 3U},
		{VIDEO_PIX_FMT_YVU420, 3U},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(cases); i++) {
		zassert_equal(video_num_planes(cases[i].pixelformat), cases[i].planes);
	}
}

ZTEST(video_formats, test_video_estimate_fmt_size)
{
	const struct {
		uint32_t pixelformat;
		uint32_t width;
		uint32_t height;
		uint32_t pitch;
		uint32_t size;
	} cases[] = {
		{VIDEO_PIX_FMT_NV12, 320U, 240U, 320U, 153600U},
		{VIDEO_PIX_FMT_NV21, 320U, 240U, 320U, 153600U},
		{VIDEO_PIX_FMT_NV16, 320U, 240U, 320U, 153600U},
		{VIDEO_PIX_FMT_NV61, 320U, 240U, 320U, 153600U},
		{VIDEO_PIX_FMT_NV24, 320U, 240U, 320U, 230400U},
		{VIDEO_PIX_FMT_NV42, 320U, 240U, 320U, 230400U},
		{VIDEO_PIX_FMT_YUV420, 320U, 240U, 320U, 230400U},
		{VIDEO_PIX_FMT_YVU420, 320U, 240U, 320U, 230400U},
		{VIDEO_PIX_FMT_RGB24PLANAR, 320U, 240U, 320U, 230400U},
		{VIDEO_PIX_FMT_NV16, 321U, 241U, 321U, 154722U},
		{VIDEO_PIX_FMT_NV61, 321U, 241U, 321U, 154722U},
		{VIDEO_PIX_FMT_YUV420, 321U, 241U, 321U, 232083U},
		{VIDEO_PIX_FMT_YVU420, 321U, 241U, 321U, 232083U},
		{VIDEO_PIX_FMT_RGB565, 320U, 240U, 640U, 153600U},
		{VIDEO_PIX_FMT_RGB24, 320U, 240U, 960U, 230400U},
		{VIDEO_PIX_FMT_RGBA32, 320U, 240U, 1280U, 307200U},
		{VIDEO_PIX_FMT_GREY, 320U, 240U, 320U, 76800U},
		{VIDEO_PIX_FMT_JPEG, 320U, 240U, 0U, 153600U},
		{VIDEO_PIX_FMT_PNG, 320U, 240U, 0U, 153600U},
		{VIDEO_PIX_FMT_H264, 320U, 240U, 0U, 153600U},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(cases); i++) {
		struct video_format fmt = {
			.pixelformat = cases[i].pixelformat,
			.width = cases[i].width,
			.height = cases[i].height,
		};

		zassert_ok(video_estimate_fmt_size(&fmt));
		zassert_equal(fmt.pitch, cases[i].pitch);
		zassert_equal(fmt.size, cases[i].size);

		if (fmt.pitch != 0U) {
			struct video_buffer vbuf = {
				.bytesused = fmt.size,
			};

			zassert_equal(video_get_buffer_height(&vbuf, &fmt), fmt.height);
		}
	}
}

ZTEST(video_formats, test_video_estimate_fmt_size_invalid_args)
{
	struct video_format fmt = {
		.pixelformat = 0U,
		.width = 320U,
		.height = 240U,
	};

	zassert_equal(video_estimate_fmt_size(NULL), -EINVAL);
	zassert_equal(video_estimate_fmt_size(&fmt), -ENOTSUP);
}

ZTEST(video_formats, test_video_get_buffer_height)
{
	const struct {
		uint32_t pixelformat;
		uint32_t pitch;
		uint32_t bytesused;
		uint32_t height;
	} cases[] = {
		{VIDEO_PIX_FMT_NV24, 320U, 230400U, 240U},
		{VIDEO_PIX_FMT_NV42, 320U, 230400U, 240U},
		{VIDEO_PIX_FMT_NV24, 320U, 28800U, 30U},
		{VIDEO_PIX_FMT_NV42, 320U, 28800U, 30U},
		{VIDEO_PIX_FMT_RGB565, 640U, 19200U, 30U},
		{VIDEO_PIX_FMT_RGB565, 672U, 20160U, 30U},
		{VIDEO_PIX_FMT_NV12, 320U, 19200U, 30U},
		{VIDEO_PIX_FMT_RGB24PLANAR, 320U, 28800U, 30U},
		{VIDEO_PIX_FMT_RGB565, 640U, 0U, 0U},
		{VIDEO_PIX_FMT_NV24, 320U, 959U, 0U},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(cases); i++) {
		struct video_format fmt = {
			.pixelformat = cases[i].pixelformat,
			.height = 240U,
			.pitch = cases[i].pitch,
		};
		struct video_buffer vbuf = {
			.bytesused = cases[i].bytesused,
		};

		zassert_equal(video_get_buffer_height(&vbuf, &fmt), cases[i].height);
	}
}

ZTEST(video_formats, test_video_get_buffer_height_invalid_args)
{
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
	};
	struct video_buffer vbuf = {
		.bytesused = 19200U,
	};

	zassert_equal(video_get_buffer_height(NULL, &fmt), 0U);
	zassert_equal(video_get_buffer_height(&vbuf, NULL), 0U);
	zassert_equal(video_get_buffer_height(&vbuf, &fmt), 0U);
}

ZTEST_SUITE(video_formats, NULL, NULL, NULL, NULL, NULL);

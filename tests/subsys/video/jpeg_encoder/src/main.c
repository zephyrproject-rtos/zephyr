/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Checks the JPEG images produced by the zephyr,videoenc device by parsing their markers on
 * the target itself. Input frames are synthesized, so no camera and no host tools are needed.
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_video_jpeg_encoder, LOG_LEVEL_INF);

#define WIDTH      64
#define HEIGHT     48
#define MAX_WIDTH  320
#define MAX_HEIGHT 240

#define JPEG_SOI  0xd8
#define JPEG_EOI  0xd9
#define JPEG_SOF0 0xc0
#define JPEG_DHT  0xc4
#define JPEG_SOS  0xda
#define JPEG_DQT  0xdb

struct jpeg_info {
	uint16_t width;
	uint16_t height;
	uint8_t components;
	bool sof0;
	bool dqt;
	bool dht;
	bool sos;
};

static const struct device *const enc = DEVICE_DT_GET(DT_CHOSEN(zephyr_videoenc));
static struct video_buffer *in_buf;
static struct video_buffer *out_buf;
static uint32_t in_pixfmt;
static uint32_t width;
static uint32_t height;

/* Reference image to compare other input formats against */
static uint8_t ref[8192];
static size_t ref_len;

/* Parse the markers before the scan data, and check the start and end of the image */
static void parse_jpeg(const uint8_t *p, size_t len, struct jpeg_info *info)
{
	size_t i = 2;

	memset(info, 0, sizeof(*info));

	zassert_true(len > 4, "image of %zu bytes", len);
	zassert_true(p[0] == 0xff && p[1] == JPEG_SOI, "no start of image marker");
	zassert_true(p[len - 2] == 0xff && p[len - 1] == JPEG_EOI, "no end of image marker");

	while (i + 4 <= len && !info->sos) {
		uint8_t marker = p[i + 1];
		uint16_t seg_len = sys_get_be16(&p[i + 2]);

		zassert_equal(p[i], 0xff, "no marker at offset %zu", i);
		zassert_true(i + 2 + seg_len <= len, "segment 0x%02x overruns the image", marker);

		switch (marker) {
		case JPEG_SOF0:
			info->sof0 = true;
			info->height = sys_get_be16(&p[i + 5]);
			info->width = sys_get_be16(&p[i + 7]);
			info->components = p[i + 9];
			break;
		case JPEG_DQT:
			info->dqt = true;
			break;
		case JPEG_DHT:
			info->dht = true;
			break;
		case JPEG_SOS:
			info->sos = true;
			break;
		default:
			break;
		}
		i += 2 + seg_len;
	}

	zassert_true(info->sof0, "no baseline frame header");
	zassert_true(info->dqt && info->dht && info->sos, "missing DQT, DHT or SOS");
}

static void sync_buf(struct video_buffer *vbuf)
{
	(void)sys_cache_data_flush_and_invd_range(vbuf->buffer, vbuf->size);
}

/* Detailed luma, and chroma that is constant over each 2x2 block */
static uint8_t luma(uint32_t x, uint32_t y, uint32_t n)
{
	return (uint8_t)((x * 37) ^ (y * 17)) + n;
}

static uint8_t cb(uint32_t x, uint32_t y)
{
	return 96 + ((x / 2) * 8) % 64;
}

static uint8_t cr(uint32_t x, uint32_t y)
{
	return 160 - ((y / 2) * 8) % 64;
}

/* Write the same picture in the current input pixel format */
static void fill_frame(uint32_t n)
{
	uint8_t *p = in_buf->buffer;
	uint32_t w = width;
	uint32_t h = height;

	switch (in_pixfmt) {
	case VIDEO_PIX_FMT_NV12:
	case VIDEO_PIX_FMT_YUV420:
		for (uint32_t y = 0; y < h; y++) {
			for (uint32_t x = 0; x < w; x++) {
				p[y * w + x] = luma(x, y, n);
			}
		}
		for (uint32_t y = 0; y < h / 2; y++) {
			for (uint32_t x = 0; x < w / 2; x++) {
				if (in_pixfmt == VIDEO_PIX_FMT_NV12) {
					p[w * h + y * w + 2 * x] = cb(2 * x, 2 * y);
					p[w * h + y * w + 2 * x + 1] = cr(2 * x, 2 * y);
				} else {
					p[w * h + y * w / 2 + x] = cb(2 * x, 2 * y);
					p[w * h * 5 / 4 + y * w / 2 + x] = cr(2 * x, 2 * y);
				}
			}
		}
		in_buf->bytesused = w * h * 3 / 2;
		break;
	case VIDEO_PIX_FMT_UYVY:
	case VIDEO_PIX_FMT_YUYV:
		for (uint32_t y = 0; y < h; y++) {
			for (uint32_t x = 0; x < w; x += 2) {
				uint8_t *q = &p[(y * w + x) * 2];

				if (in_pixfmt == VIDEO_PIX_FMT_UYVY) {
					q[0] = cb(x, y);
					q[1] = luma(x, y, n);
					q[2] = cr(x, y);
					q[3] = luma(x + 1, y, n);
				} else {
					q[0] = luma(x, y, n);
					q[1] = cb(x, y);
					q[2] = luma(x + 1, y, n);
					q[3] = cr(x, y);
				}
			}
		}
		in_buf->bytesused = w * h * 2;
		break;
	case VIDEO_PIX_FMT_RGB565:
	case VIDEO_PIX_FMT_RGB565X:
		for (uint32_t i = 0; i < w * h; i++) {
			uint16_t px = (uint16_t)((i % w) * 2048 / w) << 5 | (i / w) % 32;

			if (in_pixfmt == VIDEO_PIX_FMT_RGB565) {
				sys_put_le16(px, &p[i * 2]);
			} else {
				sys_put_be16(px, &p[i * 2]);
			}
		}
		in_buf->bytesused = w * h * 2;
		break;
	case VIDEO_PIX_FMT_GREY:
		for (uint32_t y = 0; y < h; y++) {
			for (uint32_t x = 0; x < w; x++) {
				p[y * w + x] = luma(x, y, n);
			}
		}
		in_buf->bytesused = w * h;
		break;
	default:
		zassert_unreachable("format %s", VIDEO_FOURCC_TO_STR(in_pixfmt));
	}
	sync_buf(in_buf);
}

/* Set the input and output formats; the stream is stopped */
static void configure(uint32_t pixfmt, uint32_t w, uint32_t h)
{
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_INPUT,
		.pixelformat = pixfmt,
		.width = w,
		.height = h,
	};

	zassert_ok(video_set_format(enc, &fmt), "input %s %ux%u", VIDEO_FOURCC_TO_STR(pixfmt), w,
		   h);
	zassert_true(fmt.size <= in_buf->size);

	fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	fmt.pixelformat = VIDEO_PIX_FMT_JPEG;
	zassert_ok(video_set_format(enc, &fmt));

	in_pixfmt = pixfmt;
	width = w;
	height = h;
}

/* Encode one frame and check the image structure */
static void encode(uint32_t n, struct jpeg_info *info)
{
	struct video_buffer *vbuf;

	fill_frame(n);
	sync_buf(out_buf);

	out_buf->type = VIDEO_BUF_TYPE_OUTPUT;
	zassert_ok(video_enqueue(enc, out_buf));
	in_buf->type = VIDEO_BUF_TYPE_INPUT;
	zassert_ok(video_enqueue(enc, in_buf));

	vbuf = &(struct video_buffer){.type = VIDEO_BUF_TYPE_OUTPUT};
	zassert_ok(video_dequeue(enc, &vbuf, K_SECONDS(1)), "frame %u: no output", n);
	zassert_equal_ptr(vbuf, out_buf);

	vbuf = &(struct video_buffer){.type = VIDEO_BUF_TYPE_INPUT};
	zassert_ok(video_dequeue(enc, &vbuf, K_SECONDS(1)), "frame %u: input not returned", n);
	zassert_equal_ptr(vbuf, in_buf);

	sync_buf(out_buf);
	zassert_true(out_buf->bytesused > 0, "frame %u: empty output", n);
	zassert_true(out_buf->bytesused <= out_buf->size);

	parse_jpeg(out_buf->buffer, out_buf->bytesused, info);
	zassert_equal(info->width, width, "width %u", info->width);
	zassert_equal(info->height, height, "height %u", info->height);
}

static void stream_start(void)
{
	zassert_ok(video_stream_start(enc, VIDEO_BUF_TYPE_INPUT));
	zassert_ok(video_stream_start(enc, VIDEO_BUF_TYPE_OUTPUT));
}

static void stream_stop(void)
{
	(void)video_stream_stop(enc, VIDEO_BUF_TYPE_OUTPUT);
	(void)video_stream_stop(enc, VIDEO_BUF_TYPE_INPUT);
}

static bool input_supported(uint32_t pixfmt)
{
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_INPUT};

	zassert_ok(video_get_caps(enc, &caps));
	for (const struct video_format_cap *c = caps.format_caps; c->pixelformat != 0; c++) {
		if (c->pixelformat == pixfmt) {
			return true;
		}
	}
	return false;
}

static void set_quality(int32_t quality)
{
	struct video_control ctrl = {.id = VIDEO_CID_JPEG_COMPRESSION_QUALITY, .val = quality};

	zassert_ok(video_set_ctrl(enc, &ctrl), "quality %d", quality);
}

static void *jpeg_setup(void)
{
	zassert_true(device_is_ready(enc), "%s not ready", enc->name);

	in_buf = video_buffer_aligned_alloc(MAX_WIDTH * MAX_HEIGHT * 2,
					    CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_NO_WAIT);
	zassert_not_null(in_buf);

	out_buf = video_buffer_aligned_alloc(MAX_WIDTH * MAX_HEIGHT * 2,
					     CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_NO_WAIT);
	zassert_not_null(out_buf);

	return NULL;
}

static void jpeg_before(void *fixture)
{
	ARG_UNUSED(fixture);

	configure(VIDEO_PIX_FMT_NV12, WIDTH, HEIGHT);
	stream_start();
}

static void jpeg_after(void *fixture)
{
	struct video_ctrl_query cq = {.dev = enc, .id = VIDEO_CID_JPEG_COMPRESSION_QUALITY};

	ARG_UNUSED(fixture);

	stream_stop();
	if (video_query_ctrl(&cq) == 0) {
		set_quality(cq.range.def);
	}
}

ZTEST(video_jpeg_encoder, test_image_structure)
{
	struct jpeg_info info;

	encode(0, &info);
	zassert_equal(info.components, 3, "%u components", info.components);
	LOG_INF("%ux%u NV12: %u bytes", WIDTH, HEIGHT, out_buf->bytesused);
}

/* Each input buffer gives a complete image, also across stream restarts */
ZTEST(video_jpeg_encoder, test_consecutive_images)
{
	struct jpeg_info info;

	for (uint32_t n = 0; n < 5; n++) {
		encode(n, &info);
	}

	stream_stop();
	stream_start();
	encode(5, &info);
}

/* Higher quality must not give smaller images, and the extremes must differ */
ZTEST(video_jpeg_encoder, test_quality)
{
	static const int32_t qualities[] = {5, 25, 50, 75, 90, 100};
	struct video_ctrl_query cq = {.dev = enc, .id = VIDEO_CID_JPEG_COMPRESSION_QUALITY};
	struct jpeg_info info;
	uint32_t prev = 0;
	uint32_t first = 0;

	if (video_query_ctrl(&cq) < 0) {
		ztest_test_skip();
	}

	ARRAY_FOR_EACH(qualities, i) {
		int32_t q = CLAMP(qualities[i], cq.range.min, cq.range.max);

		/* Applies from the next image, without restarting the stream */
		set_quality(q);
		encode(0, &info);
		LOG_INF("quality %d: %u bytes", q, out_buf->bytesused);

		zassert_true(out_buf->bytesused >= prev, "quality %d: %u bytes < %u", q,
			     out_buf->bytesused, prev);
		prev = out_buf->bytesused;
		if (i == 0) {
			first = prev;
		}
	}

	zassert_true(prev > first, "quality has no effect: %u bytes", prev);
}

/*
 * Every advertised input format must give a valid image. The YUV formats carry the same
 * picture, so they must give the same image as NV12.
 */
ZTEST(video_jpeg_encoder, test_input_formats)
{
	static const uint32_t formats[] = {
		VIDEO_PIX_FMT_NV12,   VIDEO_PIX_FMT_YUV420,  VIDEO_PIX_FMT_UYVY, VIDEO_PIX_FMT_YUYV,
		VIDEO_PIX_FMT_RGB565, VIDEO_PIX_FMT_RGB565X, VIDEO_PIX_FMT_GREY,
	};
	struct jpeg_info info;

	ARRAY_FOR_EACH(formats, i) {
		uint32_t pixfmt = formats[i];
		bool yuv = pixfmt != VIDEO_PIX_FMT_RGB565 && pixfmt != VIDEO_PIX_FMT_RGB565X &&
			   pixfmt != VIDEO_PIX_FMT_GREY;

		if (!input_supported(pixfmt)) {
			LOG_INF("%s: not supported", VIDEO_FOURCC_TO_STR(pixfmt));
			continue;
		}

		stream_stop();
		configure(pixfmt, WIDTH, HEIGHT);
		stream_start();
		encode(0, &info);

		zassert_equal(info.components, pixfmt == VIDEO_PIX_FMT_GREY ? 1 : 3,
			      "%s: %u components", VIDEO_FOURCC_TO_STR(pixfmt), info.components);
		LOG_INF("%s: %u bytes", VIDEO_FOURCC_TO_STR(pixfmt), out_buf->bytesused);

		if (pixfmt == VIDEO_PIX_FMT_NV12) {
			zassert_true(out_buf->bytesused <= sizeof(ref));
			memcpy(ref, out_buf->buffer, out_buf->bytesused);
			ref_len = out_buf->bytesused;
		} else if (yuv) {
			zassert_true(out_buf->bytesused == ref_len &&
					     memcmp(out_buf->buffer, ref, ref_len) == 0,
				     "%s gives a different image than NV12",
				     VIDEO_FOURCC_TO_STR(pixfmt));
		}
	}
}

/* The smallest size and a typical one */
ZTEST(video_jpeg_encoder, test_sizes)
{
	static const uint16_t sizes[][2] = {{16, 16}, {MAX_WIDTH, MAX_HEIGHT}};
	struct jpeg_info info;

	ARRAY_FOR_EACH(sizes, i) {
		stream_stop();
		configure(VIDEO_PIX_FMT_NV12, sizes[i][0], sizes[i][1]);
		stream_start();
		encode(0, &info);
		LOG_INF("%ux%u: %u bytes", sizes[i][0], sizes[i][1], out_buf->bytesused);
	}
}

ZTEST_SUITE(video_jpeg_encoder, NULL, jpeg_setup, jpeg_before, jpeg_after, NULL);

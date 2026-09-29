/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Checks the H.264 bitstream produced by the zephyr,videoenc device, by
 * parsing NAL unit headers of every output buffer on the target itself.
 * Input frames are synthesized, so no camera and no host tools are needed.
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_video_encoder, LOG_LEVEL_INF);

#define WIDTH  320
#define HEIGHT 240

/* Current default of the in-tree H.264 encoders: one IDR every 30 frames */
#define DEFAULT_GOP_SIZE 30

#define NAL_SLICE 1
#define NAL_IDR   5
#define NAL_SPS   7
#define NAL_PPS   8

struct nal_stats {
	unsigned int slice;
	unsigned int idr;
	unsigned int sps;
	unsigned int pps;
	unsigned int other;
};

static const struct device *const enc = DEVICE_DT_GET(DT_CHOSEN(zephyr_videoenc));
static struct video_buffer *in_buf;
static struct video_buffer *out_buf;

static void scan_nals(const uint8_t *p, size_t len, struct nal_stats *s)
{
	memset(s, 0, sizeof(*s));

	for (size_t i = 0; i + 3 < len; i++) {
		if (p[i] != 0 || p[i + 1] != 0 || p[i + 2] != 1) {
			continue;
		}

		switch (p[i + 3] & 0x1f) {
		case NAL_SLICE:
			s->slice++;
			break;
		case NAL_IDR:
			s->idr++;
			break;
		case NAL_SPS:
			s->sps++;
			break;
		case NAL_PPS:
			s->pps++;
			break;
		default:
			s->other++;
			break;
		}
		i += 3;
	}
}

/*
 * The buffers are shared with the encoder hardware: keep them coherent in
 * case the data cache is enabled. This is a no-op otherwise.
 */
static void sync_buf(struct video_buffer *vbuf)
{
	(void)sys_cache_data_flush_and_invd_range(vbuf->buffer, vbuf->size);
}

/* NV12 gradient that moves with the frame number, so P frames are not empty */
static void fill_frame(struct video_buffer *vbuf, uint32_t n)
{
	uint8_t *luma = vbuf->buffer;
	uint8_t *chroma = luma + WIDTH * HEIGHT;

	for (uint32_t y = 0; y < HEIGHT; y++) {
		for (uint32_t x = 0; x < WIDTH; x++) {
			luma[y * WIDTH + x] = (uint8_t)(x + y + 4 * n);
		}
	}
	memset(chroma, 128, WIDTH * HEIGHT / 2);
	vbuf->bytesused = WIDTH * HEIGHT * 3 / 2;
	sync_buf(vbuf);
}

/* Push one input frame, return the NAL units found in the output buffer */
static void encode(uint32_t n, struct nal_stats *s)
{
	struct video_buffer *vbuf;

	fill_frame(in_buf, n);
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
	scan_nals(out_buf->buffer, out_buf->bytesused, s);

	LOG_DBG("frame %u: %u bytes, slice=%u idr=%u sps=%u pps=%u other=%u", n, out_buf->bytesused,
		s->slice, s->idr, s->sps, s->pps, s->other);
}

static void *encoder_setup(void)
{
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
		.pixelformat = VIDEO_PIX_FMT_H264,
		.width = WIDTH,
		.height = HEIGHT,
	};

	zassert_true(device_is_ready(enc), "%s not ready", enc->name);

	zassert_ok(video_set_format(enc, &fmt));

	fmt.type = VIDEO_BUF_TYPE_INPUT;
	fmt.pixelformat = VIDEO_PIX_FMT_NV12;
	zassert_ok(video_set_format(enc, &fmt));

	in_buf = video_buffer_aligned_alloc(WIDTH * HEIGHT * 3 / 2, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
					    K_NO_WAIT);
	zassert_not_null(in_buf);

	/* Leave room above the driver estimate for intra frames */
	out_buf = video_buffer_aligned_alloc(WIDTH * HEIGHT, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
					     K_NO_WAIT);
	zassert_not_null(out_buf);

	return NULL;
}

static void encoder_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(video_stream_start(enc, VIDEO_BUF_TYPE_INPUT));
	zassert_ok(video_stream_start(enc, VIDEO_BUF_TYPE_OUTPUT));
}

static void encoder_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)video_stream_stop(enc, VIDEO_BUF_TYPE_OUTPUT);
	(void)video_stream_stop(enc, VIDEO_BUF_TYPE_INPUT);
}

/* The first output buffer of a stream only carries the SPS and PPS */
static void expect_stream_header(void)
{
	struct nal_stats s;

	encode(0, &s);
	zassert_equal(s.sps, 1, "stream header: %u SPS", s.sps);
	zassert_equal(s.pps, 1, "stream header: %u PPS", s.pps);
	zassert_equal(s.slice + s.idr, 0, "stream header carries picture data");
}

ZTEST(video_encoder, test_stream_header)
{
	expect_stream_header();
}

ZTEST(video_encoder, test_default_gop)
{
	struct nal_stats s;

	expect_stream_header();

	for (uint32_t n = 0; n <= 2 * DEFAULT_GOP_SIZE; n++) {
		bool key = (n % DEFAULT_GOP_SIZE) == 0;

		encode(n + 1, &s);
		zassert_equal(s.idr + s.slice, 1, "frame %u: %u IDR + %u non-IDR slices", n, s.idr,
			      s.slice);
		zassert_equal(s.idr, key ? 1 : 0, "frame %u: expected %s", n,
			      key ? "IDR" : "non-IDR");

		if (key) {
			LOG_INF("frame %u: IDR, %u bytes, SPS/PPS repeated: %s", n,
				out_buf->bytesused, s.sps > 0 && s.pps > 0 ? "yes" : "no");
		}
	}
}

/* Stopping and restarting the stream starts a new, self-contained bitstream */
ZTEST(video_encoder, test_restart)
{
	struct nal_stats s;

	expect_stream_header();
	encode(1, &s);
	zassert_equal(s.idr, 1, "first frame is not an IDR");

	encoder_after(NULL);
	encoder_before(NULL);

	expect_stream_header();
	encode(2, &s);
	zassert_equal(s.idr, 1, "first frame after restart is not an IDR");
}

ZTEST_SUITE(video_encoder, NULL, encoder_setup, encoder_before, encoder_after, NULL);

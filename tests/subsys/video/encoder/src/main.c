/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Checks the H.264 bitstream produced by the zephyr,videoenc device, by
 * parsing NAL unit headers of every output buffer on the target itself.
 * Input frames are synthesized, so no camera and no host tools are needed.
 */

#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/controls.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(test_video_encoder, LOG_LEVEL_INF);

#define WIDTH  256
#define HEIGHT 192

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

/* Controls and frame interval changed by a test case, restored after it */
static struct video_control saved_ctrls[8];
static size_t num_saved_ctrls;
static struct video_frmival saved_frmival;
static bool has_saved_frmival;

/* Pixel format of the input frames */
static uint32_t in_pixfmt = VIDEO_PIX_FMT_NV12;

/* Content of the input frames */
static enum {
	CONTENT_GRADIENT,
	CONTENT_NOISE,
} content;

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

/*
 * NV12 gradient that moves with the frame number, so P frames are not empty, or noise that
 * rate control has to work on.
 */
static uint8_t luma_sample(uint32_t x, uint32_t y, uint32_t n, uint32_t *seed)
{
	if (content == CONTENT_NOISE) {
		*seed = *seed * 1103515245u + 12345u;
		return (uint8_t)(x + y + (*seed >> 28));
	}

	return (uint8_t)(x + y + 4 * n);
}

/* Write the same picture, with flat chroma, in the current input pixel format */
static void fill_frame(struct video_buffer *vbuf, uint32_t n)
{
	uint8_t *p = vbuf->buffer;
	uint32_t seed = n * 2654435761u + 1;
	bool packed = in_pixfmt == VIDEO_PIX_FMT_YUYV || in_pixfmt == VIDEO_PIX_FMT_UYVY;
	/* Offset of the luma sample within each pair of bytes of packed formats */
	uint32_t yoff = in_pixfmt == VIDEO_PIX_FMT_UYVY ? 1 : 0;

	for (uint32_t y = 0; y < HEIGHT; y++) {
		for (uint32_t x = 0; x < WIDTH; x++) {
			uint8_t l = luma_sample(x, y, n, &seed);

			if (packed) {
				p[(y * WIDTH + x) * 2 + yoff] = l;
				p[(y * WIDTH + x) * 2 + 1 - yoff] = 128;
			} else {
				p[y * WIDTH + x] = l;
			}
		}
	}

	if (packed) {
		vbuf->bytesused = WIDTH * HEIGHT * 2;
	} else {
		/* NV12 and YUV420 have the same chroma size */
		memset(p + WIDTH * HEIGHT, 128, WIDTH * HEIGHT / 2);
		vbuf->bytesused = WIDTH * HEIGHT * 3 / 2;
	}
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
	size_t out_size;

	zassert_true(device_is_ready(enc), "%s not ready", enc->name);

	zassert_ok(video_set_format(enc, &fmt));
	/* Leave room above the driver estimate for intra frames */
	out_size = MAX(fmt.size, WIDTH * HEIGHT);

	fmt.type = VIDEO_BUF_TYPE_INPUT;
	fmt.pixelformat = VIDEO_PIX_FMT_NV12;
	zassert_ok(video_set_format(enc, &fmt));

	/* Large enough for packed 4:2:2 formats */
	in_buf = video_buffer_aligned_alloc(WIDTH * HEIGHT * 2, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
					    K_NO_WAIT);
	zassert_not_null(in_buf);

	out_buf = video_buffer_aligned_alloc(out_size, CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_NO_WAIT);
	zassert_not_null(out_buf);

	return NULL;
}

static void encoder_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(video_stream_start(enc, VIDEO_BUF_TYPE_INPUT));
	zassert_ok(video_stream_start(enc, VIDEO_BUF_TYPE_OUTPUT));
}

static void stream_stop(void)
{
	(void)video_stream_stop(enc, VIDEO_BUF_TYPE_OUTPUT);
	(void)video_stream_stop(enc, VIDEO_BUF_TYPE_INPUT);
}

static void encoder_after(void *fixture)
{
	ARG_UNUSED(fixture);

	stream_stop();

	if (has_saved_frmival) {
		(void)video_set_frmival(enc, &saved_frmival);
		has_saved_frmival = false;
	}

	/* Restore the controls in reverse order */
	while (num_saved_ctrls > 0) {
		num_saved_ctrls--;
		(void)video_set_ctrl(enc, &saved_ctrls[num_saved_ctrls]);
	}
	content = CONTENT_GRADIENT;
}

/* Skip the test case if the encoder doesn't implement the control */
static void require_ctrl(uint32_t id)
{
	struct video_ctrl_query cq = {.dev = enc, .id = id};

	if (video_query_ctrl(&cq) < 0) {
		ztest_test_skip();
	}
}

/* Set a control, saving its value first so that it is restored after the test case */
static void set_ctrl(uint32_t id, int32_t val)
{
	struct video_ctrl_query cq = {.dev = enc, .id = id};
	struct video_control ctrl = {.id = id};

	zassert_ok(video_query_ctrl(&cq));
	if (cq.type != VIDEO_CTRL_TYPE_BUTTON) {
		zassert_true(num_saved_ctrls < ARRAY_SIZE(saved_ctrls));
		zassert_ok(video_get_ctrl(enc, &ctrl));
		saved_ctrls[num_saved_ctrls++] = ctrl;
	}

	ctrl.val = val;
	zassert_ok(video_set_ctrl(enc, &ctrl), "control 0x%08x = %d", id, val);
}

/* Set a control that can't change while streaming */
static void set_ctrl_stopped(uint32_t id, int32_t val)
{
	stream_stop();
	set_ctrl(id, val);
	encoder_before(NULL);
}

/* Check that a control the driver can't change while streaming is rejected */
static void expect_busy_while_streaming(uint32_t id, int32_t val)
{
	struct video_control ctrl = {.id = id, .val = val};

	zassert_equal(video_set_ctrl(enc, &ctrl), -EBUSY, "control 0x%08x", id);
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

ZTEST(video_encoder, test_frmival)
{
	struct video_frmival desired = {.numerator = 1, .denominator = 15};
	struct video_frmival got;
	struct nal_stats s;

	if (video_get_frmival(enc, &saved_frmival) == -ENOSYS) {
		ztest_test_skip();
	}
	has_saved_frmival = true;

	stream_stop();
	zassert_ok(video_set_frmival(enc, &desired));
	zassert_ok(video_get_frmival(enc, &got));
	zassert_equal(video_frmival_nsec(&got), video_frmival_nsec(&desired));

	encoder_before(NULL);
	expect_stream_header();
	encode(1, &s);
	zassert_equal(s.idr, 1);
}

ZTEST(video_encoder, test_force_key_frame)
{
	struct nal_stats s;

	require_ctrl(VIDEO_CID_CODEC_FORCE_KEY_FRAME);

	expect_stream_header();
	for (uint32_t n = 0; n < 5; n++) {
		encode(n + 1, &s);
	}
	zassert_equal(s.idr, 0, "frame 4 is an IDR");

	set_ctrl(VIDEO_CID_CODEC_FORCE_KEY_FRAME, 0);
	encode(6, &s);
	zassert_equal(s.idr, 1, "no IDR after the key frame request");
	encode(7, &s);
	zassert_equal(s.idr, 0, "IDR repeated after the key frame request");
}

ZTEST(video_encoder, test_gop_size)
{
	const uint32_t gop = 10;
	struct nal_stats s;

	require_ctrl(VIDEO_CID_CODEC_GOP_SIZE);

	set_ctrl(VIDEO_CID_CODEC_GOP_SIZE, gop);
	expect_stream_header();
	for (uint32_t n = 0; n <= 2 * gop; n++) {
		encode(n + 1, &s);
		zassert_equal(s.idr, n % gop == 0 ? 1 : 0, "frame %u", n);
	}

	/* 0: only the first frame is an IDR */
	set_ctrl_stopped(VIDEO_CID_CODEC_GOP_SIZE, 0);
	expect_stream_header();
	for (uint32_t n = 0; n <= 2 * DEFAULT_GOP_SIZE; n++) {
		encode(n + 1, &s);
		zassert_equal(s.idr, n == 0 ? 1 : 0, "frame %u", n);
	}
}

ZTEST(video_encoder, test_header_joined)
{
	struct video_ctrl_query cq = {.dev = enc, .id = VIDEO_CID_CODEC_HEADER_MODE};
	struct nal_stats s;

	require_ctrl(VIDEO_CID_CODEC_HEADER_MODE);
	zassert_ok(video_query_ctrl(&cq));
	if (!IN_RANGE(VIDEO_CODEC_HEADER_MODE_JOINED_WITH_1ST_FRAME, cq.range.min, cq.range.max)) {
		ztest_test_skip();
	}
	expect_busy_while_streaming(VIDEO_CID_CODEC_HEADER_MODE,
				    VIDEO_CODEC_HEADER_MODE_JOINED_WITH_1ST_FRAME);

	set_ctrl_stopped(VIDEO_CID_CODEC_HEADER_MODE,
			 VIDEO_CODEC_HEADER_MODE_JOINED_WITH_1ST_FRAME);
	encode(0, &s);
	zassert_true(s.sps == 1 && s.pps == 1 && s.idr == 1, "first buffer: %u SPS, %u PPS, %u IDR",
		     s.sps, s.pps, s.idr);
	encode(1, &s);
	zassert_true(s.sps == 0 && s.slice == 1, "second buffer: %u SPS, %u slices", s.sps,
		     s.slice);
}

ZTEST(video_encoder, test_prepend_spspps)
{
	const uint32_t gop = 5;
	struct nal_stats s;

	require_ctrl(VIDEO_CID_CODEC_PREPEND_SPSPPS_TO_IDR);
	require_ctrl(VIDEO_CID_CODEC_GOP_SIZE);

	set_ctrl_stopped(VIDEO_CID_CODEC_PREPEND_SPSPPS_TO_IDR, 1);
	set_ctrl(VIDEO_CID_CODEC_GOP_SIZE, gop);
	expect_stream_header();
	for (uint32_t n = 0; n <= 2 * gop; n++) {
		bool key = n % gop == 0;

		encode(n + 1, &s);
		zassert_equal(s.idr, key ? 1 : 0, "frame %u", n);
		if (n == 0) {
			/*
			 * When the stream header buffer immediately precedes frame 0, hardware
			 * encoders may omit duplicating SPS/PPS on frame 0 while repeating them
			 * on every subsequent IDR frame.
			 */
			zassert_equal(s.sps, s.pps, "frame 0: %u SPS vs %u PPS", s.sps, s.pps);
			zassert_true(s.sps <= 1, "frame 0: %u SPS", s.sps);
		} else {
			zassert_equal(s.sps, key ? 1 : 0, "frame %u: %u SPS", n, s.sps);
			zassert_equal(s.pps, key ? 1 : 0, "frame %u: %u PPS", n, s.pps);
		}
	}
}

/* profile_idc, constraint flags and level_idc follow the NAL header of the SPS */
static void read_sps(const uint8_t *p, size_t len, uint8_t sps[3])
{
	for (size_t i = 0; i + 6 < len; i++) {
		if (p[i] != 0 && p[i + 1] != 0) {
			continue;
		}
		if (p[i] == 0 && p[i + 1] == 0 && p[i + 2] == 1 && (p[i + 3] & 0x1f) == NAL_SPS) {
			memcpy(sps, &p[i + 4], 3);
			return;
		}
	}
	zassert_unreachable("no SPS");
}

static void check_profile_level(int32_t profile, int32_t level, uint8_t profile_idc,
				bool constrained, uint8_t level_idc)
{
	struct video_ctrl_query cq = {.dev = enc, .id = VIDEO_CID_H264_PROFILE};
	uint8_t sps[3] = {0};

	zassert_ok(video_query_ctrl(&cq));
	if (!IN_RANGE(profile, cq.range.min, cq.range.max)) {
		TC_PRINT("profile %d not supported\n", profile);
		return;
	}

	set_ctrl_stopped(VIDEO_CID_H264_PROFILE, profile);
	set_ctrl_stopped(VIDEO_CID_H264_LEVEL, level);
	encode(0, &(struct nal_stats){0});
	read_sps(out_buf->buffer, out_buf->bytesused, sps);

	TC_PRINT("profile %d level %d: profile_idc %u, constraints 0x%02x, level_idc %u\n", profile,
		 level, sps[0], sps[1], sps[2]);
	zassert_equal(sps[0], profile_idc);
	if (profile_idc == 66) {
		/* constraint_set1_flag tells Constrained Baseline from Baseline */
		zassert_equal((sps[1] & 0x40) != 0, constrained, "constraint_set1_flag");
	}
	zassert_equal(sps[2], level_idc);
}

ZTEST(video_encoder, test_profile_level)
{
	require_ctrl(VIDEO_CID_H264_PROFILE);
	require_ctrl(VIDEO_CID_H264_LEVEL);
	expect_busy_while_streaming(VIDEO_CID_H264_LEVEL, VIDEO_H264_LEVEL_3_1);

	check_profile_level(VIDEO_H264_PROFILE_CONSTRAINED_BASELINE, VIDEO_H264_LEVEL_3_1, 66, true,
			    31);
	check_profile_level(VIDEO_H264_PROFILE_MAIN, VIDEO_H264_LEVEL_4_0, 77, false, 40);
	check_profile_level(VIDEO_H264_PROFILE_HIGH, VIDEO_H264_LEVEL_4_1, 100, false, 41);
}

/* Mean size of the non-IDR frames in [first, last] */
static size_t mean_p_size(uint32_t first, uint32_t last)
{
	struct nal_stats s;
	size_t total = 0;
	uint32_t count = 0;

	for (uint32_t n = first; n <= last; n++) {
		encode(n, &s);
		if (s.idr == 0) {
			total += out_buf->bytesused;
			count++;
		}
	}
	zassert_true(count > 0);

	return total / count;
}

ZTEST(video_encoder, test_bitrate)
{
	struct video_ctrl_query rc_cq = {.dev = enc, .id = VIDEO_CID_CODEC_FRAME_RC_ENABLE};
	const int32_t high = 1200000;
	const int32_t low = 300000;
	size_t high_size, low_size;

	require_ctrl(VIDEO_CID_CODEC_BITRATE);
	if (video_query_ctrl(&rc_cq) == 0) {
		set_ctrl_stopped(VIDEO_CID_CODEC_FRAME_RC_ENABLE, 1);
	}

	content = CONTENT_NOISE;
	set_ctrl(VIDEO_CID_CODEC_BITRATE, high);
	expect_stream_header();
	(void)mean_p_size(1, 15);
	high_size = mean_p_size(16, 30);

	/* Lower the bitrate while streaming */
	set_ctrl(VIDEO_CID_CODEC_BITRATE, low);
	(void)mean_p_size(31, 45);
	low_size = mean_p_size(46, 60);

	TC_PRINT("mean P frame: %zu bytes at %d bit/s (target %d), %zu bytes at %d bit/s "
		 "(target %d)\n",
		 high_size, high, high / 8 / 30, low_size, low, low / 8 / 30);

	/* Loose bands: rate control needs a few frames to settle */
	zassert_true(IN_RANGE(high_size, high / 8 / 30 / 3, high / 8 / 30 * 3));
	zassert_true(IN_RANGE(low_size, low / 8 / 30 / 3, low / 8 / 30 * 3));
	zassert_true(low_size * 2 < high_size, "bitrate change had no effect");
}

ZTEST(video_encoder, test_constant_qp)
{
	size_t fine, coarse;

	require_ctrl(VIDEO_CID_CODEC_FRAME_RC_ENABLE);
	require_ctrl(VIDEO_CID_H264_P_FRAME_QP);

	content = CONTENT_NOISE;
	set_ctrl_stopped(VIDEO_CID_CODEC_FRAME_RC_ENABLE, 0);

	set_ctrl(VIDEO_CID_H264_P_FRAME_QP, 20);
	expect_stream_header();
	fine = mean_p_size(1, 5);

	set_ctrl(VIDEO_CID_H264_P_FRAME_QP, 40);
	(void)mean_p_size(6, 6);
	coarse = mean_p_size(7, 11);

	TC_PRINT("mean P frame: %zu bytes at QP 20, %zu bytes at QP 40\n", fine, coarse);
	zassert_true(coarse * 2 < fine, "P frame QP had no effect");
}

static void set_input_format(uint32_t pixfmt)
{
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_INPUT,
		.pixelformat = pixfmt,
		.width = WIDTH,
		.height = HEIGHT,
	};

	zassert_ok(video_set_format(enc, &fmt), "input format %s", VIDEO_FOURCC_TO_STR(pixfmt));
	in_pixfmt = pixfmt;
}

/* Encode a short stream and return a checksum of it */
static uint32_t stream_checksum(void)
{
	struct nal_stats s;
	uint32_t sum = 0;

	for (uint32_t n = 0; n < 4; n++) {
		encode(n, &s);
		for (uint32_t i = 0; i < out_buf->bytesused; i++) {
			sum = sum * 31 + ((uint8_t *)out_buf->buffer)[i];
		}
	}

	return sum;
}

/*
 * Every advertised input format carries the same picture, so the encoded streams are the
 * same. This catches swapped luma and chroma, or swapped chroma planes.
 */
ZTEST(video_encoder, test_input_formats)
{
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_INPUT};
	uint32_t reference;
	int tested = 0;

	zassert_ok(video_get_caps(enc, &caps));

	stream_stop();
	set_input_format(VIDEO_PIX_FMT_NV12);
	encoder_before(NULL);
	reference = stream_checksum();

	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		uint32_t pixfmt = caps.format_caps[i].pixelformat;

		if (pixfmt != VIDEO_PIX_FMT_YUV420 && pixfmt != VIDEO_PIX_FMT_YUYV &&
		    pixfmt != VIDEO_PIX_FMT_UYVY) {
			continue;
		}

		stream_stop();
		set_input_format(pixfmt);
		encoder_before(NULL);
		zassert_equal(stream_checksum(), reference, "%s stream differs from NV12",
			      VIDEO_FOURCC_TO_STR(pixfmt));
		TC_PRINT("%s: same stream as NV12\n", VIDEO_FOURCC_TO_STR(pixfmt));
		tested++;
	}

	stream_stop();
	set_input_format(VIDEO_PIX_FMT_NV12);

	if (tested == 0) {
		ztest_test_skip();
	}
}

ZTEST_SUITE(video_encoder, NULL, encoder_setup, encoder_before, encoder_after, NULL);

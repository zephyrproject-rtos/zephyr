/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define ALL_BLEND_MODES (DISPLAY_BLEND_NONE | DISPLAY_BLEND_COVERAGE | DISPLAY_BLEND_PREMULTIPLIED)
/* A controller fetches a plane by DMA; 64 bytes satisfies every one in the tree */
#define PLANE_BUF_ALIGN 64

static const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static struct display_capabilities caps;

/* The first of the formats this test can fill, among those a plane scans out */
static enum display_pixel_format pick_format(uint32_t supported)
{
	static const enum display_pixel_format preferred[] = {
		PIXEL_FORMAT_ARGB_8888,
		PIXEL_FORMAT_XRGB_8888,
		PIXEL_FORMAT_RGB_565,
		PIXEL_FORMAT_RGB_888,
	};

	for (size_t i = 0; i < ARRAY_SIZE(preferred); i++) {
		if ((supported & preferred[i]) != 0U) {
			return preferred[i];
		}
	}

	return 0;
}

static uint32_t frame_bytes(enum display_pixel_format fmt, uint16_t w, uint16_t h)
{
	return (uint32_t)w * h * DISPLAY_BITS_PER_PIXEL(fmt) / BITS_PER_BYTE;
}

/* Write one color into every pixel; the value is 0xAARRGGBB */
static void fill(void *buf, enum display_pixel_format fmt, uint16_t w, uint16_t h, uint32_t argb)
{
	uint32_t n = (uint32_t)w * h;

	switch (fmt) {
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_XRGB_8888:
		for (uint32_t i = 0; i < n; i++) {
			((uint32_t *)buf)[i] = argb;
		}
		break;
	case PIXEL_FORMAT_RGB_565: {
		uint16_t px = (uint16_t)(((argb >> 8) & 0xF800U) | ((argb >> 5) & 0x07E0U) |
					 ((argb >> 3) & 0x001FU));

		for (uint32_t i = 0; i < n; i++) {
			((uint16_t *)buf)[i] = px;
		}
		break;
	}
	case PIXEL_FORMAT_RGB_888:
		for (uint32_t i = 0; i < n; i++) {
			((uint8_t *)buf)[3 * i] = (uint8_t)(argb >> 16);
			((uint8_t *)buf)[3 * i + 1] = (uint8_t)(argb >> 8);
			((uint8_t *)buf)[3 * i + 2] = (uint8_t)argb;
		}
		break;
	default:
		break;
	}
}

static void *setup(void)
{
	zassert_true(device_is_ready(dev), "display not ready");
	display_get_capabilities(dev, &caps);

	return NULL;
}

/* Skip the test when the display has no planes, or fewer than needed */
static void need_planes(uint8_t count)
{
	if (caps.num_planes < count) {
		ztest_test_skip();
	}
}

ZTEST(display_planes, test_no_planes)
{
	struct display_plane_capabilities pcaps;
	struct display_plane_config cfg = {0};

	if (caps.num_planes != 0U) {
		ztest_test_skip();
	}

	zassert_equal(display_get_plane_capabilities(dev, 0, &pcaps), -ENOSYS);
	zassert_equal(display_set_plane(dev, 0, &cfg), -ENOSYS);
	zassert_equal(display_commit(dev), -ENOSYS);
}

ZTEST(display_planes, test_plane_capabilities)
{
	struct display_plane_capabilities pcaps;

	need_planes(1);

	for (uint8_t i = 0; i < caps.num_planes; i++) {
		zassert_ok(display_get_plane_capabilities(dev, i, &pcaps), "plane %u", i);
		zassert_not_equal(pcaps.supported_pixel_formats, 0U, "plane %u formats", i);
		zassert_true(pcaps.max_width > 0U && pcaps.max_height > 0U, "plane %u size", i);
		zassert_true(pcaps.width_align > 0U, "plane %u align", i);
		zassert_equal(pcaps.supported_blend_modes & ~ALL_BLEND_MODES, 0U,
			      "plane %u unknown blend mode", i);
	}

	zassert_equal(display_get_plane_capabilities(dev, caps.num_planes, &pcaps), -EINVAL);
	zassert_equal(display_get_plane_capabilities(dev, 0, NULL), -EINVAL);
}

ZTEST(display_planes, test_set_plane_rejects)
{
	struct display_plane_capabilities pcaps;
	static uint32_t pixels[8 * 8] __aligned(PLANE_BUF_ALIGN);
	struct display_plane_config cfg;
	enum display_pixel_format fmt;
	uint32_t bad_fmt = 0;

	need_planes(1);
	zassert_ok(display_get_plane_capabilities(dev, 0, &pcaps));
	fmt = pick_format(pcaps.supported_pixel_formats);
	zassert_not_equal(fmt, 0, "no format this test can fill");

	cfg = (struct display_plane_config){
		.buf = pixels,
		.desc = {.buf_size = sizeof(pixels), .width = 8, .height = 8, .pitch = 8},
		.pixel_format = fmt,
		.alpha = 255,
		.blend_mode = DISPLAY_BLEND_NONE,
	};

	zassert_equal(display_set_plane(dev, caps.num_planes, &cfg), -EINVAL, "plane range");
	zassert_equal(display_set_plane(dev, 0, NULL), -EINVAL, "NULL config");

	for (uint8_t bit = 0; bit < 32; bit++) {
		if ((pcaps.supported_pixel_formats & BIT(bit)) == 0U) {
			bad_fmt = BIT(bit);
			break;
		}
	}
	if (bad_fmt != 0U) {
		cfg.pixel_format = bad_fmt;
		zassert_equal(display_set_plane(dev, 0, &cfg), -ENOTSUP, "unsupported format");
		cfg.pixel_format = fmt;
	}

	cfg.desc.width = 0;
	zassert_equal(display_set_plane(dev, 0, &cfg), -EINVAL, "zero width");
	cfg.desc.width = 8;

	cfg.desc.pitch = 4;
	zassert_equal(display_set_plane(dev, 0, &cfg), -EINVAL, "pitch below width");
	cfg.desc.pitch = 8;

	cfg.x = pcaps.max_width - 4;
	zassert_equal(display_set_plane(dev, 0, &cfg), -EINVAL, "off the right edge");
	cfg.x = 0;

	cfg.y = pcaps.max_height - 4;
	zassert_equal(display_set_plane(dev, 0, &cfg), -EINVAL, "off the bottom edge");
	cfg.y = 0;

	cfg.desc.buf_size = frame_bytes(fmt, 8, 8) - 1U;
	zassert_equal(display_set_plane(dev, 0, &cfg), -EINVAL, "buffer too small");
	cfg.desc.buf_size = sizeof(pixels);

	zassert_ok(display_set_plane(dev, 0, &cfg), "a valid configuration stages");
	cfg.buf = NULL;
	zassert_ok(display_set_plane(dev, 0, &cfg), "a disable stages");
	zassert_ok(display_commit(dev));
}

ZTEST(display_planes, test_blend_mode_rejects)
{
	struct display_plane_capabilities pcaps;
	static uint32_t pixels[8 * 8] __aligned(PLANE_BUF_ALIGN);
	struct display_plane_config cfg;
	uint32_t bad_mode = 0;

	need_planes(2);
	zassert_ok(display_get_plane_capabilities(dev, 1, &pcaps));

	cfg = (struct display_plane_config){
		.buf = pixels,
		.desc = {.buf_size = sizeof(pixels), .width = 8, .height = 8, .pitch = 8},
		.pixel_format = pick_format(pcaps.supported_pixel_formats),
		.alpha = 255,
		.blend_mode = BIT(3),
	};
	zassert_equal(display_set_plane(dev, 1, &cfg), -ENOTSUP, "unknown blend mode");

	for (uint8_t bit = 0; bit < 3; bit++) {
		if ((pcaps.supported_blend_modes & BIT(bit)) == 0U &&
		    BIT(bit) != DISPLAY_BLEND_NONE) {
			bad_mode = BIT(bit);
			break;
		}
	}
	if (bad_mode != 0U) {
		cfg.blend_mode = bad_mode;
		zassert_equal(display_set_plane(dev, 1, &cfg), -ENOTSUP, "unsupported blend mode");
	}

	cfg.blend_mode = DISPLAY_BLEND_NONE;
	zassert_ok(display_set_plane(dev, 1, &cfg), "opaque is always accepted");
	cfg.buf = NULL;
	zassert_ok(display_set_plane(dev, 1, &cfg));
	zassert_ok(display_commit(dev));
}

ZTEST(display_planes, test_commit_nothing_staged)
{
	need_planes(1);
	zassert_ok(display_commit(dev));
	zassert_ok(display_commit(dev));
}

struct commit_worker {
	struct k_thread thread;
	uint8_t plane;
	int ret;
};

static K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, 2, 1024);
static uint32_t worker_pixels[2][16 * 16] __aligned(PLANE_BUF_ALIGN);

static void commit_worker_fn(void *p1, void *p2, void *p3)
{
	struct commit_worker *w = p1;
	struct display_plane_capabilities pcaps;
	struct display_plane_config cfg;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	w->ret = display_get_plane_capabilities(dev, w->plane, &pcaps);
	if (w->ret != 0) {
		return;
	}

	cfg = (struct display_plane_config){
		.buf = worker_pixels[w->plane & 1U],
		.desc = {.buf_size = sizeof(worker_pixels[0]),
			 .width = 16,
			 .height = 16,
			 .pitch = 16},
		.pixel_format = pick_format(pcaps.supported_pixel_formats),
		.x = (uint16_t)(16U * w->plane),
		.alpha = 255,
		.blend_mode = DISPLAY_BLEND_NONE,
	};

	for (int i = 0; i < 10; i++) {
		w->ret = display_set_plane(dev, w->plane, &cfg);
		if (w->ret != 0) {
			return;
		}
		w->ret = display_commit(dev);
		if (w->ret != 0) {
			return;
		}
		k_yield();
	}

	cfg.buf = NULL;
	(void)display_set_plane(dev, w->plane, &cfg);
	(void)display_commit(dev);
}

/* One producer per plane, each committing on its own, as two pipelines would */
ZTEST(display_planes, test_concurrent_commits)
{
	static struct commit_worker workers[2];

	need_planes(2);

	for (uint8_t i = 0; i < 2; i++) {
		workers[i].plane = i;
		workers[i].ret = -1;
		k_thread_create(&workers[i].thread, worker_stacks[i],
				K_THREAD_STACK_SIZEOF(worker_stacks[i]), commit_worker_fn,
				&workers[i], NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	for (uint8_t i = 0; i < 2; i++) {
		zassert_ok(k_thread_join(&workers[i].thread, K_SECONDS(10)));
		zassert_ok(workers[i].ret, "worker %u", i);
	}
}

/*
 * Plane 0 filled with blue, plane 1 a half-alpha red rectangle in the middle:
 * a viewer sees purple over blue on a display that blends, and the display
 * keeps it for two seconds.
 */
ZTEST(display_planes, test_two_planes_visible)
{
	struct display_plane_capabilities pcaps0, pcaps1;
	struct display_plane_config cfg0, cfg1;
	enum display_pixel_format fmt0, fmt1;
	uint16_t w1, h1;
	void *buf0, *buf1;

	need_planes(2);
	zassert_ok(display_get_plane_capabilities(dev, 0, &pcaps0));
	zassert_ok(display_get_plane_capabilities(dev, 1, &pcaps1));
	fmt0 = pick_format(pcaps0.supported_pixel_formats);
	fmt1 = (pcaps1.supported_pixel_formats & PIXEL_FORMAT_ARGB_8888) != 0U
		       ? PIXEL_FORMAT_ARGB_8888
		       : pick_format(pcaps1.supported_pixel_formats);
	zassert_true(fmt0 != 0 && fmt1 != 0, "no format this test can fill");

	w1 = ROUND_DOWN(caps.x_resolution / 2, pcaps1.width_align);
	h1 = caps.y_resolution / 2;
	buf0 = k_aligned_alloc(PLANE_BUF_ALIGN,
			       frame_bytes(fmt0, caps.x_resolution, caps.y_resolution));
	buf1 = k_aligned_alloc(PLANE_BUF_ALIGN, frame_bytes(fmt1, w1, h1));
	zassert_not_null(buf0);
	zassert_not_null(buf1);
	fill(buf0, fmt0, caps.x_resolution, caps.y_resolution, 0xFF0000FFU);
	fill(buf1, fmt1, w1, h1, 0x80FF0000U);

	cfg0 = (struct display_plane_config){
		.buf = buf0,
		.desc = {.buf_size = frame_bytes(fmt0, caps.x_resolution, caps.y_resolution),
			 .width = caps.x_resolution,
			 .height = caps.y_resolution,
			 .pitch = caps.x_resolution},
		.pixel_format = fmt0,
		.alpha = 255,
		.blend_mode = DISPLAY_BLEND_NONE,
	};
	cfg1 = (struct display_plane_config){
		.buf = buf1,
		.desc = {.buf_size = frame_bytes(fmt1, w1, h1),
			 .width = w1,
			 .height = h1,
			 .pitch = w1},
		.pixel_format = fmt1,
		.x = caps.x_resolution / 4,
		.y = caps.y_resolution / 4,
		.alpha = 255,
		.blend_mode = (pcaps1.supported_blend_modes & DISPLAY_BLEND_COVERAGE) != 0U
				      ? DISPLAY_BLEND_COVERAGE
				      : DISPLAY_BLEND_NONE,
	};

	(void)display_blanking_off(dev);
	zassert_ok(display_set_plane(dev, 0, &cfg0));
	zassert_ok(display_set_plane(dev, 1, &cfg1));
	zassert_ok(display_commit(dev));
	k_sleep(K_SECONDS(2));

	cfg1.buf = NULL;
	zassert_ok(display_set_plane(dev, 1, &cfg1));
	zassert_ok(display_commit(dev));
	cfg0.buf = NULL;
	zassert_ok(display_set_plane(dev, 0, &cfg0));
	zassert_ok(display_commit(dev));

	k_free(buf1);
	k_free(buf0);
}

ZTEST_SUITE(display_planes, NULL, setup, NULL, NULL, NULL);

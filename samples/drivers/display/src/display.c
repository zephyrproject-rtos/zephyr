/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * Based on ST7789V sample:
 * Copyright (c) 2019 Marc Reilly
 *
 * Copyright (c) 2026 NXP
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

#include "display.h"

/*
 * Display render context
 */
struct render_ctx {
	const struct device *display_dev;
	uint8_t *buf;
	size_t buf_size;
	uint16_t rect_w;
	uint16_t rect_h;
	uint8_t full_bpp;
	uint16_t frame_w;
	uint8_t **full_buf;
	size_t num_full_buffers;
	size_t full_frame_size;
	struct display_buffer_descriptor *buf_desc;
	struct display_buffer_descriptor *full_desc;
};

#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
static void blit_rect(uint8_t *frame, uint16_t frame_w,
		      uint16_t x, uint16_t y, uint16_t w, uint16_t h,
		      const uint8_t *src, uint8_t bpp_bytes)
{
	size_t row_bytes = (size_t)w * bpp_bytes;

	for (uint16_t row = 0; row < h; row++) {
		memcpy(frame + ((size_t)(y + row) * frame_w + x) * bpp_bytes,
		       src + (size_t)row * row_bytes,
		       row_bytes);
	}
}
#endif /* CONFIG_SAMPLE_DISPLAY_FULL_FRAME */

enum corner {
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT,
	BOTTOM_LEFT
};

typedef void (*fill_buffer)(enum corner corner, uint8_t grey, uint8_t *buf,
			    size_t buf_size, enum display_pixel_format fmt);


static void fill_buffer_argb8888(enum corner corner, uint8_t grey, uint8_t *buf,
				 size_t buf_size, enum display_pixel_format fmt)
{
	uint8_t r = 0, g = 0, b = 0;
	uint8_t a = 0xFFu;
	uint32_t color;

	switch (corner) {
	case TOP_LEFT:
		r = 0xFF;
		break;
	case TOP_RIGHT:
		g = 0xFF;
		break;
	case BOTTOM_RIGHT:
		b = 0xFF;
		break;
	case BOTTOM_LEFT:
	default:
		r = grey; g = grey; b = grey;
		break;
	}

	switch (fmt) {
	case PIXEL_FORMAT_ABGR_8888:
		color = (uint32_t)a << 24 | (uint32_t)b << 16 | (uint32_t)g << 8 | r;
		break;
	case PIXEL_FORMAT_RGBA_8888:
		color = (uint32_t)r << 24 | (uint32_t)g << 16 | (uint32_t)b << 8 | a;
		break;
	case PIXEL_FORMAT_BGRA_8888:
		color = (uint32_t)b << 24 | (uint32_t)g << 16 | (uint32_t)r << 8 | a;
		break;
	default: /* PIXEL_FORMAT_ARGB_8888 / PIXEL_FORMAT_XRGB_8888 */
		color = (uint32_t)a << 24 | (uint32_t)r << 16 | (uint32_t)g << 8 | b;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 4) {
		*((uint32_t *)(buf + idx)) = sys_cpu_to_le32(color);
	}
}

static void fill_buffer_rgb888(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size, enum display_pixel_format fmt)
{
	uint8_t r = 0, g = 0, b = 0;
	uint8_t byte0, byte1, byte2;

	switch (corner) {
	case TOP_LEFT:
		r = 0xFF;
		break;
	case TOP_RIGHT:
		g = 0xFF;
		break;
	case BOTTOM_RIGHT:
		b = 0xFF;
		break;
	case BOTTOM_LEFT:
	default:
		r = grey; g = grey; b = grey;
		break;
	}

	if (fmt == PIXEL_FORMAT_BGR_888) {
		byte0 = r; byte1 = g; byte2 = b;
	} else {
		byte0 = b; byte1 = g; byte2 = r;
	}

	for (size_t idx = 0; idx < buf_size; idx += 3) {
		*(buf + idx + 0) = byte0;
		*(buf + idx + 1) = byte1;
		*(buf + idx + 2) = byte2;
	}
}

static uint16_t get_rgb565_color(enum corner corner, uint8_t grey)
{
	uint16_t color = 0;
	uint16_t grey_5bit;

	switch (corner) {
	case TOP_LEFT:
		color = 0xF800u;
		break;
	case TOP_RIGHT:
		color = 0x07E0u;
		break;
	case BOTTOM_RIGHT:
		color = 0x001Fu;
		break;
	case BOTTOM_LEFT:
		grey_5bit = grey & 0x1Fu;
		/* shift the green an extra bit, it has 6 bits */
		color = grey_5bit << 11 | grey_5bit << (5 + 1) | grey_5bit;
		break;
	}
	return color;
}

static void fill_buffer_rgb565x(enum corner corner, uint8_t grey, uint8_t *buf,
				size_t buf_size, enum display_pixel_format fmt)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(buf + idx + 0) = (color >> 8) & 0xFFu;
		*(buf + idx + 1) = (color >> 0) & 0xFFu;
	}
}

static void fill_buffer_rgb565(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size, enum display_pixel_format fmt)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(uint16_t *)(buf + idx) = color;
	}
}

static void fill_buffer_mono(enum corner corner, uint8_t grey,
			     uint8_t black, uint8_t white,
			     uint8_t *buf, size_t buf_size)
{
	uint16_t color;

	switch (corner) {
	case BOTTOM_LEFT:
		color = (grey & 0x01u) ? white : black;
		break;
	default:
		color = black;
		break;
	}

	memset(buf, color, buf_size);
}

static inline void fill_buffer_l_8(enum corner corner, uint8_t grey, uint8_t *buf,
				    size_t buf_size, enum display_pixel_format fmt)
{
	uint8_t color;

	switch (corner) {
	case TOP_LEFT:
		color = 0x00u;
		break;
	case TOP_RIGHT:
		/* Use 0xE0 since 0xFF leads to drawing white corner on white back-ground */
		color = 0xE0u;
		break;
	case BOTTOM_RIGHT:
		color = 0x88u;
		break;
	case BOTTOM_LEFT:
		color = 0x00u | grey;
		break;
	default:
		color = 0;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 1) {
		*(uint8_t *)(buf + idx) = color;
	}
}

static inline void fill_buffer_l_4(enum corner corner, uint8_t grey, uint8_t *buf,
				   size_t buf_size, enum display_pixel_format fmt)
{
	uint8_t nibble;

	switch (corner) {
	case TOP_LEFT:
		nibble = 0x0u;
		break;
	case TOP_RIGHT:
		nibble = 0xEu;
		break;
	case BOTTOM_RIGHT:
		nibble = 0x8u;
		break;
	case BOTTOM_LEFT:
		nibble = grey & 0x0Fu;
		break;
	default:
		nibble = 0;
		break;
	}

	uint8_t byte_val = (nibble << 4) | nibble;

	for (size_t idx = 0; idx < buf_size; idx += 1) {
		buf[idx] = byte_val;
	}
}

static void fill_buffer_al_88(enum corner corner, uint8_t grey, uint8_t *buf,
			      size_t buf_size, enum display_pixel_format fmt)
{
	uint16_t color;

	switch (corner) {
	case TOP_LEFT:
		color = 0xFF00u;
		break;
	case TOP_RIGHT:
		color = 0xFFFFu;
		break;
	case BOTTOM_RIGHT:
		color = 0xFF88u;
		break;
	case BOTTOM_LEFT:
		color = 0xFF00u | grey;
		break;
	default:
		color = 0;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*((uint16_t *)(buf + idx)) = sys_cpu_to_le16(color);
	}
}

static inline void fill_buffer_mono01(enum corner corner, uint8_t grey,
				      uint8_t *buf, size_t buf_size,
				      enum display_pixel_format fmt)
{
	fill_buffer_mono(corner, grey, 0x00u, 0xFFu, buf, buf_size);
}

static inline void fill_buffer_mono10(enum corner corner, uint8_t grey,
				      uint8_t *buf, size_t buf_size,
				      enum display_pixel_format fmt)
{
	fill_buffer_mono(corner, grey, 0xFFu, 0x00u, buf, buf_size);
}

#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
static int alloc_full_framebuffers(uint8_t *full_buf[], size_t count,
				    size_t frame_size, uint8_t bg_color)
{
	for (size_t i = 0; i < count; i++) {
		full_buf[i] = k_aligned_alloc(CONFIG_SAMPLE_BUFFER_ADDR_ALIGN, frame_size);
		if (full_buf[i] == NULL) {
			LOG_ERR("Could not allocate frame buffer %zu (%zu B). "
				"Increase CONFIG_HEAP_MEM_POOL_ADD_SIZE_SAMPLE.",
				i, frame_size);
			return -ENOMEM;
		}
		(void)memset(full_buf[i], bg_color, frame_size);
	}
	return 0;
}
#endif /* CONFIG_SAMPLE_DISPLAY_FULL_FRAME */

/*
 * Renders one static corner rectangle, either into every full-frame buffer
 * (so all buffers carry the same background before the animation starts) or
 * directly to the display when full-frame mode is off.
 */
static int render_static_corner(const struct render_ctx *ctx, fill_buffer fnc,
				 enum corner corner, enum display_pixel_format fmt,
				 uint16_t x, uint16_t y)
{
	fnc(corner, 0, ctx->buf, ctx->buf_size, fmt);

	if (IS_ENABLED(CONFIG_SAMPLE_DISPLAY_FULL_FRAME)) {
		for (size_t i = 0; i < ctx->num_full_buffers; i++) {
			blit_rect(ctx->full_buf[i], ctx->frame_w, x, y,
				  ctx->rect_w, ctx->rect_h, ctx->buf, ctx->full_bpp);
		}
		return 0;
	}

	return display_write(ctx->display_dev, x, y, ctx->buf_desc, ctx->buf);
}

/*
 * Renders the animated bottom-left rectangle for one frame: blits into the
 * buffer selected by render_idx and submits it, or writes the small buffer
 * directly when full-frame mode is off.
 */
static int render_frame(const struct render_ctx *ctx, uint16_t x, uint16_t y, size_t render_idx)
{
	if (IS_ENABLED(CONFIG_SAMPLE_DISPLAY_FULL_FRAME)) {
		blit_rect(ctx->full_buf[render_idx], ctx->frame_w, x, y,
			  ctx->rect_w, ctx->rect_h, ctx->buf, ctx->full_bpp);
		return display_write(ctx->display_dev, 0, 0, ctx->full_desc,
				      ctx->full_buf[render_idx]);
	}

	return display_write(ctx->display_dev, x, y, ctx->buf_desc, ctx->buf);
}

int sample_display_draw(void)
{
	uint16_t x;
	uint16_t y;
	uint16_t rect_w;
	uint16_t rect_h;
	uint16_t h_step;
	size_t scale;
	size_t grey_count;
	uint8_t bg_color;
	uint8_t *buf;
	int32_t grey_scale_sleep;
	const struct device *display_dev;
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size = 0;
	fill_buffer fill_buffer_fnc = NULL;
	int ret = 0;
	uint8_t full_bpp = 0;
	size_t full_frame_size = 0;
	size_t render_idx = 0;
	struct display_buffer_descriptor full_desc = {0};
	uint8_t *full_buf[SAMPLE_DISPLAY_NUM_FULL_FRAMEBUFFERS] = {NULL};

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.",
			display_dev->name);
		ret = -ENODEV;
		goto end;
	}

	/* Hold a runtime PM reference so the display stays active for the
	 * duration of the sample. No-op when runtime PM is not enabled on
	 * the device.
	 */
	(void)pm_device_runtime_get(display_dev);

	LOG_INF("Display sample for %s", display_dev->name);
	display_get_capabilities(display_dev, &capabilities);

	if (capabilities.screen_info & SCREEN_INFO_MONO_VTILED) {
		rect_w = 16;
		rect_h = 8;
	} else {
		rect_w = 2;
		rect_h = 1;
	}

	if ((capabilities.x_resolution < 3 * rect_w) ||
	    (capabilities.y_resolution < 3 * rect_h) ||
	    (capabilities.x_resolution < 8 * rect_h)) {
		rect_w = capabilities.x_resolution * 40 / 100;
		rect_h = capabilities.y_resolution * 40 / 100;
		h_step = capabilities.y_resolution * 20 / 100;
		scale = 1;
	} else {
		h_step = rect_h;
		scale = (capabilities.x_resolution / 8) / rect_h;
	}

	rect_w *= scale;
	rect_h *= scale;

	if (capabilities.screen_info & SCREEN_INFO_EPD) {
		grey_scale_sleep = 10000;
	} else {
		grey_scale_sleep = 100;
	}

	if (capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
		rect_w = capabilities.x_resolution;
	}

	rect_w = ROUND_UP(rect_w, CONFIG_SAMPLE_PITCH_ALIGN);

	buf_size = rect_w * rect_h;

	if (buf_size < (capabilities.x_resolution * h_step)) {
		buf_size = capabilities.x_resolution * h_step;
	}

	switch (capabilities.current_pixel_format) {
	case PIXEL_FORMAT_XRGB_8888:
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_ABGR_8888:
	case PIXEL_FORMAT_RGBA_8888:
	case PIXEL_FORMAT_BGRA_8888:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_argb8888;
		break;
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_BGR_888:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb888;
		break;
	case PIXEL_FORMAT_RGB_565:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb565;
		break;
	case PIXEL_FORMAT_RGB_565X:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb565x;
		break;
	case PIXEL_FORMAT_L_8:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_l_8;
		break;
	case PIXEL_FORMAT_L_4:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_l_4;
		break;
	case PIXEL_FORMAT_AL_88:
		bg_color = 0x00u;
		fill_buffer_fnc = fill_buffer_al_88;
		break;
	case PIXEL_FORMAT_MONO01:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_mono01;
		break;
	case PIXEL_FORMAT_MONO10:
		bg_color = 0x00u;
		fill_buffer_fnc = fill_buffer_mono10;
		break;
	default:
		LOG_ERR("Unsupported pixel format. Aborting sample.");
		ret = -ENOTSUP;
		goto end;
	}

	/* Amount of bytes necessary depends on format - ensure to round up, necessary for
	 * MONO formats
	 */
	buf_size *= DISPLAY_BITS_PER_PIXEL(capabilities.current_pixel_format);
	buf_size = DIV_ROUND_UP(DIV_ROUND_UP(buf_size, NUM_BITS(uint8_t)), sizeof(uint8_t));

	buf = k_aligned_alloc(CONFIG_SAMPLE_BUFFER_ADDR_ALIGN, buf_size);

	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting sample.");
		ret = -ENOMEM;
		goto end;
	}

	(void)memset(buf, bg_color, buf_size);

	if (IS_ENABLED(CONFIG_SAMPLE_DISPLAY_FULL_FRAME)) {
		full_bpp = DIV_ROUND_UP(
			DISPLAY_BITS_PER_PIXEL(capabilities.current_pixel_format),
			NUM_BITS(uint8_t));
		full_frame_size = (size_t)capabilities.x_resolution *
				  capabilities.y_resolution * full_bpp;

		ret = alloc_full_framebuffers(full_buf, SAMPLE_DISPLAY_NUM_FULL_FRAMEBUFFERS,
					       full_frame_size, bg_color);
		if (ret < 0) {
			goto end;
		}

		full_desc.buf_size        = full_frame_size;
		full_desc.pitch           = ROUND_UP(capabilities.x_resolution,
						     CONFIG_SAMPLE_PITCH_ALIGN);
		full_desc.width           = capabilities.x_resolution;
		full_desc.height          = capabilities.y_resolution;
		full_desc.frame_incomplete = false;
	}

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = ROUND_UP(capabilities.x_resolution, CONFIG_SAMPLE_PITCH_ALIGN);
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;

	/*
	 * The following writes will only render parts of the image,
	 * so turn this option on.
	 * This allows double-buffered displays to hold the pixels
	 * back until the image is complete.
	 */
	buf_desc.frame_incomplete = true;

	if (!IS_ENABLED(CONFIG_SAMPLE_DISPLAY_FULL_FRAME)) {
		for (uint16_t idx = 0; idx < capabilities.y_resolution; idx += h_step) {
			/*
			 * Tweaking the height value not to draw outside of the display.
			 * It is required when using a monochrome display whose vertical
			 * resolution can not be divided by 8.
			 */
			if ((capabilities.y_resolution - idx) < h_step) {
				buf_desc.height = (capabilities.y_resolution - idx);
			}
			ret = display_write(display_dev, 0, idx, &buf_desc, buf);
			if (ret < 0) {
				LOG_ERR("Failed to write to display (error %d)", ret);
				goto end;
			}
		}
	}

	buf_desc.pitch = ROUND_UP(rect_w, CONFIG_SAMPLE_PITCH_ALIGN);
	buf_desc.width = rect_w;
	buf_desc.height = rect_h;

	struct render_ctx ctx = {
		.display_dev = display_dev,
		.buf = buf,
		.buf_size = buf_size,
		.rect_w = rect_w,
		.rect_h = rect_h,
		.full_bpp = full_bpp,
		.frame_w = capabilities.x_resolution,
		.full_buf = full_buf,
		.num_full_buffers = SAMPLE_DISPLAY_NUM_FULL_FRAMEBUFFERS,
		.full_frame_size = full_frame_size,
		.buf_desc = &buf_desc,
		.full_desc = &full_desc,
	};

	x = 0;
	y = 0;
	ret = render_static_corner(&ctx, fill_buffer_fnc, TOP_LEFT,
				    capabilities.current_pixel_format, x, y);
	if (ret < 0) {
		LOG_ERR("Failed to write to display (error %d)", ret);
		goto end;
	}

	x = capabilities.x_resolution - rect_w;
	y = 0;
	ret = render_static_corner(&ctx, fill_buffer_fnc, TOP_RIGHT,
				    capabilities.current_pixel_format, x, y);
	if (ret < 0) {
		LOG_ERR("Failed to write to display (error %d)", ret);
		goto end;
	}

	/*
	 * This is the last write of the frame, so turn this off.
	 * Double-buffered displays will now present the new image
	 * to the user.
	 */
	buf_desc.frame_incomplete = false;

	x = capabilities.x_resolution - rect_w;
	y = capabilities.y_resolution - rect_h;
	ret = render_static_corner(&ctx, fill_buffer_fnc, BOTTOM_RIGHT,
				    capabilities.current_pixel_format, x, y);
	if (ret < 0) {
		LOG_ERR("Failed to write to display (error %d)", ret);
		goto end;
	}

	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		goto end;
	}

	grey_count = 0;
	x = 0;
	y = capabilities.y_resolution - rect_h;

	LOG_INF("Display starts");
	while (1) {
		fill_buffer_fnc(BOTTOM_LEFT, grey_count, buf, buf_size,
			capabilities.current_pixel_format);

		ret = render_frame(&ctx, x, y, render_idx);
		if (ret < 0) {
			LOG_ERR("Failed to write to display (error %d)", ret);
			goto end;
		}

		++grey_count;
		k_msleep(grey_scale_sleep);
		render_idx = (render_idx + 1) % SAMPLE_DISPLAY_NUM_FULL_FRAMEBUFFERS;

#if CONFIG_TEST
		if (grey_count >= 30) {
			LOG_INF("Display sample test mode done %s", display_dev->name);
			break;
		}
#endif
	}

end:
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	for (size_t i = 0; i < SAMPLE_DISPLAY_NUM_FULL_FRAMEBUFFERS; i++) {
		k_free(full_buf[i]);
	}
#endif /* CONFIG_SAMPLE_DISPLAY_FULL_FRAME */

#if CONFIG_TEST
	if (ret == 0) {
		LOG_INF("PROJECT EXECUTION SUCCESSFUL");
	} else {
		LOG_INF("PROJECT EXECUTION FAILED");
	}
#endif
#ifdef CONFIG_ARCH_POSIX
	posix_exit(ret == 0 ? 0 : 1);
#endif
	return 0;
}

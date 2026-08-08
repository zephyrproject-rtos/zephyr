/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * Based on ST7789V sample:
 * Copyright (c) 2019 Marc Reilly
 *
 * Copyright (c) 2026 NXP
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
#include <zephyr/cache.h>

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

#include "display.h"

static inline void flush_buf(void *addr, size_t size)
{
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	sys_cache_data_flush_range(addr, size);
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
#endif
}

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

#ifdef CONFIG_SAMPLE_DISPLAY_VSYNC_CALLBACK
static K_SEM_DEFINE(vsync_sem, 0, 1);

static enum display_event_result on_vsync(const struct device *dev,
					  uint32_t evt,
					  const struct display_event_data *data,
					  void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(evt);
	ARG_UNUSED(data);
	ARG_UNUSED(user_data);

	k_sem_give(&vsync_sem);
	return DISPLAY_EVENT_RESULT_CONTINUE;
}
#endif /* CONFIG_SAMPLE_DISPLAY_VSYNC_CALLBACK */

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
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	uint8_t full_bpp;
	size_t full_frame_size;
	struct display_buffer_descriptor full_desc;
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
	uint8_t *full_buf[2] = {NULL, NULL};
	int render_idx = 0;
#else
	uint8_t *full_buf = NULL;
#endif
#endif
#ifdef CONFIG_SAMPLE_DISPLAY_VSYNC_CALLBACK
	uint32_t vsync_handle = 0;
#endif

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

#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	full_bpp = DIV_ROUND_UP(
		DISPLAY_BITS_PER_PIXEL(capabilities.current_pixel_format),
		NUM_BITS(uint8_t));
	full_frame_size = (size_t)capabilities.x_resolution *
			  capabilities.y_resolution * full_bpp;
	full_buf[0] = k_aligned_alloc(64, full_frame_size);
	if (full_buf[0] == NULL) {
		LOG_ERR("Could not allocate full frame buffer (%zu B). "
			"Increase CONFIG_HEAP_MEM_POOL_ADD_SIZE_SAMPLE.",
			full_frame_size);
		ret = -ENOMEM;
		goto end;
	}
	(void)memset(full_buf[0], bg_color, full_frame_size);

#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
	full_buf[1] = k_aligned_alloc(64, full_frame_size);
	if (full_buf[1] == NULL) {
		LOG_ERR("Could not allocate second frame buffer (%zu B). "
			"Increase CONFIG_HEAP_MEM_POOL_ADD_SIZE_SAMPLE.",
			full_frame_size);
		ret = -ENOMEM;
		goto end;
	}
	(void)memset(full_buf[1], bg_color, full_frame_size);
#endif /* CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER */

	full_desc.buf_size        = full_frame_size;
	full_desc.pitch           = ROUND_UP(capabilities.x_resolution,
					     CONFIG_SAMPLE_PITCH_ALIGN);
	full_desc.width           = capabilities.x_resolution;
	full_desc.height          = capabilities.y_resolution;
	full_desc.frame_incomplete = false;
#endif /* CONFIG_SAMPLE_DISPLAY_FULL_FRAME */

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

#ifndef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	flush_buf(buf, buf_size);
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
#endif /* !CONFIG_SAMPLE_DISPLAY_FULL_FRAME */

	buf_desc.pitch = ROUND_UP(rect_w, CONFIG_SAMPLE_PITCH_ALIGN);
	buf_desc.width = rect_w;
	buf_desc.height = rect_h;

	fill_buffer_fnc(TOP_LEFT, 0, buf, buf_size, capabilities.current_pixel_format);
	x = 0;
	y = 0;
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	blit_rect(full_buf[0], capabilities.x_resolution, x, y,
		  rect_w, rect_h, buf, full_bpp);
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
	blit_rect(full_buf[1], capabilities.x_resolution, x, y,
		  rect_w, rect_h, buf, full_bpp);
#endif
#else
	flush_buf(buf, buf_size);
	ret = display_write(display_dev, x, y, &buf_desc, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write to display (error %d)", ret);
		goto end;
	}
#endif

	fill_buffer_fnc(TOP_RIGHT, 0, buf, buf_size, capabilities.current_pixel_format);
	x = capabilities.x_resolution - rect_w;
	y = 0;
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	blit_rect(full_buf[0], capabilities.x_resolution, x, y,
		  rect_w, rect_h, buf, full_bpp);
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
	blit_rect(full_buf[1], capabilities.x_resolution, x, y,
		  rect_w, rect_h, buf, full_bpp);
#endif
#else
	flush_buf(buf, buf_size);
	ret = display_write(display_dev, x, y, &buf_desc, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write to display (error %d)", ret);
		goto end;
	}
#endif

	/*
	 * This is the last write of the frame, so turn this off.
	 * Double-buffered displays will now present the new image
	 * to the user.
	 */
	buf_desc.frame_incomplete = false;

	fill_buffer_fnc(BOTTOM_RIGHT, 0, buf, buf_size, capabilities.current_pixel_format);
	x = capabilities.x_resolution - rect_w;
	y = capabilities.y_resolution - rect_h;
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	blit_rect(full_buf[0], capabilities.x_resolution, x, y,
		  rect_w, rect_h, buf, full_bpp);
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
	blit_rect(full_buf[1], capabilities.x_resolution, x, y,
		  rect_w, rect_h, buf, full_bpp);
#endif
#else
	flush_buf(buf, buf_size);
	ret = display_write(display_dev, x, y, &buf_desc, buf);
	if (ret < 0) {
		LOG_ERR("Failed to write to display (error %d)", ret);
		goto end;
	}
#endif

	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		goto end;
	}

#ifdef CONFIG_SAMPLE_DISPLAY_VSYNC_CALLBACK
	ret = display_register_event_cb(display_dev, on_vsync, NULL,
					DISPLAY_EVENT_VSYNC, true, &vsync_handle);
	if (ret < 0) {
		LOG_WRN("VSYNC callback not supported (%d), falling back to polling", ret);
		vsync_handle = 0;
	} else {
		LOG_INF("VSYNC callback registered (handle %u)", vsync_handle);
	}
#endif

	grey_count = 0;
	x = 0;
	y = capabilities.y_resolution - rect_h;

	LOG_INF("Display starts");
	while (1) {
		fill_buffer_fnc(BOTTOM_LEFT, grey_count, buf, buf_size,
			capabilities.current_pixel_format);
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
		blit_rect(full_buf[render_idx], capabilities.x_resolution, x, y,
			  rect_w, rect_h, buf, full_bpp);
		flush_buf(full_buf[render_idx], full_frame_size);
		ret = display_write(display_dev, 0, 0, &full_desc, full_buf[render_idx]);
#else
		blit_rect(full_buf[0], capabilities.x_resolution, x, y,
			  rect_w, rect_h, buf, full_bpp);
		flush_buf(full_buf[0], full_frame_size);
		ret = display_write(display_dev, 0, 0, &full_desc, full_buf[0]);
#endif
#else
		flush_buf(buf, buf_size);
		ret = display_write(display_dev, x, y, &buf_desc, buf);
#endif
		if (ret < 0) {
			LOG_ERR("Failed to write to display (error %d)", ret);
			goto end;
		}

		++grey_count;
#ifdef CONFIG_SAMPLE_DISPLAY_VSYNC_CALLBACK
		if (vsync_handle != 0) {
			k_sem_take(&vsync_sem, K_FOREVER);
		} else {
			k_msleep(grey_scale_sleep);
		}
#else
		k_msleep(grey_scale_sleep);
#endif
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
			render_idx ^= 1;
#endif
#if CONFIG_TEST
		if (grey_count >= 30) {
			LOG_INF("Display sample test mode done %s", display_dev->name);
			break;
		}
#endif
	}

end:
#ifdef CONFIG_SAMPLE_DISPLAY_VSYNC_CALLBACK
	if (vsync_handle != 0) {
		(void)display_unregister_event_cb(display_dev, vsync_handle);
	}
#endif
#ifdef CONFIG_SAMPLE_DISPLAY_FULL_FRAME
	k_free(full_buf[0]);
#ifdef CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER
	k_free(full_buf[1]);
#endif
#endif
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

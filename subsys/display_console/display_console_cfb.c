/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Renderer drawing the text grid with a character framebuffer font, in the
 * pixel format the display is configured for.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/display/display_console.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include "display_console_renderer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(display_console, CONFIG_DISPLAY_CONSOLE_LOG_LEVEL);

#define DISPLAY_CONSOLE_CFB_BUF_SIZE CONFIG_DISPLAY_CONSOLE_CFB_BUFFER_SIZE

struct display_console_cfb {
	const struct device *dev;
	const struct cfb_font *font;
	uint16_t x_res;
	uint16_t y_res;
	enum display_pixel_format pixel_format;
	enum display_screen_info screen_info;
	/* Bytes per pixel, 0 for the packed monochrome formats. */
	uint8_t bpp;
	uint8_t fg_px[DISPLAY_CONSOLE_ATTR_COUNT][4];
	uint8_t bg_px[4];
};

static struct display_console_cfb display_console_cfb_inst;
static uint8_t display_console_cfb_buf[DISPLAY_CONSOLE_CFB_BUF_SIZE];

static int display_console_cfb_format_bpp(enum display_pixel_format fmt)
{
	switch (fmt) {
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		return 0;
	case PIXEL_FORMAT_L_8:
		return 1;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_RGB_565X:
	case PIXEL_FORMAT_AL_88:
		return 2;
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_BGR_888:
		return 3;
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_XRGB_8888:
	case PIXEL_FORMAT_ABGR_8888:
	case PIXEL_FORMAT_RGBA_8888:
	case PIXEL_FORMAT_BGRA_8888:
		return 4;
	default:
		return -ENOTSUP;
	}
}

static void display_console_cfb_pack_pixel(const struct display_console_cfb *c,
					   const struct display_console_rgb *rgb, uint8_t *out)
{
	const uint32_t r = rgb->r;
	const uint32_t g = rgb->g;
	const uint32_t b = rgb->b;
	const uint8_t luma = (uint8_t)((r * 77U + g * 151U + b * 28U) >> 8);
	const uint16_t rgb565 = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));

	switch (c->pixel_format) {
	case PIXEL_FORMAT_RGB_888:
		out[0] = b;
		out[1] = g;
		out[2] = r;
		break;
	case PIXEL_FORMAT_BGR_888:
		out[0] = r;
		out[1] = g;
		out[2] = b;
		break;
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_XRGB_8888:
		sys_put_le32(0xff000000U | (r << 16) | (g << 8) | b, out);
		break;
	case PIXEL_FORMAT_ABGR_8888:
		sys_put_le32(0xff000000U | (b << 16) | (g << 8) | r, out);
		break;
	case PIXEL_FORMAT_RGBA_8888:
		sys_put_le32((r << 24) | (g << 16) | (b << 8) | 0xffU, out);
		break;
	case PIXEL_FORMAT_BGRA_8888:
		sys_put_le32((b << 24) | (g << 16) | (r << 8) | 0xffU, out);
		break;
	case PIXEL_FORMAT_RGB_565:
		sys_put_le16(rgb565, out);
		break;
	case PIXEL_FORMAT_RGB_565X:
		sys_put_be16(rgb565, out);
		break;
	case PIXEL_FORMAT_L_8:
		out[0] = luma;
		break;
	case PIXEL_FORMAT_AL_88:
		out[0] = luma;
		out[1] = 0xff;
		break;
	default:
		break;
	}
}

static bool display_console_cfb_glyph_pixel(const struct display_console_cfb *c,
					    const uint8_t *glyph, uint8_t gx, uint8_t gy)
{
	const struct cfb_font *font = c->font;
	const uint8_t bit = ((font->caps & CFB_FONT_MSB_FIRST) != 0) ? 7U - (gy % 8U) : gy % 8U;

	if (glyph == NULL) {
		return false;
	}

	return (glyph[gx * DIV_ROUND_UP(font->height, 8U) + gy / 8U] & BIT(bit)) != 0U;
}

static const uint8_t *display_console_cfb_glyph(const struct display_console_cfb *c, char ch)
{
	const struct cfb_font *font = c->font;

	if ((uint8_t)ch < font->first_char || (uint8_t)ch > font->last_char) {
		return NULL;
	}

	return (const uint8_t *)font->data +
	       ((uint8_t)ch - font->first_char) * font->width * DIV_ROUND_UP(font->height, 8U);
}

static int display_console_cfb_blit(struct display_console_cfb *c, uint16_t x, uint16_t y,
				    uint16_t w, uint16_t h, size_t size, bool last)
{
	const struct display_buffer_descriptor desc = {
		.buf_size = size,
		.width = w,
		.height = h,
		.pitch = w,
		.frame_incomplete = !last,
	};

	return display_write(c->dev, x, y, &desc, display_console_cfb_buf);
}

/* Render one text row of a colour or greyscale display, as many cells per write as fit. */
static int display_console_cfb_row_color(struct display_console_cfb *c, uint16_t row,
					 const char *text, const uint8_t *attr, uint16_t cols,
					 bool last)
{
	const uint8_t fw = c->font->width;
	const uint8_t fh = c->font->height;
	const uint16_t per_write = MIN(DISPLAY_CONSOLE_CFB_BUF_SIZE / (fw * fh * c->bpp), cols);
	int err = 0;

	for (uint16_t c0 = 0; c0 < cols && err == 0; c0 += per_write) {
		const uint16_t n = MIN(per_write, cols - c0);
		const uint16_t w = n * fw;

		for (uint16_t i = 0; i < n; i++) {
			const uint8_t *glyph = display_console_cfb_glyph(c, text[c0 + i]);
			const uint8_t *fg = c->fg_px[attr[c0 + i]];

			for (uint8_t gy = 0; gy < fh; gy++) {
				uint8_t *dst =
					&display_console_cfb_buf[((size_t)gy * w + (size_t)i * fw) *
								 c->bpp];

				for (uint8_t gx = 0; gx < fw; gx++) {
					memcpy(dst,
					       display_console_cfb_glyph_pixel(c, glyph, gx, gy)
						       ? fg
						       : c->bg_px,
					       c->bpp);
					dst += c->bpp;
				}
			}
		}

		err = display_console_cfb_blit(c, c0 * fw, row * fh, w, fh, (size_t)w * fh * c->bpp,
					       last && (c0 + n >= cols));
	}

	return err;
}

/* Render one text row of a monochrome display as a full-width band of tiles. */
static int display_console_cfb_row_mono(struct display_console_cfb *c, uint16_t row,
					const char *text, uint16_t cols, bool last)
{
	const uint8_t fw = c->font->width;
	const uint8_t fh = c->font->height;
	const bool vtiled = (c->screen_info & SCREEN_INFO_MONO_VTILED) != 0;
	const bool msb = (c->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0;
	const bool one_is_white = c->pixel_format == PIXEL_FORMAT_MONO01;
	const size_t stride = vtiled ? c->x_res : DIV_ROUND_UP(c->x_res, 8U);
	const size_t size = vtiled ? stride * DIV_ROUND_UP(fh, 8U) : stride * fh;

	memset(display_console_cfb_buf, one_is_white ? 0x00 : 0xff, size);

	for (uint16_t i = 0; i < cols; i++) {
		const uint8_t *glyph = display_console_cfb_glyph(c, text[i]);

		for (uint8_t gy = 0; gy < fh; gy++) {
			for (uint8_t gx = 0; gx < fw; gx++) {
				const uint16_t x = i * fw + gx;
				uint8_t *byte;
				uint8_t bit;

				if (!display_console_cfb_glyph_pixel(c, glyph, gx, gy)) {
					continue;
				}

				if (vtiled) {
					byte = &display_console_cfb_buf[(gy / 8U) * stride + x];
					bit = msb ? 7U - (gy % 8U) : gy % 8U;
				} else {
					byte = &display_console_cfb_buf[gy * stride + x / 8U];
					bit = msb ? 7U - (x % 8U) : x % 8U;
				}

				if (one_is_white) {
					*byte |= BIT(bit);
				} else {
					*byte &= (uint8_t)~BIT(bit);
				}
			}
		}
	}

	return display_console_cfb_blit(c, 0, row * fh, c->x_res, fh, size, last);
}

/* Fill display rows [y, y + h) with the background colour. */
static int display_console_cfb_fill(struct display_console_cfb *c, uint16_t y, uint16_t h,
				    bool last)
{
	const uint16_t y_end = y + h;
	uint16_t w_max;
	uint16_t band;
	size_t size;
	int err = 0;

	if (c->bpp == 0U) {
		const bool vtiled = (c->screen_info & SCREEN_INFO_MONO_VTILED) != 0;
		const size_t stride = vtiled ? c->x_res : DIV_ROUND_UP(c->x_res, 8U);
		/* A vertically tiled buffer row holds eight pixel rows. */
		const uint16_t band_max = vtiled ? (DISPLAY_CONSOLE_CFB_BUF_SIZE / stride) * 8U
						 : DISPLAY_CONSOLE_CFB_BUF_SIZE / stride;

		if (band_max == 0U) {
			return -ENOSPC;
		}

		memset(display_console_cfb_buf,
		       (c->pixel_format == PIXEL_FORMAT_MONO01) ? 0x00 : 0xff,
		       DISPLAY_CONSOLE_CFB_BUF_SIZE);

		while (y < y_end && err == 0) {
			band = MIN(band_max, y_end - y);
			size = vtiled ? stride * DIV_ROUND_UP(band, 8U) : stride * band;
			err = display_console_cfb_blit(c, 0, y, c->x_res, band, size,
						       last && (y + band >= y_end));
			y += band;
		}

		return err;
	}

	/* As many whole pixel rows as fit, or part of a row when one does not. */
	w_max = MIN(c->x_res, DISPLAY_CONSOLE_CFB_BUF_SIZE / c->bpp);
	band = MIN(h, DISPLAY_CONSOLE_CFB_BUF_SIZE / ((size_t)w_max * c->bpp));

	for (size_t px = 0; px < (size_t)band * w_max; px++) {
		memcpy(&display_console_cfb_buf[px * c->bpp], c->bg_px, c->bpp);
	}

	while (y < y_end && err == 0) {
		const uint16_t bh = MIN(band, y_end - y);

		for (uint16_t x = 0; x < c->x_res && err == 0; x += w_max) {
			const uint16_t w = MIN(w_max, c->x_res - x);

			err = display_console_cfb_blit(c, x, y, w, bh, (size_t)w * bh * c->bpp,
						       last && (y + bh >= y_end) &&
							       (x + w >= c->x_res));
		}
		y += bh;
	}

	return err;
}

static int display_console_cfb_render(const struct display_console_snapshot *s)
{
	struct display_console_cfb *c = &display_console_cfb_inst;
	int16_t last_row = -1;
	int err = 0;

	for (uint16_t r = 0; r < s->rows; r++) {
		if (s->dirty[(s->top + r) % s->rows]) {
			last_row = r;
		}
	}

	for (uint16_t r = 0; r <= last_row; r++) {
		const uint16_t phys = (s->top + r) % s->rows;
		int row_err;

		if (!s->dirty[phys]) {
			continue;
		}

		if (c->bpp == 0U) {
			row_err = display_console_cfb_row_mono(c, r, s->text[phys], s->cols,
							       r == last_row);
		} else {
			row_err = display_console_cfb_row_color(c, r, s->text[phys], s->attr[phys],
								s->cols, r == last_row);
		}

		if (row_err != 0 && err == 0) {
			err = row_err;
		}
	}

	return err;
}

static int display_console_cfb_init(uint16_t *rows, uint16_t *cols)
{
	struct display_console_cfb *c = &display_console_cfb_inst;
	struct display_capabilities caps;
	size_t num_fonts;
	int err;

	c->dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(c->dev)) {
		LOG_ERR_DEVICE_NOT_READY(c->dev);
		return -ENODEV;
	}

	display_get_capabilities(c->dev, &caps);
	c->x_res = caps.x_resolution;
	c->y_res = caps.y_resolution;
	c->pixel_format = caps.current_pixel_format;
	c->screen_info = caps.screen_info;

	err = display_console_cfb_format_bpp(c->pixel_format);
	if (err < 0) {
		LOG_ERR("pixel format 0x%x unsupported", c->pixel_format);
		return err;
	}
	c->bpp = (uint8_t)err;

	STRUCT_SECTION_COUNT(cfb_font, &num_fonts);
	if (CONFIG_DISPLAY_CONSOLE_CFB_FONT_IDX >= num_fonts) {
		LOG_ERR("font %d unavailable, %zu fonts", CONFIG_DISPLAY_CONSOLE_CFB_FONT_IDX,
			num_fonts);
		return -EINVAL;
	}
	STRUCT_SECTION_GET(cfb_font, CONFIG_DISPLAY_CONSOLE_CFB_FONT_IDX, &c->font);
	if ((c->font->caps & CFB_FONT_MONO_VPACKED) == 0) {
		LOG_ERR("font %d is not column packed", CONFIG_DISPLAY_CONSOLE_CFB_FONT_IDX);
		return -EINVAL;
	}

	if (c->bpp == 0U && (c->screen_info & SCREEN_INFO_MONO_VTILED) != 0 &&
	    (c->font->height % 8U) != 0U) {
		LOG_ERR("font height %d not tile aligned", c->font->height);
		return -EINVAL;
	}

	if (c->bpp != 0U) {
		const size_t cell_size = (size_t)c->font->width * c->font->height * c->bpp;

		if (DISPLAY_CONSOLE_CFB_BUF_SIZE < cell_size) {
			LOG_ERR("buffer too small for one cell (%zu)", cell_size);
			return -ENOSPC;
		}
	} else {
		const bool vtiled = (c->screen_info & SCREEN_INFO_MONO_VTILED) != 0;
		const size_t row_size =
			vtiled ? (size_t)c->x_res * DIV_ROUND_UP(c->font->height, 8U)
			       : DIV_ROUND_UP(c->x_res, 8U) * c->font->height;

		if (DISPLAY_CONSOLE_CFB_BUF_SIZE < row_size) {
			LOG_ERR("buffer too small for one row (%zu)", row_size);
			return -ENOSPC;
		}
	}

	for (uint8_t a = 0; a < DISPLAY_CONSOLE_ATTR_COUNT; a++) {
		display_console_cfb_pack_pixel(c, &display_console_fg_rgb[a], c->fg_px[a]);
	}
	display_console_cfb_pack_pixel(c, &display_console_bg_rgb, c->bg_px);

	err = display_console_cfb_fill(c, 0, c->y_res, true);
	if (err != 0) {
		LOG_ERR("clearing the display failed (%d)", err);
		return err;
	}

	err = display_blanking_off(c->dev);
	if (err != 0 && err != -ENOSYS) {
		return err;
	}

	*rows = c->y_res / c->font->height;
	*cols = c->x_res / c->font->width;

	return 0;
}

const struct display_console_renderer_api display_console_renderer = {
	.init = display_console_cfb_init,
	.render = display_console_cfb_render,
};

#ifdef CONFIG_LOG
/*
 * Log backend for the panic hook, so fault reports are drawn. It also shows
 * the messages while no other backend is active, as before the shell starts;
 * enabling it later would drop them.
 */
static uint8_t display_console_cfb_log_buf[64];

static int display_console_cfb_log_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);

	display_console_write(data, length);

	return (int)length;
}

LOG_OUTPUT_DEFINE(display_console_cfb_log_output, display_console_cfb_log_out,
		  display_console_cfb_log_buf, sizeof(display_console_cfb_log_buf));

static bool display_console_cfb_log_other_active(const struct log_backend *const self)
{
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (backend != self && log_backend_is_active(backend)) {
			return true;
		}
	}

	return false;
}

static void display_console_cfb_log_process(const struct log_backend *const backend,
					    union log_msg_generic *msg)
{
	if (display_console_cfb_log_other_active(backend)) {
		return;
	}

	log_output_msg_process(&display_console_cfb_log_output, &msg->log,
			       LOG_OUTPUT_FLAG_COLORS | LOG_OUTPUT_FLAG_LEVEL |
				       LOG_OUTPUT_FLAG_TIMESTAMP |
				       LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP);
}

static void display_console_cfb_log_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	display_console_panic();
}

static const struct log_backend_api display_console_cfb_log_api = {
	.process = display_console_cfb_log_process,
	.panic = display_console_cfb_log_panic,
};

LOG_BACKEND_DEFINE(display_console_cfb_log, display_console_cfb_log_api, true);
#endif /* CONFIG_LOG */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/display/cfb.h>
#include <zephyr/drivers/video.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/vid/mpipe_vid_draw.h>

/* One pixel of the target format, prepared once per drawing call */
struct vid_draw_px {
	uint8_t bytes[4];
	uint8_t size;
};

static int vid_draw_pack(uint32_t pixfmt, uint32_t argb, struct vid_draw_px *px)
{
	uint8_t r = (uint8_t)(argb >> 16);
	uint8_t g = (uint8_t)(argb >> 8);
	uint8_t b = (uint8_t)argb;

	switch (pixfmt) {
	case VIDEO_PIX_FMT_RGB565: {
		uint16_t v = (uint16_t)(((r & 0xF8U) << 8) | ((g & 0xFCU) << 3) | (b >> 3));

		px->bytes[0] = (uint8_t)v;
		px->bytes[1] = (uint8_t)(v >> 8);
		px->size = 2;

		return 0;
	}
	case VIDEO_PIX_FMT_BGRX32:
		px->bytes[0] = b;
		px->bytes[1] = g;
		px->bytes[2] = r;
		px->bytes[3] = 0;
		px->size = 4;

		return 0;
	default:
		return -ENOTSUP;
	}
}

static void vid_draw_hline(uint8_t *frame, uint16_t width, uint16_t height,
			   const struct vid_draw_px *px, int32_t x0, int32_t x1, int32_t y)
{
	if (y < 0 || y >= (int32_t)height) {
		return;
	}

	x0 = CLAMP(x0, 0, (int32_t)width - 1);
	x1 = CLAMP(x1, 0, (int32_t)width - 1);

	for (int32_t x = x0; x <= x1; x++) {
		uint8_t *dst = &frame[((uint32_t)y * width + (uint32_t)x) * px->size];

		for (uint8_t i = 0; i < px->size; i++) {
			dst[i] = px->bytes[i];
		}
	}
}

static void vid_draw_vline(uint8_t *frame, uint16_t width, uint16_t height,
			   const struct vid_draw_px *px, int32_t x, int32_t y0, int32_t y1)
{
	if (x < 0 || x >= (int32_t)width) {
		return;
	}

	y0 = CLAMP(y0, 0, (int32_t)height - 1);
	y1 = CLAMP(y1, 0, (int32_t)height - 1);

	for (int32_t y = y0; y <= y1; y++) {
		uint8_t *dst = &frame[((uint32_t)y * width + (uint32_t)x) * px->size];

		for (uint8_t i = 0; i < px->size; i++) {
			dst[i] = px->bytes[i];
		}
	}
}

int mpipe_vid_draw_rect(uint8_t *frame, uint16_t width, uint16_t height, uint32_t pixfmt,
			const struct mpipe_vid_rect *rect, uint32_t argb, uint8_t thickness)
{
	struct vid_draw_px px;
	int ret;

	if (frame == NULL || rect == NULL) {
		return -EINVAL;
	}

	ret = vid_draw_pack(pixfmt, argb, &px);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t t = 0; t < thickness; t++) {
		vid_draw_hline(frame, width, height, &px, rect->x0, rect->x1, rect->y0 + t);
		vid_draw_hline(frame, width, height, &px, rect->x0, rect->x1, rect->y1 - t);
		vid_draw_vline(frame, width, height, &px, rect->x0 + t, rect->y0, rect->y1);
		vid_draw_vline(frame, width, height, &px, rect->x1 - t, rect->y0, rect->y1);
	}

	return 0;
}

int mpipe_vid_draw_fill(uint8_t *frame, uint16_t width, uint16_t height, uint32_t pixfmt,
			const struct mpipe_vid_rect *rect, uint32_t argb)
{
	struct vid_draw_px px;
	int ret;

	if (frame == NULL || rect == NULL) {
		return -EINVAL;
	}

	ret = vid_draw_pack(pixfmt, argb, &px);
	if (ret != 0) {
		return ret;
	}

	for (int32_t y = MAX(rect->y0, 0); y <= MIN(rect->y1, (int32_t)height - 1); y++) {
		vid_draw_hline(frame, width, height, &px, rect->x0, rect->x1, y);
	}

	return 0;
}

/* The largest built-in bitmap font, or NULL when none is compiled in */
static const struct cfb_font *vid_draw_font(void)
{
	const struct cfb_font *best = NULL;

#if defined(CONFIG_CHARACTER_FRAMEBUFFER)
	STRUCT_SECTION_FOREACH(cfb_font, font) {
		if ((font->caps & CFB_FONT_MONO_VPACKED) != 0 &&
		    (best == NULL || font->height > best->height)) {
			best = font;
		}
	}
#endif

	return best;
}

/* Whether the glyph of @p c has its pixel (@p gx, @p gy) set */
static bool vid_draw_glyph_pixel(const struct cfb_font *font, char c, uint8_t gx, uint8_t gy)
{
	const uint8_t bytes_per_col = DIV_ROUND_UP(font->height, 8U);
	const uint8_t *glyph;
	uint8_t byte;
	uint8_t bit;

	if ((uint8_t)c < font->first_char || (uint8_t)c > font->last_char) {
		return false;
	}

	glyph = (const uint8_t *)font->data +
		((uint8_t)c - font->first_char) * font->width * bytes_per_col;
	byte = glyph[gx * bytes_per_col + gy / 8U];
	bit = ((font->caps & CFB_FONT_MSB_FIRST) != 0) ? (7U - (gy % 8U)) : (gy % 8U);

	return ((byte >> bit) & 1U) != 0U;
}

int mpipe_vid_draw_text_size(uint8_t scale, const char *text, uint32_t *w, uint32_t *h)
{
	const struct cfb_font *font = vid_draw_font();

	if (font == NULL) {
		return -ENOTSUP;
	}

	if (w != NULL) {
		*w = (text != NULL ? (uint32_t)strlen(text) : 0U) * font->width * scale;
	}
	if (h != NULL) {
		*h = (uint32_t)font->height * scale;
	}

	return 0;
}

int mpipe_vid_draw_text(uint8_t *frame, uint16_t width, uint16_t height, uint32_t pixfmt, int32_t x,
			int32_t y, uint8_t scale, uint32_t argb, const char *text)
{
	const struct cfb_font *font = vid_draw_font();
	struct vid_draw_px px;
	int ret;

	if (frame == NULL || text == NULL || scale == 0U) {
		return -EINVAL;
	}

	if (font == NULL) {
		return -ENOTSUP;
	}

	ret = vid_draw_pack(pixfmt, argb, &px);
	if (ret != 0) {
		return ret;
	}

	for (; *text != '\0'; text++, x += (int32_t)font->width * scale) {
		if (x >= (int32_t)width || x + (int32_t)font->width * scale <= 0) {
			continue;
		}

		for (uint8_t gy = 0; gy < font->height; gy++) {
			for (uint8_t gx = 0; gx < font->width; gx++) {
				if (!vid_draw_glyph_pixel(font, *text, gx, gy)) {
					continue;
				}

				for (uint8_t sy = 0; sy < scale; sy++) {
					vid_draw_hline(frame, width, height, &px,
						       x + (int32_t)gx * scale,
						       x + (int32_t)gx * scale + scale - 1,
						       y + (int32_t)gy * scale + sy);
				}
			}
		}
	}

	return 0;
}

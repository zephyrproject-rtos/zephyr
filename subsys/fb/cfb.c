/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_CFB_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cfb);

STRUCT_SECTION_START_EXTERN(cfb_font);
STRUCT_SECTION_END_EXTERN(cfb_font);

#define LSB_BIT_MASK(x) BIT_MASK(x)
#define MSB_BIT_MASK(x) (BIT_MASK(x) << (8 - x))

static inline uint8_t byte_reverse(uint8_t b)
{
	b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
	b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
	b = (b & 0xaa) >> 1 | (b & 0x55) << 1;
	return b;
}

struct char_framebuffer {
	/** Pointer to a buffer in RAM */
	uint8_t *buf;

	/** Size of the framebuffer */
	uint32_t size;

	/** Pointer to the font entry array */
	const struct cfb_font *fonts;

	/** Display pixel format */
	enum display_pixel_format pixel_format;

	/** Display screen info */
	enum display_screen_info screen_info;

	/** Resolution of a framebuffer in pixels in X direction */
	uint16_t x_res;

	/** Resolution of a framebuffer in pixels in Y direction */
	uint16_t y_res;

	/** Number of pixels per tile, typically 8 */
	uint8_t ppt;

	/** Number of available fonts */
	uint8_t numof_fonts;

	/** Current font index */
	uint8_t font_idx;

	/** Font kerning */
	int8_t kerning;

	/** Inverted */
	bool inverted;
};

static struct char_framebuffer char_fb;

static inline uint8_t *get_glyph_ptr(const struct cfb_font *fptr, char c)
{
	return (uint8_t *)fptr->data +
	       (c - fptr->first_char) *
	       (fptr->width * fptr->height / 8U);
}

static inline uint8_t get_glyph_byte(uint8_t *glyph_ptr, const struct cfb_font *fptr,
				     uint8_t x, uint8_t y)
{
	if (fptr->caps & CFB_FONT_MONO_VPACKED) {
		return glyph_ptr[x * (fptr->height / 8U) + y];
	} else if (fptr->caps & CFB_FONT_MONO_HPACKED) {
		return glyph_ptr[y * (fptr->width) + x];
	}

	LOG_WRN("Unknown font type");
	return 0;
}

/*
 * Draw the monochrome character in the monochrome tiled framebuffer,
 * a byte is interpreted as 8 pixels ordered vertically among each other.
 */
static uint8_t draw_char_vtmono(const struct char_framebuffer *fb,
				char c, uint16_t x, uint16_t y,
				bool draw_bg)
{
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);
	const bool font_is_msbfirst = ((fptr->caps & CFB_FONT_MSB_FIRST) != 0);
	const bool need_reverse =
		(((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0) != font_is_msbfirst);
	uint8_t *glyph_ptr;

	if (c < fptr->first_char || c > fptr->last_char) {
		c = ' ';
	}

	glyph_ptr = get_glyph_ptr(fptr, c);
	if (!glyph_ptr) {
		return 0;
	}

	for (size_t g_x = 0; g_x < fptr->width; g_x++) {
		const int16_t fb_x = x + g_x;

		for (size_t g_y = 0; g_y < fptr->height;) {
			/*
			 * Process glyph rendering in the y direction
			 * by separating per 8-line boundaries.
			 */

			const int16_t fb_y = y + g_y;
			const size_t fb_index = (fb_y / 8U) * fb->x_res + fb_x;
			const size_t offset = y % 8;
			const uint8_t bottom_lines = ((offset + fptr->height) % 8);
			uint8_t bg_mask;
			uint8_t byte;
			uint8_t next_byte;

			if (fb_x < 0 || fb->x_res <= fb_x || fb_y < 0 || fb->y_res <= fb_y) {
				g_y++;
				continue;
			}

			if (offset == 0 || g_y == 0) {
				/*
				 * The case of drawing the first line of the glyphs or
				 * starting to draw with a tile-aligned position case.
				 * In this case, no character is above it.
				 * So, we process assume that nothing is drawn above.
				 */
				byte = 0;
				next_byte = get_glyph_byte(glyph_ptr, fptr, g_x, g_y / 8);
			} else {
				byte = get_glyph_byte(glyph_ptr, fptr, g_x, g_y / 8);
				next_byte = get_glyph_byte(glyph_ptr, fptr, g_x, (g_y + 8) / 8);
			}

			if (font_is_msbfirst) {
				/*
				 * Extract the necessary 8 bits from the combined 2 tiles of glyphs.
				 */
				byte = ((byte << 8) | next_byte) >> (offset);

				if (g_y == 0) {
					/*
					 * Create a mask that does not draw offset white space.
					 */
					bg_mask = BIT_MASK(8 - offset);
				} else {
					/*
					 * The drawing of the second line onwards
					 * is aligned with the tile, so it draws all the bits.
					 */
					bg_mask = 0xFF;
				}
			} else {
				byte = ((next_byte << 8) | byte) >> (8 - offset);
				if (g_y == 0) {
					bg_mask = BIT_MASK(8 - offset) << offset;
				} else {
					bg_mask = 0xFF;
				}
			}

			/*
			 * Clip the bottom margin to protect existing draw contents.
			 */
			if (((fptr->height - g_y) < 8) && (bottom_lines != 0)) {
				const uint8_t clip = font_is_msbfirst ? MSB_BIT_MASK(bottom_lines)
								      : LSB_BIT_MASK(bottom_lines);

				bg_mask &= clip;
				byte &= clip;
			}

			if (draw_bg) {
				if (need_reverse) {
					bg_mask = byte_reverse(bg_mask);
				}
				fb->buf[fb_index] &= ~bg_mask;
			}

			if (need_reverse) {
				byte = byte_reverse(byte);
			}
			fb->buf[fb_index] |= byte;

			if (g_y == 0) {
				g_y += (8 - offset);
			} else if ((fptr->height - g_y) >= 8) {
				g_y += 8;
			} else {
				g_y += bottom_lines;
			}
		}
	}

	return fptr->width;
}

static inline void draw_point(struct char_framebuffer *fb, int16_t x, int16_t y)
{
	const bool need_reverse = ((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);
	const size_t index = ((y / 8) * fb->x_res);
	uint8_t m = BIT(y % 8);

	if (x < 0 || x >= fb->x_res) {
		return;
	}

	if (y < 0 || y >= fb->y_res) {
		return;
	}

	if (need_reverse) {
		m = byte_reverse(m);
	}

	fb->buf[index + x] |= m;
}

static void draw_line(struct char_framebuffer *fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t dx = (sx > 0) ? (x1 - x0) : (x0 - x1);
	int16_t dy = (sy > 0) ? (y0 - y1) : (y1 - y0);
	int16_t err = dx + dy;
	int16_t e2;

	while (true) {
		draw_point(fb, x0, y0);

		if (x0 == x1 && y0 == y1) {
			break;
		}

		e2 = 2 * err;

		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		}

		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

static int draw_text(const struct device *dev, const char *const str, int16_t x, int16_t y,
		     bool wrap)
{
	const struct char_framebuffer *fb = &char_fb;
	const struct cfb_font *fptr;

	if (!fb->fonts || !fb->buf) {
		return -ENODEV;
	}

	fptr = &(fb->fonts[fb->font_idx]);

	if (fptr->height % 8) {
		LOG_ERR("Wrong font size");
		return -EINVAL;
	}

	if ((fb->screen_info & SCREEN_INFO_MONO_VTILED)) {
		const size_t len = strlen(str);

		for (size_t i = 0; i < len; i++) {
			if ((x + fptr->width > fb->x_res) && wrap) {
				x = 0U;
				y += fptr->height;
			}
			x += fb->kerning + draw_char_vtmono(fb, str[i], x, y, wrap);
		}
		return 0;
	}

	LOG_ERR("Unsupported framebuffer configuration");
	return -EINVAL;
}

int cfb_draw_point(const struct device *dev, const struct cfb_position *pos)
{
	struct char_framebuffer *fb = &char_fb;

	draw_point(fb, pos->x, pos->y);

	return 0;
}

int cfb_draw_line(const struct device *dev, const struct cfb_position *start,
		  const struct cfb_position *end)
{
	struct char_framebuffer *fb = &char_fb;

	draw_line(fb, start->x, start->y, end->x, end->y);

	return 0;
}

int cfb_draw_rect(const struct device *dev, const struct cfb_position *start,
		  const struct cfb_position *end)
{
	struct char_framebuffer *fb = &char_fb;

	draw_line(fb, start->x, start->y, end->x, start->y);
	draw_line(fb, end->x, start->y, end->x, end->y);
	draw_line(fb, end->x, end->y, start->x, end->y);
	draw_line(fb, start->x, end->y, start->x, start->y);

	return 0;
}

int cfb_draw_text(const struct device *dev, const char *const str, int16_t x, int16_t y)
{
	return draw_text(dev, str, x, y, false);
}

int cfb_print(const struct device *dev, const char *const str, uint16_t x, uint16_t y)
{
	return draw_text(dev, str, x, y, true);
}

int cfb_invert_area(const struct device *dev, uint16_t x, uint16_t y,
		    uint16_t width, uint16_t height)
{
	const struct char_framebuffer *fb = &char_fb;
	const bool need_reverse = ((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);

	if (x >= fb->x_res || y >= fb->y_res) {
		LOG_ERR("Coordinates outside of framebuffer");

		return -EINVAL;
	}

	if ((fb->screen_info & SCREEN_INFO_MONO_VTILED)) {
		if (x > fb->x_res) {
			x = fb->x_res;
		}

		if (y > fb->y_res) {
			y = fb->y_res;
		}

		if (x + width > fb->x_res) {
			width = fb->x_res - x;
		}

		if (y + height > fb->y_res) {
			height = fb->y_res - y;
		}

		for (size_t i = x; i < x + width; i++) {
			for (size_t j = y; j < (y + height); j++) {
				/*
				 * Process inversion in the y direction
				 * by separating per 8-line boundaries.
				 */

				const size_t index = ((j / 8) * fb->x_res) + i;
				const uint8_t remains = y + height - j;

				/*
				 * Make mask to prevent overwriting the drawing contents that on
				 * between the start line or end line and the 8-line boundary.
				 */
				if ((j % 8) > 0) {
					uint8_t m = BIT_MASK((j % 8));
					uint8_t b = fb->buf[index];

					/*
					 * Generate mask for remaining lines in case of
					 * drawing within 8 lines from the start line
					 */
					if (remains < 8) {
						m |= BIT_MASK((8 - (j % 8) + remains))
						     << ((j % 8) + remains);
					}

					if (need_reverse) {
						m = byte_reverse(m);
					}

					fb->buf[index] = (b ^ (~m));
					j += 7 - (j % 8);
				} else if (remains >= 8) {
					/* No mask required if no start or end line is included */
					fb->buf[index] = ~fb->buf[index];
					j += 7;
				} else {
					uint8_t m = BIT_MASK(8 - remains) << (remains);
					uint8_t b = fb->buf[index];

					if (need_reverse) {
						m = byte_reverse(m);
					}

					fb->buf[index] = (b ^ (~m));
					j += (remains - 1);
				}
			}
		}

		return 0;
	}

	LOG_ERR("Unsupported framebuffer configuration");
	return -EINVAL;
}

static int cfb_invert(const struct char_framebuffer *fb)
{
	for (size_t i = 0; i < fb->x_res * fb->y_res / 8U; i++) {
		fb->buf[i] = ~fb->buf[i];
	}

	return 0;
}

int cfb_framebuffer_clear(const struct device *dev, bool clear_display)
{
	const struct char_framebuffer *fb = &char_fb;

	if (!fb || !fb->buf) {
		return -ENODEV;
	}

	memset(fb->buf, 0, fb->size);

	if (clear_display) {
		cfb_framebuffer_finalize(dev);
	}

	return 0;
}

int cfb_framebuffer_invert(const struct device *dev)
{
	struct char_framebuffer *fb = &char_fb;

	fb->inverted = !fb->inverted;

	return 0;
}

int cfb_framebuffer_finalize(const struct device *dev)
{
	const struct display_driver_api *api = dev->api;
	const struct char_framebuffer *fb = &char_fb;
	int err;

	if (!fb || !fb->buf) {
		return -ENODEV;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = fb->size,
		.width = fb->x_res,
		.height = fb->y_res,
		.pitch = fb->x_res,
	};

	if (!(fb->pixel_format & PIXEL_FORMAT_MONO10) != !(fb->inverted)) {
		cfb_invert(fb);
		err = api->write(dev, 0, 0, &desc, fb->buf);
		cfb_invert(fb);
		return err;
	}

	return api->write(dev, 0, 0, &desc, fb->buf);
}

int cfb_get_display_parameter(const struct device *dev,
			       enum cfb_display_param param)
{
	const struct char_framebuffer *fb = &char_fb;

	switch (param) {
	case CFB_DISPLAY_HEIGH:
		return fb->y_res;
	case CFB_DISPLAY_WIDTH:
		return fb->x_res;
	case CFB_DISPLAY_PPT:
		return fb->ppt;
	case CFB_DISPLAY_ROWS:
		if (fb->screen_info & SCREEN_INFO_MONO_VTILED) {
			return fb->y_res / fb->ppt;
		}
		return fb->y_res;
	case CFB_DISPLAY_COLS:
		if (fb->screen_info & SCREEN_INFO_MONO_VTILED) {
			return fb->x_res;
		}
		return fb->x_res / fb->ppt;
	}
	return 0;
}

int cfb_framebuffer_set_font(const struct device *dev, uint8_t idx)
{
	struct char_framebuffer *fb = &char_fb;

	if (idx >= fb->numof_fonts) {
		return -EINVAL;
	}

	fb->font_idx = idx;

	return 0;
}

int cfb_get_font_size(const struct device *dev, uint8_t idx, uint8_t *width,
		      uint8_t *height)
{
	const struct char_framebuffer *fb = &char_fb;

	if (idx >= fb->numof_fonts) {
		return -EINVAL;
	}

	if (width) {
		*width = TYPE_SECTION_START(cfb_font)[idx].width;
	}

	if (height) {
		*height = TYPE_SECTION_START(cfb_font)[idx].height;
	}

	return 0;
}

int cfb_set_kerning(const struct device *dev, int8_t kerning)
{
	char_fb.kerning = kerning;

	return 0;
}

int cfb_get_numof_fonts(const struct device *dev)
{
	const struct char_framebuffer *fb = &char_fb;

	return fb->numof_fonts;
}

int cfb_framebuffer_init(const struct device *dev)
{
	const struct display_driver_api *api = dev->api;
	struct char_framebuffer *fb = &char_fb;
	struct display_capabilities cfg;

	api->get_capabilities(dev, &cfg);

	STRUCT_SECTION_COUNT(cfb_font, &fb->numof_fonts);

	LOG_DBG("number of fonts %d", fb->numof_fonts);

	fb->x_res = cfg.x_resolution;
	fb->y_res = cfg.y_resolution;
	fb->ppt = 8U;
	fb->pixel_format = cfg.current_pixel_format;
	fb->screen_info = cfg.screen_info;
	fb->buf = NULL;
	fb->kerning = 0;
	fb->inverted = false;

	fb->fonts = TYPE_SECTION_START(cfb_font);
	fb->font_idx = 0U;

	fb->size = fb->x_res * fb->y_res / fb->ppt;
	fb->buf = k_malloc(fb->size);
	if (!fb->buf) {
		return -ENOMEM;
	}

	memset(fb->buf, 0, fb->size);

	return 0;
}

void cfb_framebuffer_deinit(const struct device *dev)
{
	struct char_framebuffer *fb = &char_fb;

	if (fb->buf) {
		k_free(fb->buf);
		fb->buf = NULL;
	}

}

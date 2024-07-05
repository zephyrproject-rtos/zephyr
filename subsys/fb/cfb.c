/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_CFB_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cfb);

STRUCT_SECTION_START_EXTERN(cfb_font);
STRUCT_SECTION_END_EXTERN(cfb_font);

#define LSB_BIT_MASK(x) BIT_MASK(x)
#define MSB_BIT_MASK(x) (BIT_MASK(x) << (8 - x))

/**
 * Command List processing mode
 */
enum command_process_mode {
	FINALIZE,
	IMMEDIATE,
	CLEAR_COMMANDS,
	CLEAR_DISPLAY,
};

static inline uint8_t byte_reverse(uint8_t b)
{
	b = (b & 0xf0) >> 4 | (b & 0x0f) << 4;
	b = (b & 0xcc) >> 2 | (b & 0x33) << 2;
	b = (b & 0xaa) >> 1 | (b & 0x55) << 1;
	return b;
}

static inline uint8_t *get_glyph_ptr(const struct cfb_font *fptr, char c)
{
	if (c < fptr->first_char || c > fptr->last_char) {
		return NULL;
	}

	return (uint8_t *)fptr->data +
	       (c - fptr->first_char) * (fptr->width * DIV_ROUND_UP(fptr->height, 8U));
}

static inline uint8_t get_glyph_byte(const uint8_t *glyph_ptr, const struct cfb_font *fptr,
				     uint8_t x, uint8_t y)
{
	if (!glyph_ptr) {
		return 0;
	}

	if (fptr->caps & CFB_FONT_MONO_VPACKED) {
		return glyph_ptr[x * DIV_ROUND_UP(fptr->height, 8U) + y];
	} else if (fptr->caps & CFB_FONT_MONO_HPACKED) {
		return glyph_ptr[y * (fptr->width) + x];
	}

	LOG_WRN("Unknown font type");
	return 0;
}

static inline const struct cfb_font *font_get(uint32_t idx)
{
	static const struct cfb_font *fonts = TYPE_SECTION_START(cfb_font);

	if (idx < cfb_get_numof_fonts()) {
		return fonts + idx;
	}

	return NULL;
}

static inline uint8_t pixels_per_tile(const enum display_pixel_format pixel_format)
{
	const uint32_t bits_per_pixel = DISPLAY_BITS_PER_PIXEL(pixel_format);

	return (bits_per_pixel < 8) ? (bits_per_pixel * 8) : 1;
}

static inline uint8_t bytes_per_pixel(const enum display_pixel_format pixel_format)
{
	const uint8_t bits_per_pixel = DISPLAY_BITS_PER_PIXEL(pixel_format);

	return (bits_per_pixel < 8) ? 1 : (bits_per_pixel / 8);
}

static inline uint16_t fb_top(const struct cfb_framebuffer *fb)
{
	return fb->pos.y;
}

static inline uint16_t fb_left(const struct cfb_framebuffer *fb)
{
	return fb->pos.x;
}

static inline uint16_t fb_bottom(const struct cfb_framebuffer *fb)
{
	return fb->pos.y + fb->height;
}

static inline uint16_t fb_right(const struct cfb_framebuffer *fb)
{
	return fb->pos.x + fb->width;
}

static inline uint32_t fb_screen_buf_size(const struct cfb_framebuffer *fb)
{
	return fb->res.x * fb->res.y * bytes_per_pixel(fb->pixel_format) /
	       pixels_per_tile(fb->pixel_format);
}

static bool check_font_in_rect(int16_t x, int16_t y, const struct cfb_font *fptr,
			       const struct cfb_framebuffer *fb)
{
	if ((x > fb_right(fb)) || (y > fb_bottom(fb)) || (x + fptr->width <= fb_left(fb)) ||
	    (y + fptr->height <= fb_top(fb))) {
		return false;
	}

	return true;
}

static inline bool fb_is_tiled(const struct cfb_framebuffer *fb)
{
	if ((fb->pixel_format == PIXEL_FORMAT_MONO01) ||
	    (fb->pixel_format == PIXEL_FORMAT_MONO10)) {
		return true;
	}

	return false;
}

static inline uint32_t rgba_to_color(const enum display_pixel_format pixel_format, uint8_t r,
				     uint8_t g, uint8_t b, uint8_t a)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_MONO01:
		if (r == g && g == b && b == a && a == 0) {
			return 0;
		} else {
			return 0xFFFFFFFF;
		}
	case PIXEL_FORMAT_MONO10:
		if (r == g && g == b && b == a && a == 0) {
			return 0xFFFFFFFF;
		} else {
			return 0;
		}
	case PIXEL_FORMAT_RGB_888:
		a = 0xFF;
		return a << 24 | r << 16 | g << 8 | b;
	case PIXEL_FORMAT_ARGB_8888:
		return a << 24 | r << 16 | g << 8 | b;
	case PIXEL_FORMAT_RGB_565:
		return sys_cpu_to_be16((r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3);
	case PIXEL_FORMAT_BGR_565:
		return ((r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3);
	default:
		return 0;
	}
}

static inline void color_to_rgba(const enum display_pixel_format pixel_format, uint32_t color,
				 uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_MONO01:
		if (color) {
			*a = *r = *g = *b = 0xFF;
		} else {
			*a = *r = *g = *b = 0x0;
		}
		return;
	case PIXEL_FORMAT_MONO10:
		*a = 0;
		if (color) {
			*a = *r = *g = *b = 0x0;
		} else {
			*a = *r = *g = *b = 0xFF;
		}
		return;
	case PIXEL_FORMAT_RGB_888:
		*a = 0xFF;
		*r = (color >> 16) & 0xFF;
		*g = (color >> 8) & 0xFF;
		*b = (color >> 0) & 0xFF;
		return;
	case PIXEL_FORMAT_ARGB_8888:
		*a = (color >> 24) & 0xFF;
		*r = (color >> 16) & 0xFF;
		*g = (color >> 8) & 0xFF;
		*b = (color >> 0) & 0xFF;
		return;
	case PIXEL_FORMAT_RGB_565:
		color = sys_be16_to_cpu(color);
		*a = 0xFF;
		*r = (color >> 8) & 0xF8;
		*g = (color >> 3) & 0xFC;
		*b = (color << 3) & 0xF8;
		return;
	case PIXEL_FORMAT_BGR_565:
		*a = 0xFF;
		*b = (color >> 8) & 0xF8;
		*g = (color >> 3) & 0xFC;
		*r = (color << 3) & 0xF8;
		return;
	default:
		return;
	}
}

#if defined(CONFIG_ZTEST)
void test_color_to_rgba(const enum display_pixel_format pixel_format, uint32_t color, uint8_t *r,
			uint8_t *g, uint8_t *b, uint8_t *a)
{
	color_to_rgba(pixel_format, color, r, g, b, a);
}

uint32_t test_rgba_to_color(const enum display_pixel_format pixel_format, uint8_t r, uint8_t g,
			    uint8_t b, uint8_t a)
{
	return rgba_to_color(pixel_format, r, g, b, a);
}

#endif /* CONFIG_ZTEST */

static inline void set_color_bytes(uint8_t *buf, uint8_t bpp, uint32_t color)
{
	if (bpp == 0 || bpp == 1) {
		*buf = color;
	} else if (bpp == 2) {
		*(uint16_t *)buf = color;
	} else if (bpp == 3) {
		buf[0] = color >> 16;
		buf[1] = color >> 8;
		buf[2] = color >> 0;
	} else if (bpp == 4) {
		*(uint32_t *)buf = color;
	}
}

static void fill_fb(struct cfb_framebuffer *fb, uint32_t color, size_t bpp)
{
	if (bpp == 1) {
		memset(fb->buf, color, fb->size);
	} else if (bpp == 2) {
		uint16_t *buf16 = (uint16_t *)fb->buf;

		for (size_t i = 0; i < fb->size / 2; i++) {
			buf16[i] = color;
		}
	} else if (bpp == 3) {
		for (size_t i = 0; i < fb->size; i++) {
			fb->buf[i] = color >> (8 * (2 - (i % 3)));
		}
	} else if (bpp == 4) {
		uint32_t *buf32 = (uint32_t *)fb->buf;

		for (size_t i = 0; i < fb->size / 4; i++) {
			buf32[i] = color;
		}
	}
}

/*
 * Draw the monochrome character in the monochrome tiled framebuffer,
 * a byte is interpreted as 8 pixels ordered vertically among each other.
 */
static uint8_t draw_char_vtmono(struct cfb_framebuffer *fb, char c, int16_t x, int16_t y,
				const struct cfb_font *fptr, bool draw_bg, uint32_t fg_color)
{
	const uint8_t *glyph_ptr = get_glyph_ptr(fptr, c);
	const uint8_t draw_width = MIN(fptr->width, fb_right(fb) - x);
	const uint8_t draw_height = MIN(fptr->height, fb_bottom(fb) - y);
	const bool font_is_msbfirst = ((fptr->caps & CFB_FONT_MSB_FIRST) != 0);
	const bool need_reverse =
		(((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0) != font_is_msbfirst);

	if (!check_font_in_rect(x, y, fptr, fb)) {
		return fptr->width;
	}

	for (size_t g_x = 0; g_x < draw_width; g_x++) {
		const int16_t fb_x = x + g_x - fb->pos.x;

		if (fb_x < 0 || fb->res.x <= fb_x) {
			continue;
		}

		for (size_t g_y = 0; g_y < draw_height;) {
			/*
			 * Process glyph rendering in the y direction
			 * by separating per 8-line boundaries.
			 */

			const int16_t fb_y = y + g_y - fb->pos.y;
			const size_t fb_index = (fb_y / 8U) * fb->width + fb_x;
			const size_t offset = (y >= 0) ? y % 8 : 8 + (y % 8);
			const uint8_t bottom_lines = (offset + fptr->height) % 8;
			uint8_t bg_mask;
			uint8_t byte;
			uint8_t next_byte;

			if (fb_y < 0 || fb->height <= fb_y) {
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

				if (font_is_msbfirst) {
					bg_mask = BIT_MASK(8 - offset);
				} else {
					bg_mask = BIT_MASK(8 - offset) << offset;
				}
			} else {
				byte = get_glyph_byte(glyph_ptr, fptr, g_x, g_y / 8);
				next_byte = get_glyph_byte(glyph_ptr, fptr, g_x, (g_y + 8) / 8);
				bg_mask = 0xFF;
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
				if (fg_color) {
					fb->buf[fb_index] &= ~bg_mask;
				} else {
					fb->buf[fb_index] |= bg_mask;
				}
			}

			if (need_reverse) {
				byte = byte_reverse(byte);
			}

			if (fg_color) {
				fb->buf[fb_index] |= byte;
			} else {
				fb->buf[fb_index] &= ~byte;
			}

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

static uint8_t draw_char_color(struct cfb_framebuffer *fb, char c, int16_t x, int16_t y,
			       const struct cfb_font *fptr, bool draw_bg, uint32_t fg_color,
			       uint32_t bg_color)
{
	const uint8_t *glyph_ptr = get_glyph_ptr(fptr, c);
	const uint8_t draw_width = MIN(fptr->width, fb_right(fb) - x);
	const uint8_t draw_height = MIN(fptr->height, fb_bottom(fb) - y);

	if (!check_font_in_rect(x, y, fptr, fb)) {
		return fptr->width;
	}

	for (size_t g_x = 0; g_x < draw_width; g_x++) {
		const int16_t fb_x = x + g_x - fb->pos.x;

		if ((fb_x < 0) || (fb->res.x <= fb_x)) {
			continue;
		}

		for (size_t g_y = 0; g_y < draw_height; g_y++) {
			const size_t b = g_y % 8;
			const uint8_t pos = (fptr->caps & CFB_FONT_MSB_FIRST) ? BIT(7 - b) : BIT(b);
			const int16_t fb_y = y + g_y - fb->pos.y;
			const size_t fb_index =
				(fb_y * fb->width + fb_x) * bytes_per_pixel(fb->pixel_format);
			const uint8_t byte = get_glyph_byte(glyph_ptr, fptr, g_x, g_y / 8);

			if ((fb_y < 0) || (fb->height <= fb_y)) {
				continue;
			}

			if (byte & pos) {
				set_color_bytes(&fb->buf[fb_index],
						bytes_per_pixel(fb->pixel_format), fg_color);
			} else {
				if (draw_bg) {
					set_color_bytes(&fb->buf[fb_index],
							bytes_per_pixel(fb->pixel_format),
							bg_color);
				}
			}
		}
	}

	return fptr->width;
}

static inline void draw_point(struct cfb_framebuffer *fb, int16_t x, int16_t y, uint32_t fg_color)
{
	const bool need_reverse = ((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);
	const int16_t x_off = x - fb->pos.x;
	const int16_t y_off = y - fb->pos.y;

	if (x < fb_left(fb) || x >= fb_right(fb)) {
		return;
	}

	if (y < fb_top(fb) || y >= fb_bottom(fb)) {
		return;
	}

	if (fb_is_tiled(fb)) {
		const size_t index = (y_off / 8) * fb->width;
		const uint8_t m = BIT(y_off % 8);

		if (fg_color) {
			fb->buf[index + x_off] |= (need_reverse ? byte_reverse(m) : m);
		} else {
			fb->buf[index + x_off] &= ~(need_reverse ? byte_reverse(m) : m);
		}
	} else {
		const size_t index =
			(y_off * fb->width + x_off) * bytes_per_pixel(fb->pixel_format);

		set_color_bytes(&fb->buf[index], bytes_per_pixel(fb->pixel_format), fg_color);
	}
}

static void draw_line(struct cfb_framebuffer *fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		      uint32_t fg_color)
{
	int16_t sx = (x0 < x1) ? 1 : -1;
	int16_t sy = (y0 < y1) ? 1 : -1;
	int16_t dx = (sx > 0) ? (x1 - x0) : (x0 - x1);
	int16_t dy = (sy > 0) ? (y0 - y1) : (y1 - y0);
	int16_t err = dx + dy;
	int16_t e2;

	while (true) {
		draw_point(fb, x0, y0, fg_color);

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

static void draw_text(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y,
		      bool print, const struct cfb_font *fptr, int8_t kerning, uint32_t fg_color,
		      uint32_t bg_color)
{
	for (size_t i = 0; i < strlen(str); i++) {
		if ((x + fptr->width > fb->res.x) && print) {
			x = 0U;
			y += fptr->height;
		}

		if (fb_is_tiled(fb)) {
			x += draw_char_vtmono(fb, str[i], x, y, fptr, print, fg_color);
		} else {
			x += draw_char_color(fb, str[i], x, y, fptr, print, fg_color, bg_color);
		}

		x += kerning;
	}
}

static void invert_area(struct cfb_framebuffer *fb, int16_t x, int16_t y, uint16_t width,
			uint16_t height)
{
	const bool need_reverse = ((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);

	if ((x + width) < fb_left(fb) || x >= fb_right(fb)) {
		return;
	}

	if ((y + height) < fb_top(fb) || y >= fb_bottom(fb)) {
		return;
	}

	x -= fb->pos.x;
	y -= fb->pos.y;

	if (x < 0) {
		width += x;
		x = 0;
	}

	if (y < 0) {
		height += y;
		y = 0;
	}

	width = MIN(width, fb->width - x);
	height = MIN(height, fb->height - y);

	for (int i = x; i < x + width; i++) {
		if (i < 0 || i >= fb->width) {
			continue;
		}

		for (int j = y; j < y + height; j++) {
			if (j < 0 || j >= fb->height) {
				continue;
			}

			if (fb_is_tiled(fb)) {
				/*
				 * Process inversion in the y direction
				 * by separating per 8-line boundaries.
				 */

				const size_t index = ((j / 8) * fb->width) + i;
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
			} else {
				const size_t index =
					(j * fb->width + i) * bytes_per_pixel(fb->pixel_format);

				if (index > fb->size) {
					continue;
				}

				if (bytes_per_pixel(fb->pixel_format) == 1) {
					fb->buf[index] = ~fb->buf[index];
				} else if (bytes_per_pixel(fb->pixel_format) == 2) {
					*(uint16_t *)&fb->buf[index] =
						~(*(uint16_t *)&fb->buf[index]);
				} else if (bytes_per_pixel(fb->pixel_format) == 3) {
					fb->buf[index] = ~fb->buf[index];
					fb->buf[index + 1] = ~fb->buf[index + 1];
					fb->buf[index + 2] = ~fb->buf[index + 2];
				} else if (bytes_per_pixel(fb->pixel_format) == 4) {
					*(uint32_t *)&fb->buf[index] =
						~(*(uint32_t *)&fb->buf[index]);
				}
			}
		}
	}
}

static void execute_command(struct cfb_framebuffer *fb, const struct cfb_command_param *param,
			    struct cfb_draw_settings *settings)
{
	uint32_t tmp_color;

	switch (param->op) {
	case CFB_OP_FILL:
		fill_fb(fb, settings->bg_color, bytes_per_pixel(fb->pixel_format));
		break;
	case CFB_OP_DRAW_POINT:
		draw_point(fb, param->draw_figure.start.x, param->draw_figure.start.y,
			   settings->fg_color);
		break;
	case CFB_OP_DRAW_LINE:
		draw_line(fb, param->draw_figure.start.x, param->draw_figure.start.y,
			  param->draw_figure.end.x, param->draw_figure.end.y, settings->fg_color);
		break;
	case CFB_OP_DRAW_RECT:
		draw_line(fb, param->draw_figure.start.x, param->draw_figure.start.y,
			  param->draw_figure.end.x, param->draw_figure.start.y, settings->fg_color);
		draw_line(fb, param->draw_figure.end.x, param->draw_figure.start.y,
			  param->draw_figure.end.x, param->draw_figure.end.y, settings->fg_color);
		draw_line(fb, param->draw_figure.end.x, param->draw_figure.end.y,
			  param->draw_figure.start.x, param->draw_figure.end.y, settings->fg_color);
		draw_line(fb, param->draw_figure.start.x, param->draw_figure.end.y,
			  param->draw_figure.start.x, param->draw_figure.start.y,
			  settings->fg_color);
		break;
	case CFB_OP_DRAW_TEXT:
		draw_text(fb, param->draw_text.str, param->draw_text.pos.x, param->draw_text.pos.y,
			  false, font_get(settings->font_idx), settings->kerning,
			  settings->fg_color, settings->bg_color);
		break;
	case CFB_OP_PRINT:
		draw_text(fb, param->draw_text.str, param->draw_text.pos.x, param->draw_text.pos.y,
			  true, font_get(settings->font_idx), settings->kerning, settings->fg_color,
			  settings->bg_color);
		break;
	case CFB_OP_DRAW_TEXT_REF:
		draw_text(fb, param->draw_text.str, param->draw_text.pos.x, param->draw_text.pos.y,
			  false, font_get(settings->font_idx), settings->kerning,
			  settings->fg_color, settings->bg_color);
		break;
	case CFB_OP_PRINT_REF:
		draw_text(fb, param->draw_text.str, param->draw_text.pos.x, param->draw_text.pos.y,
			  true, font_get(settings->font_idx), settings->kerning, settings->fg_color,
			  settings->bg_color);
		break;
	case CFB_OP_INVERT_AREA:
		invert_area(fb, param->invert_area.x, param->invert_area.y, param->invert_area.w,
			    param->invert_area.h);
		break;
	case CFB_OP_SWAP_FG_BG_COLOR:
		tmp_color = settings->fg_color;
		settings->fg_color = settings->bg_color;
		settings->bg_color = tmp_color;
		break;
	case CFB_OP_SET_FONT:
		settings->font_idx = param->set_font.font_idx;
		break;
	case CFB_OP_SET_KERNING:
		settings->kerning = param->set_kerning.kerning;
		break;
	case CFB_OP_SET_FG_COLOR:
		settings->fg_color = rgba_to_color(fb->pixel_format, param->set_color.red,
						   param->set_color.green, param->set_color.blue,
						   param->set_color.alpha);
		break;
	case CFB_OP_SET_BG_COLOR:
		settings->bg_color = rgba_to_color(fb->pixel_format, param->set_color.red,
						   param->set_color.green, param->set_color.blue,
						   param->set_color.alpha);
		break;
	default:
		return;
	}
}

static bool display_is_last_iterator(struct cfb_command_iterator *ite)
{
	return !ite->node &&
	       (!ite->disp->cmd.buf ||
		((void *)ite->param >= (void *)(ite->disp->cmd.buf + ite->disp->cmd.pos)));
}

/**
 * Advances the iterator by one.
 *
 * This iterator handles linked list structures and buffers in a unified manner.
 * When the end of the linked list is reached, it scans the elements stored in the buffer
 * and returns NULL when the last element is reached.
 * This function is called via the next function pointer of cfb_command_iterator.
 *
 * @param ite current iterator
 * @return iterator that is pointing next node
 */
static struct cfb_command_iterator *display_next_iterator(struct cfb_command_iterator *ite)
{
	if (ite->node != NULL) {
		struct cfb_command *pcmd;

		ite->node = ite->node->next;
		if (ite->node == NULL) {
			ite->param = (void *)ite->disp->cmd.buf;
		} else {
			pcmd = CONTAINER_OF(ite->node, struct cfb_command, node);
			ite->param = &pcmd->param;
		}
	} else {
		uint8_t *buf_ptr = (uint8_t *)(ite->param + 1);

		ite->node = NULL;
		if (ite->param->op == CFB_OP_DRAW_TEXT || ite->param->op == CFB_OP_PRINT) {
			buf_ptr += strlen(buf_ptr) + 1;
		}

		ite->param = (struct cfb_command_param *)buf_ptr;
	}

	return ite;
}

/**
 * Initialize iterator.
 *
 * This function initialize command-list iterator.
 * Iterator traverse linked lists and buffers in a same manner.
 * This function sets the iterator to the beginning of the linked list.
 * The iterator cannot process concurrently.
 *
 * @param fb framebuffer pointer that is linked to display
 * @return Pointer that reference the iterator.
 */
static struct cfb_command_iterator *display_init_iterator(struct cfb_framebuffer *fb)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	struct cfb_command *pcmd;

	disp->iterator.disp = disp;
	disp->iterator.node = sys_slist_peek_head(&disp->cmd_list);
	pcmd = CONTAINER_OF(disp->iterator.node, struct cfb_command, node);
	disp->iterator.param = &pcmd->param;
	disp->iterator.next = display_next_iterator;
	disp->iterator.is_last = display_is_last_iterator;

	return &disp->iterator;
}

/**
 * Executes a list of commands.
 * Called by cfb_finalize and cfb_clear.
 * When called from cfb_clear, only apply the settings without executing the drawing command.
 *
 * @param fb Framebuffer and rendering info
 * @param x The start x pos of rendering rect
 * @param y The start y pos of rendering rect
 * @param w The width of rendering rect
 * @param h The height of rendering rect
 * @param settings Pointer to draw settings
 * @param mode Execution mode
 *
 * @return negative value if failed, otherwise 0
 */
static int display_process_commands(struct cfb_display *disp, uint16_t x, uint16_t y, uint16_t w,
				    uint16_t h, struct cfb_draw_settings *settings,
				    enum command_process_mode mode)
{
	struct display_buffer_descriptor desc;
	const uint16_t draw_width = x + w;
	const uint16_t draw_height = y + h;
	const uint16_t start_x = x;
	const struct cfb_draw_settings restore = *settings;
	int err = 0;

	if (disp->fb.size < (w * bytes_per_pixel(disp->fb.pixel_format))) {
		w = DIV_ROUND_UP(w * bytes_per_pixel(disp->fb.pixel_format),
				 DIV_ROUND_UP(w * bytes_per_pixel(disp->fb.pixel_format),
					      disp->fb.size)) /
		    bytes_per_pixel(disp->fb.pixel_format);
		h = pixels_per_tile(disp->fb.pixel_format);
	} else {
		h = MIN(disp->fb.size / (w * bytes_per_pixel(disp->fb.pixel_format)) *
				pixels_per_tile(disp->fb.pixel_format),
			h);
	}

	for (; y < draw_height; y += h) {
		for (x = start_x; x < draw_width; x += w) {
			*settings = restore;
			for (struct cfb_command_iterator *ite = disp->fb.init_iterator(&disp->fb);
			     !ite->is_last(ite); ite = ite->next(ite)) {
				disp->fb.pos.x = x;
				disp->fb.pos.y = y;
				disp->fb.width = w;
				disp->fb.height = h;

				/* Process only settings change commands if clear. */
				if (mode >= CLEAR_COMMANDS) {
					if ((ite->param->op != CFB_OP_FILL) &&
					    (ite->param->op != CFB_OP_SET_FONT) &&
					    (ite->param->op != CFB_OP_SET_KERNING) &&
					    (ite->param->op != CFB_OP_SET_FG_COLOR) &&
					    (ite->param->op != CFB_OP_SET_BG_COLOR) &&
					    (ite->param->op != CFB_OP_SWAP_FG_BG_COLOR)) {
						continue;
					}
				}

				execute_command(&disp->fb, ite->param, settings);
			}

			/* Don't update display on clear-command case */
			if (mode != FINALIZE && mode != CLEAR_DISPLAY) {
				continue;
			}

			desc.buf_size = disp->fb.size;
			desc.height = MIN(h, disp->fb.res.y - y);
			desc.width = MIN(w, disp->fb.res.x - x);
			desc.pitch = MIN(w, disp->fb.res.x - x);

			err = display_write(disp->dev, x, y, &desc, disp->fb.buf);

			if (err) {
				LOG_DBG("display_write(%d %d %d %d) size: %d: err=%d", x, y, w, h,
					disp->fb.size, err);
				return err;
			}
		}
	}

	return 0;
}

/**
 * Set up an initialization command to be executed every time partial frame buffer drawing.
 *
 * @param disp display structure
 */
static void display_append_init_commands(struct cfb_display *disp)
{
	const struct cfb_draw_settings *settings = &disp->settings;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 0;

	disp->init_cmds[CFB_INIT_CMD_SET_FONT].param.op = CFB_OP_SET_FONT;
	disp->init_cmds[CFB_INIT_CMD_SET_FONT].param.set_font.font_idx = settings->font_idx;

	disp->init_cmds[CFB_INIT_CMD_SET_KERNING].param.op = CFB_OP_SET_KERNING;
	disp->init_cmds[CFB_INIT_CMD_SET_KERNING].param.set_kerning.kerning = settings->kerning;

	color_to_rgba(disp->fb.pixel_format, settings->fg_color, &r, &g, &b, &a);
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.op = CFB_OP_SET_FG_COLOR;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.red = r;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.green = g;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.blue = b;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.alpha = a;

	color_to_rgba(disp->fb.pixel_format, settings->bg_color, &r, &g, &b, &a);
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.op = CFB_OP_SET_BG_COLOR;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.red = r;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.green = g;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.blue = b;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.alpha = a;

	disp->init_cmds[CFB_INIT_CMD_FILL].param.op = CFB_OP_FILL;

	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_FONT].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_KERNING].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_FILL].node);
}

/**
 * Run finalizing process.
 *
 * This function is called via function-pointer in the cfb_framebuffer
 * struct on calling cfb_finalize.
 *
 * @param fb framebuffer pointer that is linked to display
 * @param x X pos
 * @param y Y pos
 * @param w width
 * @param h height
 *
 * @return negative value if failed, otherwise 0
 */
static int display_finalize(struct cfb_framebuffer *fb, int16_t x, int16_t y, uint16_t width,
			    uint16_t height)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);

	return display_process_commands(disp, MAX(x, 0), MAX(y, 0), MIN(width, fb->res.x - x),
					MIN(height, fb->res.y - y), &disp->settings, FINALIZE);
}

/**
 * Clear commands and display.
 *
 * This function is called via function-pointer in the cfb_framebuffer
 * struct on calling cfb_clear.
 *
 * @param fb framebuffer pointer that is linked to display
 * @param clear_display Clears the display as well as the command buffer
 *
 * @return negative value if failed, otherwise 0
 */
static int display_clear(struct cfb_framebuffer *fb, bool clear_display)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	int err;

	if (disp->fb.size < fb_screen_buf_size(&disp->fb)) {
		/* if clear processing, filling with the background color is done last. */
		sys_slist_find_and_remove(&disp->cmd_list,
					  &disp->init_cmds[CFB_INIT_CMD_FILL].node);
		err = display_process_commands(disp, 0, 0, fb->res.x, fb->res.y, &disp->settings,
					       CLEAR_COMMANDS);
	} else {
		sys_slist_init(&disp->cmd_list);
	}

	sys_slist_init(&disp->cmd_list);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_FILL].node);
	err = display_process_commands(disp, 0, 0, fb->res.x, fb->res.y, &disp->settings,
				       clear_display ? CLEAR_DISPLAY : CLEAR_COMMANDS);

	/* reset command list */
	disp->cmd.pos = 0;
	memset(disp->cmd.buf, 0, disp->cmd.size);
	sys_slist_init(&disp->cmd_list);
	if (disp->fb.size < fb_screen_buf_size(&disp->fb)) {
		display_append_init_commands(disp);
	}

	return err;
}

static int display_append_command_to_buffer(struct cfb_display *disp, struct cfb_command *cmd)
{
	const bool store_text =
		(cmd->param.op == CFB_OP_DRAW_TEXT || cmd->param.op == CFB_OP_PRINT);
	const size_t str_len = store_text ? strlen(cmd->param.draw_text.str) + 1 : 0;
	const size_t allocate_size = sizeof(struct cfb_command) + str_len + 1;
	struct cfb_command_param *bufparam =
		(struct cfb_command_param *)&disp->cmd.buf[disp->cmd.pos];

	if (disp->cmd.size < disp->cmd.pos + allocate_size) {
		return -ENOBUFS;
	}

	memcpy(&disp->cmd.buf[disp->cmd.pos], &cmd->param, sizeof(struct cfb_command_param));

	disp->cmd.pos += sizeof(struct cfb_command_param);

	if (store_text) {
		bufparam->draw_text.str = (const char *)&disp->cmd.buf[disp->cmd.pos];
		memcpy(&disp->cmd.buf[disp->cmd.pos], cmd->param.draw_text.str, str_len);
		disp->cmd.pos += str_len;
		disp->cmd.buf[disp->cmd.pos] = '\0';
	}

	return 0;
}

/**
 * Append a command to buffer or list
 *
 * This function is called via function-pointer in the cfb_framebuffer
 * struct on calling cfb_append_command and rendering functions.
 *
 * If the entire screen buffer is allocated, the command is processed immediately
 * and written to the buffer.
 *
 * @param fb Framebuffer pointer that is linked to display
 * @param cmd A command to append buffer or list
 *
 * @retval -ENOBUFS Not enough buffers remain to append the command.
 * @retval 0 Succeedsed
 */
static int display_append_command(struct cfb_framebuffer *fb, struct cfb_command *cmd)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);

	if (!disp->cmd.buf) {
		sys_slist_append(&disp->cmd_list, &cmd->node);
		if (disp->fb.size >= fb_screen_buf_size(&disp->fb)) {
			execute_command(&disp->fb, &cmd->param, &disp->settings);
		}
		return 0;
	} else {
		return display_append_command_to_buffer(disp, cmd);
	}
}

int cfb_get_display_parameter(const struct cfb_display *disp, enum cfb_display_param param)
{
	struct display_capabilities cfg;

	display_get_capabilities(disp->dev, &cfg);

	switch (param) {
	case CFB_DISPLAY_HEIGH:
		return cfg.y_resolution;
	case CFB_DISPLAY_WIDTH:
		return cfg.x_resolution;
	case CFB_DISPLAY_PPT:
		return pixels_per_tile(disp->fb.pixel_format);
	case CFB_DISPLAY_ROWS:
		if (disp->fb.screen_info & SCREEN_INFO_MONO_VTILED) {
			return cfg.y_resolution / pixels_per_tile(disp->fb.pixel_format);
		}
		return cfg.y_resolution;
	case CFB_DISPLAY_COLS:
		if (disp->fb.screen_info & SCREEN_INFO_MONO_VTILED) {
			return cfg.x_resolution;
		}
		return cfg.x_resolution / pixels_per_tile(disp->fb.pixel_format);
	}
	return 0;
}

int cfb_get_font_size(uint8_t idx, uint8_t *width, uint8_t *height)
{
	if (idx >= cfb_get_numof_fonts()) {
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

int cfb_get_numof_fonts(void)
{
	static int numof_fonts;

	if (numof_fonts == 0) {
		STRUCT_SECTION_COUNT(cfb_font, &numof_fonts);
	}

	return numof_fonts;
}

int cfb_display_init(struct cfb_display *disp, const struct cfb_display_init_param *param)
{
	struct display_capabilities cfg;

	display_get_capabilities(param->dev, &cfg);

	disp->dev = param->dev;
	disp->settings.font_idx = 0U;
	disp->settings.kerning = 0U;

	if (cfg.current_pixel_format == PIXEL_FORMAT_MONO10) {
		disp->settings.bg_color = 0xFFFFFFFFU;
		disp->settings.fg_color = 0x0U;
	} else {
		disp->settings.fg_color = 0xFFFFFFFFU;
		disp->settings.bg_color = 0x0U;
	}

	disp->cmd.buf = param->command_buf;
	disp->cmd.size = param->command_buf_size;
	disp->cmd.pos = 0U;

	disp->fb.pixel_format = cfg.current_pixel_format;
	disp->fb.screen_info = cfg.screen_info;
	disp->fb.pos.x = 0;
	disp->fb.pos.y = 0;
	disp->fb.res.x = cfg.x_resolution;
	disp->fb.res.y = cfg.y_resolution;
	disp->fb.width = cfg.x_resolution;
	disp->fb.height = cfg.y_resolution;
	disp->fb.size = param->transfer_buf_size;
	disp->fb.buf = param->transfer_buf;

	disp->fb.finalize = display_finalize;
	disp->fb.clear = display_clear;
	disp->fb.append_command = display_append_command;
	disp->fb.init_iterator = display_init_iterator;

	if (!disp->fb.buf) {
		return -EINVAL;
	}

	if (disp->fb.size < bytes_per_pixel(cfg.current_pixel_format)) {
		return -EINVAL;
	}

	if (param->transfer_buf_size < fb_screen_buf_size(&disp->fb)) {
		if (!disp->cmd.buf) {
			return -EINVAL;
		}
	} else {
		disp->cmd.buf = NULL;
		disp->cmd.size = 0;
	}

	fill_fb(&disp->fb, disp->settings.bg_color, bytes_per_pixel(disp->fb.pixel_format));

	sys_slist_init(&disp->cmd_list);

	if (disp->cmd.buf) {
		memset(disp->cmd.buf, 0, disp->cmd.size);
	}

	display_append_init_commands(disp);

	return 0;
}

void cfb_display_deinit(struct cfb_display *disp)
{
	memset(disp, 0, sizeof(struct cfb_display));
}

struct cfb_display *cfb_display_alloc(const struct device *dev)
{
	struct display_capabilities cfg;
	struct cfb_display *disp;
	struct cfb_display_init_param param;
	size_t buf_size;
	int err;

	display_get_capabilities(dev, &cfg);

	param.dev = dev;
	param.transfer_buf_size = cfg.x_resolution * cfg.y_resolution *
				  bytes_per_pixel(cfg.current_pixel_format) /
				  pixels_per_tile(cfg.current_pixel_format);

	buf_size = ROUND_UP(sizeof(struct cfb_display), sizeof(void *)) +
		   ROUND_UP(param.transfer_buf_size, sizeof(void *));

	disp = k_malloc(buf_size);
	if (!disp) {
		return NULL;
	}

	param.transfer_buf = (uint8_t *)disp + ROUND_UP(sizeof(struct cfb_display), sizeof(void *));

	err = cfb_display_init(disp, &param);
	if (err) {
		k_free(disp);
		return NULL;
	}

	return disp;
}

void cfb_display_free(struct cfb_display *disp)
{
	k_free(disp);
}

struct cfb_framebuffer *cfb_display_get_framebuffer(struct cfb_display *disp)
{
	return &disp->fb;
}

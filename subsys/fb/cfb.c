/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
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

static inline uint8_t *get_glyph_ptr(const struct cfb_font *fptr, char c)
{
	return (uint8_t *)fptr->data +
	       (c - fptr->first_char) * (fptr->width * DIV_ROUND_UP(fptr->height, 8U));
}

static inline uint8_t get_glyph_byte(uint8_t *glyph_ptr, const struct cfb_font *fptr,
				     uint8_t x, uint8_t y)
{
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

/*
 * Draw the monochrome character in the monochrome tiled framebuffer,
 * a byte is interpreted as 8 pixels ordered vertically among each other.
 */
static uint8_t draw_char_vtmono(struct cfb_framebuffer *fb, char c, int16_t x, int16_t y,
				bool draw_bg)
{
	const struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	const struct cfb_font *fptr = font_get(disp->font_idx);
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
			const size_t fb_index = (fb_y / 8U) * fb->width + fb_x;
			const size_t offset = (y >= 0) ? y % 8 : (8 + (y % 8));
			const uint8_t bottom_lines = ((offset + fptr->height) % 8);
			uint8_t bg_mask;
			uint8_t byte;
			uint8_t next_byte;

			if (fb_x < 0 || fb->width <= fb_x || fb_y < 0 || fb->height <= fb_y) {
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

static inline void draw_point(struct cfb_framebuffer *fb, int16_t x, int16_t y)
{
	const bool need_reverse = ((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);
	const size_t index = ((y / 8) * fb->width);
	uint8_t m = BIT(y % 8);

	if (x < 0 || x >= fb->width) {
		return;
	}

	if (y < 0 || y >= fb->height) {
		return;
	}

	if (need_reverse) {
		m = byte_reverse(m);
	}

	fb->buf[index + x] |= m;
}

static void draw_line(struct cfb_framebuffer *fb, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
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

static int draw_text(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y,
		     bool wrap)
{
	const struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	const struct cfb_font *fptr;

	if (!fb->buf) {
		return -ENODEV;
	}

	fptr = font_get(disp->font_idx);

	if ((fb->screen_info & SCREEN_INFO_MONO_VTILED)) {
		for (size_t i = 0; i < strlen(str); i++) {
			if ((x + fptr->width > fb->width) && wrap) {
				x = 0U;
				y += fptr->height;
			}
			x += disp->kerning + draw_char_vtmono(fb, str[i], x, y, wrap);
		}
		return 0;
	}

	LOG_ERR("Unsupported framebuffer configuration");
	return -EINVAL;
}

int cfb_draw_point(struct cfb_framebuffer *fb, const struct cfb_position *pos)
{
	draw_point(fb, pos->x, pos->y);

	return 0;
}

int cfb_draw_line(struct cfb_framebuffer *fb, const struct cfb_position *start,
		  const struct cfb_position *end)
{
	draw_line(fb, start->x, start->y, end->x, end->y);

	return 0;
}

int cfb_draw_rect(struct cfb_framebuffer *fb, const struct cfb_position *start,
		  const struct cfb_position *end)
{
	draw_line(fb, start->x, start->y, end->x, start->y);
	draw_line(fb, end->x, start->y, end->x, end->y);
	draw_line(fb, end->x, end->y, start->x, end->y);
	draw_line(fb, start->x, end->y, start->x, start->y);

	return 0;
}

int cfb_draw_text(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y)
{
	return draw_text(fb, str, x, y, false);
}

int cfb_print(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y)
{
	return draw_text(fb, str, x, y, true);
}

int cfb_invert_area(struct cfb_framebuffer *fb, int16_t x, int16_t y, uint16_t width,
		    uint16_t height)
{
	const bool need_reverse = ((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0);

	if ((x + width) < 0 || x >= fb->width) {
		return 0;
	}

	if ((y + height) < 0 || y >= fb->height) {
		return 0;
	}

	if ((fb->screen_info & SCREEN_INFO_MONO_VTILED)) {
		if (x < 0) {
			width += x;
			x = 0;
		}

		if (y < 0) {
			height += y;
			y = 0;
		}

		if (width > (fb->width - x)) {
			width = fb->width - x;
		}

		if (height > (fb->height - y)) {
			height = fb->height - y;
		}

		for (size_t i = x; i < (x + width); i++) {
			for (size_t j = y; j < (y + height); j++) {
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
			}
		}

		return 0;
	}

	LOG_ERR("Unsupported framebuffer configuration");
	return -EINVAL;
}

static int buffer_invert(struct cfb_framebuffer *fb)
{
	for (size_t i = 0; i < fb->size; i++) {
		fb->buf[i] = ~fb->buf[i];
	}

	return 0;
}

int cfb_clear(struct cfb_framebuffer *fb, bool clear_display)
{
	if (!fb || !fb->buf) {
		return -ENODEV;
	}

	memset(fb->buf, 0, fb->size);

	if (clear_display) {
		cfb_finalize(fb);
	}

	return 0;
}

int cfb_invert(struct cfb_framebuffer *fb)
{
	struct cfb_display *disp;

	if (!fb) {
		return -ENODEV;
	}

	disp = CONTAINER_OF(fb, struct cfb_display, fb);
	disp->inverted = !disp->inverted;

	return 0;
}

int cfb_finalize(struct cfb_framebuffer *fb)
{
	struct display_buffer_descriptor desc;
	struct cfb_display *disp;
	int err;

	if (!fb || !fb->buf) {
		return -ENODEV;
	}

	disp = CONTAINER_OF(fb, struct cfb_display, fb);

	desc.buf_size = fb->size;
	desc.width = fb->width;
	desc.height = fb->height;
	desc.pitch = fb->width;

	if (!(fb->pixel_format & PIXEL_FORMAT_MONO10) != !(disp->inverted)) {
		buffer_invert(fb);
		err = display_write(disp->dev, 0, 0, &desc, fb->buf);
		buffer_invert(fb);
		return err;
	}

	return display_write(disp->dev, 0, 0, &desc, fb->buf);
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

int cfb_set_font(struct cfb_framebuffer *fb, uint8_t idx)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);

	if (idx >= cfb_get_numof_fonts()) {
		return -EINVAL;
	}

	disp->font_idx = idx;

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

int cfb_set_kerning(struct cfb_framebuffer *fb, int8_t kerning)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);

	disp->kerning = kerning;

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
	disp->fb.pixel_format = cfg.current_pixel_format;
	disp->fb.screen_info = cfg.screen_info;
	disp->fb.buf = NULL;
	disp->kerning = 0;
	disp->inverted = false;
	disp->fb.width = cfg.x_resolution;
	disp->fb.height = cfg.y_resolution;

	disp->font_idx = 0U;

	disp->fb.size = param->transfer_buf_size;
	disp->fb.buf = param->transfer_buf;

	memset(disp->fb.buf, 0, disp->fb.size);

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
	param.transfer_buf_size = cfg.x_resolution * cfg.y_resolution / 8U;

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

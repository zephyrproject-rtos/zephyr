/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_CFB_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cfb);

STRUCT_SECTION_START_EXTERN(cfb_font);
STRUCT_SECTION_END_EXTERN(cfb_font);

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

struct transfer_param {
	/** Pointer to character framebuffer */
	const struct char_framebuffer *fb;

	/** Pointer to device */
	const struct device *dev;

	/** Pointer to buffer */
	const uint8_t *buf;

	/** Size of the buffer */
	size_t size;

	/** Flag to disable transferring. Set true for rendering to framebuffer. */
	bool offscreen;
};

struct draw_text_params {
	/* Parameter for buffer transfer */
	struct transfer_param transfer;

	/* Text to draw */
	const char *const text;

	/* Text length */
	size_t length;

	/* X position to draw */
	int16_t x;

	/* Y position to draw */
	int16_t y;

	/* Rquired height to render the text */
	int16_t height;

	/* Pointer to font to use for render */
	const struct cfb_font *fptr;

	/* Indicate need to reverse significant bit */
	bool need_reverse;

	/* Number of characters in the first line */
	uint16_t firstrow_columns;

	/* Number of characters after the second line */
	uint16_t columns;

	/* Draw background before render text */
	bool background;
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
 * Transferring buffer contents to display
 *
 * @param param buffer transfer param
 * @param x X position
 * @param y X position
 * @param w width
 * @param h height
 *
 * @return negative value if failed, otherwise 0
 */
static int transfer_buffer(const struct transfer_param *param, int16_t x, int16_t y, int16_t w,
			   int16_t h)
{
	struct display_buffer_descriptor desc;
	int err = 0;

	if (param->offscreen) {
		return 0;
	}

	if (x >= param->fb->x_res || y >= param->fb->y_res) {
		return 0;
	}

	if ((y + h) < 0) {
		return 0;
	}

	if (y < 0) {
		h += y;
		y = 0;
	}

	desc.buf_size = param->size;
	desc.width = w;
	desc.height = h;
	desc.pitch = w;

	if (desc.height + y >= param->fb->y_res) {
		desc.height = param->fb->y_res - y;
	}

	if (desc.width + x >= param->fb->x_res) {
		desc.width = param->fb->x_res - x;
		desc.pitch = param->fb->x_res - x;
	}

	err = display_write(param->dev, x, y, &desc, param->buf);
	if (err) {
		LOG_DBG("write(%d %d %d %d) size: %d: err=%d", x, y, w, h, param->size, err);
	}

	return err;
}

/**
 * Calculate start X position and width specified Y position.
 *
 * @param param draw parameter
 * @param y Y position
 * @param [out] start Start draw position of x
 * @param [out] width Width to draw
 */
static void draw_text_width(const struct draw_text_params *param, int16_t y, int16_t *start,
			    uint16_t *width)
{
	const int line = (y - param->y) / param->fptr->height;
	const int last_line =
		(param->length - param->firstrow_columns + param->columns - 1) / param->columns;
	const int width_with_kerning = param->fptr->width + param->transfer.fb->kerning;

	if (line < 0) {
		*start = 0;
		*width = 0;
	} else if (line == 0) {
		size_t len = param->length;

		if (param->firstrow_columns < param->length) {
			len = (param->transfer.fb->x_res - param->x + param->transfer.fb->kerning) /
			      width_with_kerning;
		}

		*start = param->x;
		*width = len * width_with_kerning - param->transfer.fb->kerning;
	} else if (line == last_line) {
		*start = 0;
		*width =
			((param->length - (param->firstrow_columns + (line - 1) * param->columns)) *
			 width_with_kerning) -
			param->transfer.fb->kerning;
	} else if (line < last_line) {
		*start = 0;
		*width = param->transfer.fb->x_res;
	} else {
		*start = 0;
		*width = 0;
	}
}

/**
 * Query what char is drawn at this position.
 *
 * @param param draw parameter
 * @param x X position
 * @param y Y position
 * @param [out] index Index of character in param->text.
 * @param [out] xoffset X direction offset in font data
 * @param [out] yoffset X direction offset in font data
 *
 * @return true if character drawn at this position
 */
static bool char_to_draw_at_pos(const struct draw_text_params *param, int16_t x, int16_t y,
				size_t *index, uint16_t *xoffset, uint16_t *yoffset)
{
	const int width_with_kerning = param->fptr->width + param->transfer.fb->kerning;
	const int row = (y - param->y) / param->fptr->height;
	uint16_t idx;
	uint16_t off;

	if (y < param->y) {
		return false;
	}

	if (row == 0) {
		if ((x - param->x) < 0) {
			return false;
		}
		idx = (x - param->x) / width_with_kerning;
		if (idx >= param->firstrow_columns) {
			return false;
		}
		off = (x - param->x) % width_with_kerning;
	} else {
		const size_t col = x / width_with_kerning;

		idx = (param->columns * row) + col - (param->columns - param->firstrow_columns);
		off = x % width_with_kerning;
	}

	/* If x is on the kerning */
	if (off >= param->fptr->width) {
		return false;
	}

	if (idx >= param->length) {
		return false;
	}

	*index = idx;
	*xoffset = off;
	*yoffset = ((y - param->y) % param->fptr->height) / 8U;

	return true;
}

#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
/**
 * Query text region mask bits at this position.
 *
 * @param param draw parameter
 * @param x X position
 * @param y Y position
 *
 * @return bits of text region mask at this position
 */
static uint8_t text_region_mask_at_pos(const struct draw_text_params *param, uint16_t x, uint16_t y)
{
	const uint16_t xpos = x;
	const uint16_t ypos = y - param->y;
	const uint16_t offset = y % 8U;
	const uint16_t line = ypos / param->fptr->height;
	const bool firstrow = (ypos < param->fptr->height);
	const bool out_of_range = (ypos >= param->height);

	uint16_t prev_left;
	uint16_t prev_width;
	uint16_t prev_right;
	uint16_t left;
	uint16_t width;
	uint16_t right;

	draw_text_width(param, y, &left, &width);
	draw_text_width(param, y - 8, &prev_left, &prev_width);

	prev_right = prev_left + prev_width;
	right = left + width;

	if (out_of_range) {
		if (xpos < prev_right) {
			/* Mask the overhanging of characters that rendered in previous row */
			if (ypos < (param->fptr->height * line + 8U)) {
				return BIT_MASK(8U - offset);
			}
		}
	} else if (firstrow) {
		if (IN_RANGE(xpos, left, right - 1)) {
			/* Sets the offset mask for the top tile */
			if (ypos < (param->fptr->height * line + 8U)) {
				return ~BIT_MASK(8U - offset);
			}
			return 0x00;
		}
	} else {
		if (IN_RANGE(xpos, left, right - 1U)) {
			/* Set the offset mask to the part of the second line
			 * where nothing is written in the first line.
			 */
			if (!IN_RANGE(xpos, prev_left, prev_right - 1U)) {
				if (ypos < (param->fptr->height * line + 8U)) {
					return ~BIT_MASK(8U - offset);
				}
			}
			return 0x0;
		} else if (xpos < prev_right) {
			/* Mask the overhanging of characters that rendered in previous row */
			if (ypos < (param->fptr->height * line + 8U)) {
				return BIT_MASK(8U - offset);
			}
			return 0x0;
		}
	}

	return 0xFF;
}
#endif

/**
 * Draw the 8-rows specified by y pos
 *
 * @param param draw parameter
 * @param x X-position
 * @param y X-position
 * @param buf Buffer to render
 * @param buflen Size of the buffer
 */
static void draw_text_by_tile(const struct draw_text_params *param, uint16_t x, uint16_t y,
			      uint8_t *buf, size_t buflen)
{
	const uint16_t offset = param->y % 8U;
	uint8_t *glyph_ptr = NULL;
	char c = 0;

	for (size_t i = 0; i < buflen; i++) {
		const int sign = (param->y < 0) ? -1 : 1;
		const int16_t xpos = x + i;
		uint8_t byte[2];
		uint16_t xoff;
		uint16_t yoff;
		size_t idx;

		/* Get glyphs that required to draw the 8-rows from specified y position. */
		for (size_t j = 0; j < 2; j++) {
			const int16_t ypos = y + (j * -8U * sign);

			if (!char_to_draw_at_pos(param, xpos, ypos, &idx, &xoff, &yoff)) {
				byte[j] = 0;
				continue;
			}

			if (c != param->text[idx] || !glyph_ptr) {
				c = param->text[idx];
				if (c < param->fptr->first_char || c > param->fptr->last_char) {
					c = 0;
				}

				glyph_ptr = get_glyph_ptr(param->fptr, c);
				if (!glyph_ptr) {
					c = 0;
				}
			}

			if (c != 0) {
				byte[j] = get_glyph_byte(glyph_ptr, param->fptr, xoff, yoff);
			} else {
				/* Write nothing if no suitable font is found */
				byte[j] = 0;
			}
		}

		if (param->y < 0) {
			byte[0] >>= ((8 - offset) % 8);
			byte[0] |= (~BIT_MASK(offset - 1) & (byte[1] << (offset)));

			/* Trim the overhanging rows */
			if ((y + offset + 8) > param->height + param->y) {
				byte[0] &= BIT_MASK(offset + 1);
			}
		} else {
			byte[0] <<= offset;
			byte[0] |= (byte[1] >> (8U - offset));

			/* Trim the overhanging rows */
			if ((y + offset) > param->height + param->y) {
				byte[0] &= BIT_MASK(offset + 1);
			}
		}

		if (param->need_reverse) {
			byte[0] = byte_reverse(byte[0]);
		}

#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
		if (param->background) {
			buf[i] &= text_region_mask_at_pos(param, xpos, y);
		}

		buf[i] |= byte[0];
#else
		buf[i] = param->transfer.fb->inverted ? byte[0] : ~byte[0];
#endif
	}
}

/*
 * Draw the monochrome character in the monochrome tiled framebuffer,
 * a byte is interpreted as 8 pixels ordered vertically among each other.
 */
static int draw_text_vtmono(const struct draw_text_params *param, int16_t x, int16_t y)
{
	const int16_t y_end = ROUND_UP(param->height + param->y, 8U);
	const struct transfer_param *tr = &param->transfer;
	uint8_t *buf = (uint8_t *)tr->buf;
	uint16_t ystart = ROUND_DOWN(y, 8U);
	int err = 0;

	/* Draw row by row if the buffer is longer than the screen width */
	if (tr->fb->x_res <= tr->size) {
		const uint16_t buf_height = tr->size / tr->fb->x_res * 8U;
		uint16_t drawn_lines;

		for (; y < y_end; y += 8U) {
			drawn_lines = ROUND_DOWN(y - ystart, 8U);

			/* Flush when the buffer filled */
			if ((y - ystart) > (buf_height - 8U) && !tr->offscreen) {
				err = transfer_buffer(tr, 0, ystart, tr->fb->x_res, drawn_lines);
				if (err) {
					return err;
				}
				buf = (uint8_t *)tr->buf;

				ystart = ROUND_DOWN(y, 8U);
			}

			buf += x;
			draw_text_by_tile(param, x, y, buf, tr->fb->x_res - x);
			buf += (tr->fb->x_res - x);

			/* From the second line onwards, set the character start position to x=0 */
			if ((y - ystart + 8U + (param->y % 8U)) >= param->fptr->height) {
				x = 0;
			}
		}
		drawn_lines = ROUND_DOWN(y - ystart, 8U);

		/* Flush remaining buffers */
		return transfer_buffer(tr, 0, ystart, tr->fb->x_res, drawn_lines);
	}

	/* Buffer shorter than screen width case. */
	for (; y < y_end; y += 8U) {
		int16_t left_end[2];
		uint16_t row_width[2];

		for (int i = 0; i < 2; i++) {
			draw_text_width(param, y + (i * -8U), &left_end[i], &row_width[i]);
		}

		row_width[0] = MAX(left_end[0] + row_width[0], left_end[1] + row_width[1]);
		left_end[0] = MIN(left_end[0], left_end[1]);
		row_width[0] -= left_end[0];

		if (left_end[0] < 0) {
			row_width[0] += left_end[0];
			left_end[0] = 0;
		}

		x = left_end[0];
		ystart = ROUND_DOWN(y, 8U);

		/* draw row with splitting by buffer size */
		while (x < (row_width[0] + left_end[0] + tr->size)) {
			draw_text_by_tile(param, x, y, buf, tr->size);
			size_t len = tr->fb->x_res - x;

			if (len >= tr->size) {
				len = tr->size;
			}

			err = transfer_buffer(tr, x, ystart, len, 8U);
			if (err) {
				return err;
			}

			x += tr->size;
		}
	}

	return err;
}

#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
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
#endif

static inline uint16_t cfb_get_columns(const struct char_framebuffer *fb)
{
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);

	return (fb->x_res + fb->kerning) / (fptr->width + fb->kerning);
}

static inline uint16_t cfb_get_need_reverse(const struct char_framebuffer *fb)
{
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);

	return (((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0) !=
		((fptr->caps & CFB_FONT_MSB_FIRST) != 0));
}

static inline uint16_t cfb_get_firstrow_columns(const struct char_framebuffer *fb, uint16_t x)
{
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);

	return ((fb->x_res + fb->kerning) - x) / (fptr->width + fb->kerning);
}

static inline uint16_t cfb_get_draw_height(const struct char_framebuffer *fb,
					   const char *const str, uint16_t x)
{
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);
	const uint16_t columns = cfb_get_columns(fb);
	const uint16_t firstrow_columns = cfb_get_firstrow_columns(fb, x);

	return (((strlen(str) - firstrow_columns) + columns) / columns + 1) * fptr->height;
}

static int draw_text(const struct device *dev, const char *const str, int16_t x, int16_t y,
		     bool print_cmd)
{
	const struct char_framebuffer *fb = &char_fb;
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);

	const struct draw_text_params param = {
		.transfer = {
#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
			.buf = fb->buf + ((y < 0) ? 0 : (fb->x_res * (y / 8))),
			.size = (cfb_get_draw_height(fb, str, x) / 8) * fb->x_res,
			.offscreen = true,
#else
			.buf = fb->buf,
			.size = fb->size,
			.offscreen = false,
#endif
			.fb = &char_fb,
			.dev = dev,
		},
		.need_reverse = cfb_get_need_reverse(fb),
		.x = x,
		.y = y,
		.text = str,
		.length = strlen(str),
		.height = print_cmd ? cfb_get_draw_height(fb, str, x) : fptr->height,
		.fptr = fptr,
		.columns = cfb_get_columns(fb),
		.firstrow_columns = print_cmd ? cfb_get_firstrow_columns(fb, x) : UINT16_MAX,
		.background = print_cmd,
	};

	if (param.fptr->height % 8U) {
		LOG_ERR("Wrong font size");
		return -EINVAL;
	}

	if ((x + param.fptr->width > param.transfer.fb->x_res) && print_cmd) {
		x = 0U;
		y += param.fptr->height;
	}

	if (x < 0) {
		x = 0;
	}

	if (y < 0) {
		y = 0;
	}

	return draw_text_vtmono(&param, x, y);
}

int cfb_draw_point(const struct device *dev, const struct cfb_position *pos)
{
#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
	struct char_framebuffer *fb = &char_fb;

	draw_point(fb, pos->x, pos->y);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int cfb_draw_line(const struct device *dev, const struct cfb_position *start,
		  const struct cfb_position *end)
{
#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
	struct char_framebuffer *fb = &char_fb;

	draw_line(fb, start->x, start->y, end->x, end->y);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int cfb_draw_rect(const struct device *dev, const struct cfb_position *start,
		  const struct cfb_position *end)
{
#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
	struct char_framebuffer *fb = &char_fb;

	draw_line(fb, start->x, start->y, end->x, start->y);
	draw_line(fb, end->x, start->y, end->x, end->y);
	draw_line(fb, end->x, end->y, start->x, end->y);
	draw_line(fb, start->x, end->y, start->x, start->y);

	return 0;
#else
	return -ENOTSUP;
#endif
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
#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
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
#else
	return -ENOTSUP;
#endif
}

#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
static int cfb_invert(const struct char_framebuffer *fb)
{
	for (size_t i = 0; i < fb->x_res * fb->y_res / 8U; i++) {
		fb->buf[i] = ~fb->buf[i];
	}

	return 0;
}
#endif

int cfb_framebuffer_clear(const struct device *dev, bool clear_display)
{
	const struct char_framebuffer *fb = &char_fb;

#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
	if (!fb || !fb->buf) {
		return -ENODEV;
	}

	memset(fb->buf, 0, fb->size);

	if (clear_display) {
		cfb_framebuffer_finalize(dev);
	}
#else
	int err = 0;
	uint8_t *buf = fb->buf;
	const size_t size = fb->size;
	const struct draw_text_params param = {
		.transfer = {
			.fb = &char_fb,
			.dev = dev,
			.buf = buf,
			.size = size,
			.offscreen = false,
		},
		.text = NULL,
		.x = 0,
		.y = 0,
		.fptr = NULL,
	};

	if (!fb->buf) {
		return -ENODEV;
	}
	memset(buf, fb->inverted ? 0x00 : 0xFF, size);

	if (fb->size < fb->x_res) {
		for (int y = 0; y < fb->y_res; y += fb->ppt) {
			for (int x = 0; x < (fb->x_res + size); x += size) {
				err = transfer_buffer(&param.transfer, x, y, size, 8U);
				if (err) {
					return err;
				}
			}
		}
	} else {
		for (int y = 0; y < fb->y_res; y += fb->ppt) {
			const uint16_t buf_height = fb->size / fb->x_res * 8U;

			err = transfer_buffer(&param.transfer, 0, y, size, buf_height);
			if (err) {
				return err;
			}
		}
	}
#endif

	return 0;
}

int cfb_framebuffer_invert(const struct device *dev)
{
	struct char_framebuffer *fb = &char_fb;

	if (!fb) {
		return -ENODEV;
	}

	fb->inverted = !fb->inverted;

	return 0;
}

int cfb_framebuffer_finalize(const struct device *dev)
{
#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
	const struct char_framebuffer *fb = &char_fb;
	struct display_buffer_descriptor desc;
	int err;

	if (!fb || !fb->buf) {
		return -ENODEV;
	}

	desc.buf_size = fb->size;
	desc.width = fb->x_res;
	desc.height = fb->y_res;
	desc.pitch = fb->x_res;

	if (!(fb->pixel_format & PIXEL_FORMAT_MONO10) != !(fb->inverted)) {
		cfb_invert(fb);
		err = display_write(dev, 0, 0, &desc, fb->buf);
		cfb_invert(fb);
		return err;
	}

	return display_write(dev, 0, 0, &desc, fb->buf);
#else
	return 0;
#endif
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
	struct char_framebuffer *fb = &char_fb;
	struct display_capabilities cfg;

	display_get_capabilities(dev, &cfg);

	STRUCT_SECTION_COUNT(cfb_font, &fb->numof_fonts);

	LOG_DBG("number of fonts %d", fb->numof_fonts);

	fb->x_res = cfg.x_resolution;
	fb->y_res = cfg.y_resolution;
	fb->ppt = 8U;
	fb->pixel_format = cfg.current_pixel_format;
	fb->screen_info = cfg.screen_info;
	fb->kerning = 0;
	fb->inverted = false;

	fb->fonts = TYPE_SECTION_START(cfb_font);
	fb->font_idx = 0U;

#ifdef CONFIG_CHARACTER_FRAMEBUFFER_USE_OFFSCREEN_BUFFER
	fb->size = fb->x_res * fb->y_res / fb->ppt;
	fb->buf = k_malloc(fb->size);
	if (!fb->buf) {
		return -ENOMEM;
	}

	memset(fb->buf, 0, fb->size);
#else
	fb->size = CONFIG_CHARACTER_FRAMEBUFFER_TRANSFER_BUFFER_SIZE;
	if (fb->size == 0) {
		fb->size = fb->x_res;
	}

	fb->buf = k_malloc(fb->size);
	if (!fb->buf) {
		return -ENOMEM;
	}
	memset(fb->buf, 0, fb->size);
#endif

	return 0;
}

/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/display/cfb.h>
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
	CLEAR_COMMANDS,
	CLEAR_DISPLAY,
};

/**
 * Framebuffer and rendering info
 *
 * @param screen Screen size info
 * @param pos Position of framebuffer in screen
 * @param fb Framebuffer pointer
 */
struct fb_info {
	struct cfb_position screen;
	struct cfb_position pos;
	struct cfb_framebuffer *fb;
};

/**
 * A collection of state variable references
 *
 * @param font_idx Pointer to font index state variable
 * @param kerning Pointer to kerning state variable
 * @param fg_color Pointer to forerground color state variable
 * @param bg_color Pointer to background color state variable
 */
struct state_info {
	uint8_t *font_idx;
	int8_t *kerning;
	uint32_t *fg_color;
	uint32_t *bg_color;
};

/**
 * @param node current node pointer
 * @param param A param element in the buffer. This exists only in the case that the node is
 *              pointing to the SET_COMMAND_BUFFER command.
 */
struct command_iterator {
	sys_snode_t *node;
	struct cfb_command_param *param;
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
	       (c - fptr->first_char) * (fptr->width * ((fptr->height + 7U) / 8U));
}

static inline uint8_t get_glyph_byte(const uint8_t *glyph_ptr, const struct cfb_font *fptr,
				     uint8_t x, uint8_t y)
{
	if (!glyph_ptr) {
		return 0;
	}

	if (fptr->caps & CFB_FONT_MONO_VPACKED) {
		return glyph_ptr[x * ((fptr->height + 7U) / 8U) + y];
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

static inline uint16_t fb_info_top(const struct fb_info *info)
{
	return info->pos.y;
}

static inline uint16_t fb_info_left(const struct fb_info *info)
{
	return info->pos.x;
}

static inline uint16_t fb_info_bottom(const struct fb_info *info)
{
	return info->pos.y + info->fb->height;
}

static inline uint16_t fb_info_right(const struct fb_info *info)
{
	return info->pos.x + info->fb->width;
}

static bool check_font_in_rect(int16_t x, int16_t y, const struct cfb_font *fptr,
			       const struct fb_info *info)
{
	if (x + fptr->width <= fb_info_left(info)) {
		return false;
	}

	if (y + fptr->height <= fb_info_top(info)) {
		return false;
	}

	if (x > fb_info_right(info)) {
		return false;
	}

	if (y > fb_info_bottom(info)) {
		return false;
	}

	return true;
}

static inline uint8_t fb_bpp(const struct cfb_framebuffer *fb)
{
	return fb->bpp_ppt > 0 ? fb->bpp_ppt : 1;
}

static inline uint8_t fb_ppt(const struct cfb_framebuffer *fb)
{
	return fb->bpp_ppt < 0 ? -fb->bpp_ppt : 1;
}

static inline bool fb_info_is_tiled(const struct fb_info *info)
{
	if ((info->fb->pixel_format == PIXEL_FORMAT_MONO01) ||
	    (info->fb->pixel_format == PIXEL_FORMAT_MONO10)) {
		return true;
	}

	return false;
}

static inline uint8_t bytes_per_pixel(enum display_pixel_format pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 4;
	case PIXEL_FORMAT_RGB_888:
		return 3;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_BGR_565:
		return 2;
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
	default:
		return 1;
	}

	return 0;
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
		*a = 0;
		if (color) {
			*r = *g = *b = 0xFF;
		} else {
			*r = *g = *b = 0x0;
		}
		return;
	case PIXEL_FORMAT_MONO10:
		*a = 0;
		if (color) {
			*r = *g = *b = 0x0;
		} else {
			*r = *g = *b = 0xFF;
		}
		return;
	case PIXEL_FORMAT_RGB_888:
		*a = 0;
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
		*a = 0;
		*r = (color >> 8) & 0xF8;
		*g = (color >> 3) & 0xFC;
		*b = (color << 3) & 0xF8;
		return;
	case PIXEL_FORMAT_BGR_565:
		*a = 0;
		*b = (color >> 8) & 0xF8;
		*g = (color >> 3) & 0xFC;
		*r = (color << 3) & 0xF8;
		return;
	default:
		return;
	}
}

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

static void fill_fb(const struct fb_info *info, uint32_t color, size_t bpp)
{
	if (bpp == 1) {
		memset(info->fb->buf, color, info->fb->size);
	} else if (bpp == 2) {
		uint16_t *buf16 = (uint16_t *)info->fb->buf;

		for (size_t i = 0; i < info->fb->size / 2; i++) {
			buf16[i] = color;
		}
	} else if (bpp == 3) {
		for (size_t i = 0; i < info->fb->size; i++) {
			info->fb->buf[i] = color >> (8 * (2 - (i % 3)));
		}
	} else if (bpp == 4) {
		uint32_t *buf32 = (uint32_t *)info->fb->buf;

		for (size_t i = 0; i < info->fb->size / 4; i++) {
			buf32[i] = color;
		}
	}
}

/*
 * Draw the monochrome character in the monochrome tiled framebuffer,
 * a byte is interpreted as 8 pixels ordered vertically among each other.
 */
static uint8_t draw_char_vtmono(const struct fb_info *info, char c, int16_t x, int16_t y,
				const struct cfb_font *fptr, bool draw_bg, uint32_t fg_color)
{
	const struct cfb_framebuffer *fb = info->fb;
	const bool font_is_msbfirst = (fptr->caps & CFB_FONT_MSB_FIRST) != 0;
	const bool need_reverse =
		((fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0) != font_is_msbfirst;
	const uint8_t *glyph_ptr = get_glyph_ptr(fptr, c);
	uint8_t draw_width;
	uint8_t draw_height;

	if (!check_font_in_rect(x, y, fptr, info)) {
		return fptr->width;
	}

	if (fptr->width + x > fb_info_right(info)) {
		draw_width = fb_info_right(info) - x;
	} else {
		draw_width = fptr->width;
	}

	if (fptr->height + y > fb_info_bottom(info)) {
		draw_height = fb_info_bottom(info) - y;
	} else {
		draw_height = fptr->height;
	}

	for (size_t g_x = 0; g_x < draw_width; g_x++) {
		const int16_t fb_x = x + g_x - info->pos.x;

		if (fb_x < 0 || info->screen.x <= fb_x) {
			continue;
		}

		for (size_t g_y = 0; g_y < draw_height;) {
			/*
			 * Process glyph rendering in the y direction
			 * by separating per 8-line boundaries.
			 */

			const int16_t fb_y = y + g_y - info->pos.y;
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
				byte = ((byte << 8) | next_byte) >> offset;

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
				g_y += 8 - offset;
			} else if ((fptr->height - g_y) >= 8) {
				g_y += 8;
			} else {
				g_y += bottom_lines;
			}
		}
	}

	return fptr->width;
}

static uint8_t draw_char_color(const struct fb_info *info, char c, int16_t x, int16_t y,
			       const struct cfb_font *fptr, bool draw_bg, uint32_t fg_color,
			       uint32_t bg_color)
{
	const struct cfb_framebuffer *fb = info->fb;
	const uint8_t *glyph_ptr = get_glyph_ptr(fptr, c);
	uint8_t draw_width;
	uint8_t draw_height;

	if (!check_font_in_rect(x, y, fptr, info)) {
		return fptr->width;
	}

	if (fptr->width + x > fb_info_right(info)) {
		draw_width = fb_info_right(info) - x;
	} else {
		draw_width = fptr->width;
	}

	if (fptr->height + y > fb_info_bottom(info)) {
		draw_height = fb_info_bottom(info) - y;
	} else {
		draw_height = fptr->height;
	}

	for (size_t g_x = 0; g_x < draw_width; g_x++) {
		const int16_t fb_x = x + g_x - info->pos.x;

		if ((fb_x < 0) || (info->screen.x <= fb_x)) {
			continue;
		}

		for (size_t g_y = 0; g_y < draw_height; g_y++) {
			const size_t b = g_y % 8;
			const uint8_t pos = (fptr->caps & CFB_FONT_MSB_FIRST) ? BIT(7 - b) : BIT(b);
			const int16_t fb_y = y + g_y - info->pos.y;
			const size_t fb_index = (fb_y * fb->width + fb_x) * fb_bpp(fb);
			const uint8_t byte = get_glyph_byte(glyph_ptr, fptr, g_x, g_y / 8);

			if ((fb_y < 0) || (fb->height <= fb_y)) {
				continue;
			}

			if (byte & pos) {
				set_color_bytes(&fb->buf[fb_index], fb_bpp(fb), fg_color);
			} else {
				if (draw_bg) {
					set_color_bytes(&fb->buf[fb_index], fb_bpp(fb), bg_color);
				}
			}
		}
	}

	return fptr->width;
}

static inline void draw_point(const struct fb_info *info, int16_t x, int16_t y, uint32_t fg_color)
{
	const struct cfb_framebuffer *fb = info->fb;
	const bool need_reverse = (fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0;
	const int16_t x_off = x - info->pos.x;
	const int16_t y_off = y - info->pos.y;

	if (x < fb_info_left(info) || x >= fb_info_right(info)) {
		return;
	}

	if (y < fb_info_top(info) || y >= fb_info_bottom(info)) {
		return;
	}

	if (fb_info_is_tiled(info)) {
		const size_t index = (y_off / 8) * fb->width;
		uint8_t m = BIT(y_off % 8);

		if (need_reverse) {
			m = byte_reverse(m);
		}

		if (fg_color) {
			fb->buf[index + x_off] |= m;
		} else {
			fb->buf[index + x_off] &= ~m;
		}
	} else {
		const size_t index = (y_off * fb->width + x_off) * fb_bpp(fb);

		set_color_bytes(&fb->buf[index], fb_bpp(fb), fg_color);
	}
}

static void draw_line(const struct fb_info *info, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
		      uint32_t fg_color)
{
	int16_t sx = x0 < x1 ? 1 : -1;
	int16_t sy = y0 < y1 ? 1 : -1;
	int16_t dx = sx > 0 ? x1 - x0 : x0 - x1;
	int16_t dy = sy > 0 ? y0 - y1 : y1 - y0;
	int16_t err = dx + dy;
	int16_t e2;

	while (true) {
		draw_point(info, x0, y0, fg_color);

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

static void draw_text(const struct fb_info *info, const char *const str, int16_t x, int16_t y,
		      bool print, const struct cfb_font *fptr, int8_t kerning, uint32_t fg_color,
		      uint32_t bg_color)
{
	for (size_t i = 0; i < strlen(str); i++) {
		if ((x + fptr->width > info->screen.x) && print) {
			x = 0U;
			y += fptr->height;
		}

		if (fb_info_is_tiled(info)) {
			x += draw_char_vtmono(info, str[i], x, y, fptr, print, fg_color);
		} else {
			x += draw_char_color(info, str[i], x, y, fptr, print, fg_color, bg_color);
		}

		x += kerning;
	}
}

static void invert_area(const struct fb_info *info, int16_t x, int16_t y, uint16_t width,
			uint16_t height)
{
	const struct cfb_framebuffer *fb = info->fb;
	const bool need_reverse = (fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) != 0;

	if ((x + width) < fb_info_left(info) || x >= fb_info_right(info)) {
		return;
	}

	if ((y + height) < fb_info_top(info) || y >= fb_info_bottom(info)) {
		return;
	}

	x -= info->pos.x;
	y -= info->pos.y;

	if (x < 0) {
		width += x;
		x = 0;
	}

	if (y < 0) {
		height += y;
		y = 0;
	}

	if (width > fb->width - x) {
		width = fb->width - x;
	}

	if (height > fb->height - y) {
		height = fb->height - y;
	}

	for (size_t i = x; i < x + width; i++) {
		for (size_t j = y; j < y + height; j++) {
			if (fb_info_is_tiled(info)) {
				/*
				 * Process inversion in the y direction
				 * by separating per 8-line boundaries.
				 */

				const size_t index = (j / 8) * fb->width + i;
				const uint8_t remains = y + height - j;

				/*
				 * Make mask to prevent overwriting the drawing contents that on
				 * between the start line or end line and the 8-line boundary.
				 */
				if ((j % 8) > 0) {
					uint8_t m = BIT_MASK(j % 8);
					uint8_t b = fb->buf[index];

					/*
					 * Generate mask for remaining lines in case of
					 * drawing within 8 lines from the start line
					 */
					if (remains < 8) {
						m |= BIT_MASK(8 - (j % 8) + remains)
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
				const size_t index = (j * fb->width + i) * fb_bpp(fb);

				if (index > fb->size) {
					continue;
				}

				if (fb_bpp(fb) == 1) {
					fb->buf[index] = ~fb->buf[index];
				} else if (fb_bpp(fb) == 2) {
					*(uint16_t *)&fb->buf[index] =
						~(*(uint16_t *)&fb->buf[index]);
				} else if (fb_bpp(fb) == 3) {
					fb->buf[index] = ~fb->buf[index];
					fb->buf[index + 1] = ~fb->buf[index + 1];
					fb->buf[index + 2] = ~fb->buf[index + 2];
				} else if (fb_bpp(fb) == 4) {
					*(uint32_t *)&fb->buf[index] =
						~(*(uint32_t *)&fb->buf[index]);
				}
			}
		}
	}
}

static inline bool iterator_is_last(struct command_iterator ite)
{
	return !ite.node;
}

static inline struct cfb_command_param *iterator_get_param(struct command_iterator ite)
{
	struct cfb_command *pcmd = CONTAINER_OF(ite.node, struct cfb_command, node);

	return ite.param ? ite.param : &pcmd->param;
}

/**
 * This function scans linked lists and buffers as well as samely.
 * If there is a SET_COMMAND_BUFFER in the command list,
 * look inside it.
 *
 * @param ite current iterator
 * @param iterator that is pointing next node
 */
static struct command_iterator next_iterator(struct command_iterator ite)
{
	struct cfb_commandbuffer *cmdbuf;
	struct command_iterator next;
	uint8_t *buf_ptr;

	struct cfb_command *cmd = CONTAINER_OF(ite.node, struct cfb_command, node);

	if (cmd->param.op != CFB_OP_SET_COMMAND_BUFFER) {
		next.node = sys_slist_peek_next(ite.node);
		next.param = NULL;

		return next;
	}

	cmdbuf = cmd->param.cmd_buffer.cmdbuf;

	if (ite.param) {
		buf_ptr = (uint8_t *)(ite.param + 1);

		if (ite.param->op == CFB_OP_DRAW_TEXT || ite.param->op == CFB_OP_PRINT) {
			buf_ptr += strlen(buf_ptr) + 1;
		}
	} else {
		buf_ptr = cmdbuf->buf;
	}

	next.node = ite.node;
	next.param = (void *)buf_ptr;

	if (buf_ptr < cmdbuf->buf || cmdbuf->size <= (buf_ptr - cmdbuf->buf) ||
	    next.param->op == CFB_OP_NOP || next.param->op == CFB_OP_TERMINATE) {
		next.node = sys_slist_peek_next(ite.node);
		next.param = NULL;
	}

	return next;
}

/**
 * Executes a list of commands.
 * Called by cfb_finalize and cfb_clear.
 * When called from cfb_clear, only apply the settings without executing the drawing command.
 *
 * @param fb_info Framebuffer and rendering info
 * @param x The start x position of rendering rect
 * @param y The start y position of rendering rect
 * @param w The width of rendering rect
 * @param h The height of rendering rect
 * @param state Pointer to state variable structure
 * @param ite A command list iterator
 * @param mode Execution mode
 *
 * @return negative value if failed, otherwise 0
 */
static int process_command_list(struct fb_info *info, uint16_t x, uint16_t y, uint16_t w,
				uint16_t h, struct state_info *state, struct command_iterator ite,
				enum command_process_mode mode)
{
	struct cfb_framebuffer *fb = info->fb;
	const uint16_t draw_width = x + w;
	const uint16_t draw_height = y + h;
	const uint16_t start_x = x;
	const struct command_iterator ite_start = ite;
	int err = 0;

	if (fb->size < (w * fb_bpp(fb))) {
		w = DIV_ROUND_UP(w * fb_bpp(fb), DIV_ROUND_UP(w * fb_bpp(fb), fb->size)) /
		    fb_bpp(fb);
		h = fb_ppt(fb);
	} else {
		h = MIN((fb->size / (w * fb_bpp(fb))) * fb_ppt(fb), h);
	}

	for (; y < draw_height; y += h) {
		for (x = start_x; x < draw_width; x += w) {
			for (ite = ite_start; !iterator_is_last(ite); ite = next_iterator(ite)) {
				const struct cfb_command_param *param = iterator_get_param(ite);

				info->pos.x = x;
				info->pos.y = y;
				fb->width = w;
				fb->height = h;

				/* Process only state change commands if clear. */
				if (mode != FINALIZE) {
					if ((param->op != CFB_OP_FILL) &&
					    (param->op != CFB_OP_SET_FONT) &&
					    (param->op != CFB_OP_SET_KERNING) &&
					    (param->op != CFB_OP_SET_FG_COLOR) &&
					    (param->op != CFB_OP_SET_BG_COLOR) &&
					    (param->op != CFB_OP_SWAP_FG_BG_COLOR)) {
						continue;
					}
				}

				if (param->op == CFB_OP_FILL) {
					fill_fb(info, *state->bg_color,
						bytes_per_pixel(fb->pixel_format));
				} else if (param->op == CFB_OP_DRAW_POINT) {
					draw_point(info, param->draw_figure.start.x,
						   param->draw_figure.start.y, *state->fg_color);
				} else if (param->op == CFB_OP_DRAW_LINE) {
					draw_line(info, param->draw_figure.start.x,
						  param->draw_figure.start.y,
						  param->draw_figure.end.x,
						  param->draw_figure.end.y, *state->fg_color);
				} else if (param->op == CFB_OP_DRAW_RECT) {
					draw_line(info, param->draw_figure.start.x,
						  param->draw_figure.start.y,
						  param->draw_figure.end.x,
						  param->draw_figure.start.y, *state->fg_color);
					draw_line(info, param->draw_figure.end.x,
						  param->draw_figure.start.y,
						  param->draw_figure.end.x,
						  param->draw_figure.end.y, *state->fg_color);
					draw_line(info, param->draw_figure.end.x,
						  param->draw_figure.end.y,
						  param->draw_figure.start.x,
						  param->draw_figure.end.y, *state->fg_color);
					draw_line(info, param->draw_figure.start.x,
						  param->draw_figure.end.y,
						  param->draw_figure.start.x,
						  param->draw_figure.start.y, *state->fg_color);
				} else if (param->op == CFB_OP_DRAW_TEXT) {
					draw_text(info, (char *)(param + 1), param->draw_text.pos.x,
						  param->draw_text.pos.y, false,
						  font_get(*state->font_idx), *state->kerning,
						  *state->fg_color, *state->bg_color);
				} else if (param->op == CFB_OP_PRINT) {
					draw_text(info, (char *)(param + 1), param->draw_text.pos.x,
						  param->draw_text.pos.y, true,
						  font_get(*state->font_idx), *state->kerning,
						  *state->fg_color, *state->bg_color);
				} else if (param->op == CFB_OP_DRAW_TEXT_REF) {
					draw_text(info, param->draw_text.str,
						  param->draw_text.pos.x, param->draw_text.pos.y,
						  false, font_get(*state->font_idx),
						  *state->kerning, *state->fg_color,
						  *state->bg_color);
				} else if (param->op == CFB_OP_PRINT_REF) {
					draw_text(info, param->draw_text.str,
						  param->draw_text.pos.x, param->draw_text.pos.y,
						  true, font_get(*state->font_idx), *state->kerning,
						  *state->fg_color, *state->bg_color);
				} else if (param->op == CFB_OP_INVERT_AREA) {
					invert_area(info, param->invert_area.x,
						    param->invert_area.y, param->invert_area.w,
						    param->invert_area.h);
				} else if (param->op == CFB_OP_SWAP_FG_BG_COLOR) {
					const uint32_t tmp_fg = *state->fg_color;
					*state->fg_color = *state->bg_color;
					*state->bg_color = tmp_fg;
				} else if (param->op == CFB_OP_SET_FONT) {
					*state->font_idx = param->set_font.font_idx;
				} else if (param->op == CFB_OP_SET_KERNING) {
					*state->kerning = param->set_kerning.kerning;
				} else if (param->op == CFB_OP_SET_FG_COLOR) {
					*state->fg_color = rgba_to_color(
						fb->pixel_format, param->set_color.red,
						param->set_color.green, param->set_color.blue,
						param->set_color.alpha);
				} else if (param->op == CFB_OP_SET_BG_COLOR) {
					*state->bg_color = rgba_to_color(
						fb->pixel_format, param->set_color.red,
						param->set_color.green, param->set_color.blue,
						param->set_color.alpha);
				} else if (param->op == CFB_OP_SET_COMMAND_BUFFER) {
					/* nop */
				} else {
					break;
				}
			}

			/* Don't update display on clear-command case */
			if (mode != CLEAR_COMMANDS) {
				err = fb->transfer_buffer(fb, x, y, w, h);
			}

			if (err) {
				return err;
			}
		}
	}

	return 0;
}

/**
 * Transferring buffer contents to display
 *
 * @param fb framebuffer pointer that is linked to display
 * @param x X position
 * @param y Y position
 * @param w width
 * @param h height
 *
 * @return negative value if failed, otherwise 0
 */
static int display_transfer_buffer(const struct cfb_framebuffer *fb, int16_t x, int16_t y,
				   uint16_t w, uint16_t h)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	struct display_buffer_descriptor desc;
	int err = 0;

	if (x >= disp->x_res || y >= disp->y_res) {
		return 0;
	}

	if (y + h < 0) {
		return 0;
	}

	if (y < 0) {
		h += y;
		y = 0;
	}

	desc.buf_size = fb->size;
	desc.width = w;
	desc.height = h;
	desc.pitch = w;

	if (desc.height + y >= disp->y_res) {
		desc.height = disp->y_res - y;
	}

	if (desc.width + x >= disp->x_res) {
		desc.width = disp->x_res - x;
		desc.pitch = disp->x_res - x;
	}

	err = display_write(disp->dev, x, y, &desc, fb->buf);
	if (err) {
		LOG_DBG("display_write(%d %d %d %d) size: %d: err=%d", x, y, w, h, fb->size, err);
	}

	return err;
}

/*
 * Set up an initialization command to be executed every time partial frame buffer drawing.
 *
 * @param disp display structure
 */
static void display_append_init_commands(struct cfb_display *disp)
{
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 0;

	disp->init_cmds[CFB_INIT_CMD_SET_FONT].param.op = CFB_OP_SET_FONT;
	disp->init_cmds[CFB_INIT_CMD_SET_FONT].param.set_font.font_idx = disp->font_idx;

	disp->init_cmds[CFB_INIT_CMD_SET_KERNING].param.op = CFB_OP_SET_KERNING;
	disp->init_cmds[CFB_INIT_CMD_SET_KERNING].param.set_kerning.kerning = disp->kerning;

	color_to_rgba(disp->fb.pixel_format, disp->fg_color, &r, &g, &b, &a);
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.op = CFB_OP_SET_FG_COLOR;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.red = r;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.green = g;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.blue = b;
	disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].param.set_color.alpha = a;

	color_to_rgba(disp->fb.pixel_format, disp->bg_color, &r, &g, &b, &a);
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.op = CFB_OP_SET_BG_COLOR;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.red = r;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.green = g;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.blue = b;
	disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].param.set_color.alpha = a;

	disp->init_cmds[CFB_INIT_CMD_FILL].param.op = CFB_OP_FILL;
	disp->init_cmds[CFB_INIT_CMD_SET_COMMAND_BUFFER].param.op = CFB_OP_SET_COMMAND_BUFFER;
	disp->init_cmds[CFB_INIT_CMD_SET_COMMAND_BUFFER].param.cmd_buffer.cmdbuf = &disp->cmdbuf;

	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_FONT].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_KERNING].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_FG_COLOR].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_BG_COLOR].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_FILL].node);
	sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_SET_COMMAND_BUFFER].node);
}

/**
 * Run finalizing process.
 *
 * This function is called via function-pointer in the cfb_framebuffer
 * struct on calling cfb_finalize.
 *
 * @param fb framebuffer pointer that is linked to display
 * @param x X position
 * @param y Y position
 * @param w width
 * @param h height
 *
 * @return negative value if failed, otherwise 0
 */
static int display_finalize(const struct cfb_framebuffer *fb, int16_t x, int16_t y, uint16_t width,
			    uint16_t height)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	struct state_info state = {
		.font_idx = &disp->font_idx,
		.kerning = &disp->kerning,
		.fg_color = &disp->fg_color,
		.bg_color = &disp->bg_color,
	};
	struct fb_info info = {
		.screen = {disp->x_res, disp->y_res},
		.fb = (struct cfb_framebuffer *)fb,
	};
	struct command_iterator ite;

	x = x < 0 ? 0 : x;
	y = y < 0 ? 0 : y;
	width = width >= disp->x_res - x ? disp->x_res - x : width;
	height = height >= disp->y_res - y ? disp->y_res - y : height;

	ite.node = sys_slist_peek_head(&disp->cmd_list);
	ite.param = NULL;

	return process_command_list(&info, x, y, width, height, &state, ite, FINALIZE);
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
static int display_clear(const struct cfb_framebuffer *fb, bool clear_display)
{
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	struct state_info state = {
		.font_idx = &disp->font_idx,
		.kerning = &disp->kerning,
		.fg_color = &disp->fg_color,
		.bg_color = &disp->bg_color,
	};
	struct fb_info info = {
		.screen = {disp->x_res, disp->y_res},
		.fb = (struct cfb_framebuffer *)fb,
	};
	struct command_iterator ite;
	int err;

	if (clear_display) {
		/* if clear processing, filling with the background color is done last. */
		sys_slist_remove(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_FILL - 1].node,
				 &disp->init_cmds[CFB_INIT_CMD_FILL].node);
		sys_slist_append(&disp->cmd_list, &disp->init_cmds[CFB_INIT_CMD_FILL].node);
	}

	ite.node = sys_slist_peek_head(&disp->cmd_list);
	ite.param = NULL;

	err = process_command_list(&info, 0, 0, disp->x_res, disp->y_res, &state, ite,
				   clear_display ? CLEAR_DISPLAY : CLEAR_COMMANDS);

	/* reset command list */
	disp->cmdbuf.pos = 0;
	memset(disp->cmdbuf.buf, 0, disp->cmdbuf.size);
	sys_slist_init(&disp->cmd_list);
	display_append_init_commands(disp);

	return err;
}

/**
 * Append a command to buffer or list
 *
 * This function is called via function-pointer in the cfb_framebuffer
 * struct on calling cfb_append_command and rendering functions.
 *
 * When adding commands to the list, you must manage the added commands so
 * that they are not discarded until they are discarded with cfb_clear.
 *
 * The CFB_SET_COMMAND_BUFFER command must be placed at the end of the command
 * list to add to the buffer.
 * This is normally enabled if you specify a valid command buffer in
 * cfb_display_init.
 *
 * @param fb Framebuffer pointer that is linked to display
 * @param cmd A command to append buffer or list
 * @param append_buffer Store in buffer if true
 *
 * @retval -ENOBUFS Not enough buffers remain to append the command.
 * @retval 0 Succeedsed
 */
static int display_append_command(const struct cfb_framebuffer *fb, struct cfb_command *cmd,
				  bool append_buffer)
{
	const bool store_text =
		(cmd->param.op == CFB_OP_DRAW_TEXT || cmd->param.op == CFB_OP_PRINT);
	const size_t str_len = store_text ? strlen(cmd->param.draw_text.str) + 1 : 0;
	const size_t allocate_size = sizeof(struct cfb_command) + str_len + 1;
	struct cfb_display *disp = CONTAINER_OF(fb, struct cfb_display, fb);
	sys_snode_t *tail = sys_slist_peek_tail(&disp->cmd_list);
	struct cfb_command *tail_cmd = CONTAINER_OF(tail, struct cfb_command, node);

	if (!append_buffer) {
		sys_slist_append(&disp->cmd_list, &cmd->node);
	} else if (tail_cmd->param.op == CFB_OP_SET_COMMAND_BUFFER) {
		struct cfb_commandbuffer *cmdbuf = tail_cmd->param.cmd_buffer.cmdbuf;

		if (cmdbuf->size < cmdbuf->pos + allocate_size) {
			return -ENOBUFS;
		}

		memcpy(&cmdbuf->buf[cmdbuf->pos], &cmd->param, sizeof(struct cfb_command_param));

		cmdbuf->pos += sizeof(struct cfb_command_param);

		if (store_text) {
			memcpy(&cmdbuf->buf[cmdbuf->pos], cmd->param.draw_text.str, str_len);
			cmdbuf->pos += str_len;
			cmdbuf->buf[cmdbuf->pos] = '\0';
		}
	} else {
		return -ENOBUFS;
	}

	return 0;
}

int cfb_get_display_parameter(const struct cfb_display *disp, enum cfb_display_param param)
{
	switch (param) {
	case CFB_DISPLAY_HEIGH:
		return disp->y_res;
	case CFB_DISPLAY_WIDTH:
		return disp->x_res;
	case CFB_DISPLAY_PPT:
		return fb_ppt(&disp->fb);
	case CFB_DISPLAY_ROWS:
		if (disp->fb.screen_info & SCREEN_INFO_MONO_VTILED) {
			return disp->y_res / fb_ppt(&disp->fb);
		}
		return disp->y_res;
	case CFB_DISPLAY_COLS:
		if (disp->fb.screen_info & SCREEN_INFO_MONO_VTILED) {
			return disp->x_res;
		}
		return disp->x_res / fb_ppt(&disp->fb);
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

int cfb_display_init(struct cfb_display *disp, const struct device *dev, void *xferbuf,
		     size_t xferbuf_size, void *cmdbuf, size_t cmdbuf_size)
{
	struct display_capabilities cfg;

	display_get_capabilities(dev, &cfg);

	cfb_display_deinit(disp);

	if (xferbuf_size < bytes_per_pixel(cfg.current_pixel_format)) {
		return -EINVAL;
	}

	disp->dev = dev;
	disp->x_res = cfg.x_resolution;
	disp->y_res = cfg.y_resolution;
	disp->font_idx = 0U;
	disp->kerning = 0U;

	disp->fb.buf = xferbuf;
	disp->fb.size = xferbuf_size;
	disp->fb.screen_info = cfg.screen_info;
	disp->fb.width = cfg.x_resolution;
	disp->fb.height = cfg.y_resolution;
	disp->fb.pixel_format = cfg.current_pixel_format;
	if ((cfg.current_pixel_format == PIXEL_FORMAT_MONO01) ||
	    (cfg.current_pixel_format == PIXEL_FORMAT_MONO10)) {
		disp->fb.bpp_ppt = -8;
	} else {
		disp->fb.bpp_ppt = bytes_per_pixel(cfg.current_pixel_format);
	}

	disp->fb.finalize = display_finalize;
	disp->fb.clear = display_clear;
	disp->fb.append_command = display_append_command;
	disp->fb.transfer_buffer = display_transfer_buffer;

	if (cfg.current_pixel_format == PIXEL_FORMAT_MONO10) {
		disp->bg_color = 0xFFFFFFFFU;
		disp->fg_color = 0x0U;
	} else {
		disp->fg_color = 0xFFFFFFFFU;
		disp->bg_color = 0x0U;
	}

	disp->cmdbuf.buf = cmdbuf;
	disp->cmdbuf.size = cmdbuf_size;
	disp->cmdbuf.pos = 0U;

	memset(cmdbuf, 0, cmdbuf_size);

	sys_slist_init(&disp->cmd_list);

	display_append_init_commands(disp);

	return 0;
}

void cfb_display_deinit(struct cfb_display *disp)
{
	memset(disp, 0, sizeof(struct cfb_display));
}

struct cfb_display *cfb_display_alloc(const struct device *dev, size_t xferbuf_size,
				      size_t cmdbuf_size)
{
	struct display_capabilities cfg;
	struct cfb_display *disp;
	uint8_t *xferbuf;
	uint8_t *cmdbuf;
	size_t bufsize;
	int err;

	display_get_capabilities(dev, &cfg);
	if (xferbuf_size == 0) {
		xferbuf_size = cfg.x_resolution * cfg.y_resolution / 8U;
	}

	bufsize = ROUND_UP(sizeof(struct cfb_display), sizeof(void *)) +
		  ROUND_UP(xferbuf_size, sizeof(void *)) + ROUND_UP(cmdbuf_size, sizeof(void *));

	disp = k_malloc(bufsize);
	if (!disp) {
		return NULL;
	}

	cmdbuf = (uint8_t *)disp + ROUND_UP(sizeof(struct cfb_display), sizeof(void *));
	xferbuf = cmdbuf + ROUND_UP(cmdbuf_size, sizeof(void *));

	err = cfb_display_init(disp, dev, xferbuf, xferbuf_size, cmdbuf, cmdbuf_size);
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

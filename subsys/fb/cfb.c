/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <display/cfb.h>

#define LOG_LEVEL CONFIG_CFB_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(cfb);

extern const struct cfb_font __font_entry_start[];
extern const struct cfb_font __font_entry_end[];

struct char_framebuffer {
	/** Pointer to a buffer in RAM */
	u8_t *buf;

	/** Size of the framebuffer */
	u32_t size;

	/** Pointer to the font entry array */
	const struct cfb_font *fonts;

	/** Display pixel format */
	enum display_pixel_format pixel_format;

	/** Display screen info */
	enum display_screen_info screen_info;

	/** Resolution of a framebuffer in pixels in X direction */
	u16_t x_res;

	/** Resolution of a framebuffer in pixels in Y direction */
	u16_t y_res;

	/** Number of pixels per tile, typically 8 */
	u8_t ppt;

	/** Number of available fonts */
	u8_t numof_fonts;

	/** Current font index */
	u8_t font_idx;

	/** Font kerning */
	s8_t kerning;

	/** Invertedj*/
	bool inverted;
};

static struct char_framebuffer char_fb;

static inline u8_t *get_glyph_ptr(const struct cfb_font *fptr, char c)
{
	if (fptr->caps & CFB_FONT_MONO_VPACKED) {
		return (u8_t *)fptr->data +
		       (c - fptr->first_char) *
		       (fptr->width * fptr->height / 8U);
	}

	return NULL;
}

/*
 * Draw the monochrome character in the monochrome tiled framebuffer,
 * a byte is interpreted as 8 pixels ordered vertically among each other.
 */
static u8_t draw_char_vtmono(const struct char_framebuffer *fb,
			     char c, u16_t x, u16_t y)
{
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);
	u8_t *glyph_ptr;

	if (c < fptr->first_char || c > fptr->last_char) {
		c = ' ';
	}

	glyph_ptr = get_glyph_ptr(fptr, c);
	if (!glyph_ptr) {
		return 0;
	}

	for (size_t g_x = 0; g_x < fptr->width; g_x++) {
		u32_t y_segment = y / 8U;

		for (size_t g_y = 0; g_y < fptr->height / 8U; g_y++) {
			u32_t fb_y = (y_segment + g_y) * fb->x_res;

			if ((fb_y + x + g_x) >= fb->size) {
				return 0;
			}
			fb->buf[fb_y + x + g_x] =
				glyph_ptr[g_x * (fptr->height / 8U) + g_y];
		}

	}

	return fptr->width;
}

int cfb_print(struct device *dev, char *str, u16_t x, u16_t y)
{
	const struct char_framebuffer *fb = &char_fb;
	const struct cfb_font *fptr = &(fb->fonts[fb->font_idx]);

	if (!fb->fonts || !fb->buf) {
		return -1;
	}

	if (fptr->height % 8) {
		LOG_ERR("Wrong font size");
		return -1;
	}

	if ((fb->screen_info & SCREEN_INFO_MONO_VTILED) && !(y % 8)) {
		for (size_t i = 0; i < strlen(str); i++) {
			if (x + fptr->width > fb->x_res) {
				x = 0U;
				y += fptr->height;
			}
			x += fb->kerning + draw_char_vtmono(fb, str[i], x, y);
		}
		return 0;
	}

	LOG_ERR("Unsupported framebuffer configuration");
	return -1;
}

static int cfb_reverse_bytes(const struct char_framebuffer *fb)
{
	if (!(fb->screen_info & SCREEN_INFO_MONO_VTILED)) {
		LOG_ERR("Unsupported framebuffer configuration");
		return -1;
	}

	for (size_t i = 0; i < fb->x_res * fb->y_res / 8U; i++) {
		fb->buf[i] = (fb->buf[i] & 0xf0) >> 4 |
			      (fb->buf[i] & 0x0f) << 4;
		fb->buf[i] = (fb->buf[i] & 0xcc) >> 2 |
			      (fb->buf[i] & 0x33) << 2;
		fb->buf[i] = (fb->buf[i] & 0xaa) >> 1 |
			      (fb->buf[i] & 0x55) << 1;
	}

	return 0;
}

static int cfb_invert(const struct char_framebuffer *fb)
{
	for (size_t i = 0; i < fb->x_res * fb->y_res / 8U; i++) {
		fb->buf[i] = ~fb->buf[i];
	}

	return 0;
}

int cfb_framebuffer_clear(struct device *dev, bool clear_display)
{
	const struct char_framebuffer *fb = &char_fb;
	struct display_buffer_descriptor desc;

	if (!fb || !fb->buf) {
		return -1;
	}

	desc.buf_size = fb->size;
	desc.width = fb->x_res;
	desc.height = fb->y_res;
	desc.pitch = fb->x_res;
	memset(fb->buf, 0, fb->size);

	return 0;
}


int cfb_framebuffer_invert(struct device *dev)
{
	struct char_framebuffer *fb = &char_fb;

	if (!fb || !fb->buf) {
		return -1;
	}

	fb->inverted = !fb->inverted;

	return 0;
}

int cfb_framebuffer_finalize(struct device *dev)
{
	const struct display_driver_api *api = dev->driver_api;
	const struct char_framebuffer *fb = &char_fb;
	struct display_buffer_descriptor desc;

	if (!fb || !fb->buf) {
		return -1;
	}

	desc.buf_size = fb->size;
	desc.width = fb->x_res;
	desc.height = fb->y_res;
	desc.pitch = fb->x_res;

	if (!(fb->pixel_format & PIXEL_FORMAT_MONO10) != !(fb->inverted)) {
		cfb_invert(fb);
	}

	if (fb->screen_info & SCREEN_INFO_MONO_MSB_FIRST) {
		cfb_reverse_bytes(fb);
	}

	return api->write(dev, 0, 0, &desc, fb->buf);
}

int cfb_get_display_parameter(struct device *dev,
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

int cfb_framebuffer_set_font(struct device *dev, u8_t idx)
{
	struct char_framebuffer *fb = &char_fb;

	if (idx >= fb->numof_fonts) {
		return -1;
	}

	fb->font_idx = idx;

	return 0;
}

int cfb_get_font_size(struct device *dev, u8_t idx, u8_t *width, u8_t *height)
{
	const struct char_framebuffer *fb = &char_fb;

	if (idx >= fb->numof_fonts) {
		return -1;
	}

	if (width) {
		*width = __font_entry_start[idx].width;
	}

	if (height) {
		*height = __font_entry_start[idx].height;
	}

	return 0;
}

int cfb_get_numof_fonts(struct device *dev)
{
	const struct char_framebuffer *fb = &char_fb;

	return fb->numof_fonts;
}

int cfb_framebuffer_init(struct device *dev)
{
	const struct display_driver_api *api = dev->driver_api;
	struct char_framebuffer *fb = &char_fb;
	struct display_capabilities cfg;

	api->get_capabilities(dev, &cfg);

	fb->numof_fonts = __font_entry_end - __font_entry_start;
	LOG_DBG("number of fonts %d", fb->numof_fonts);
	if (!fb->numof_fonts) {
		return -1;
	}

	fb->x_res = cfg.x_resolution;
	fb->y_res = cfg.y_resolution;
	fb->ppt = 8U;
	fb->pixel_format = cfg.current_pixel_format;
	fb->screen_info = cfg.screen_info;
	fb->buf = NULL;
	fb->font_idx = 0U;
	fb->kerning = 0;
	fb->inverted = false;

	fb->fonts = __font_entry_start;
	fb->font_idx = 0U;

	fb->size = fb->x_res * fb->y_res / fb->ppt;
	fb->buf = k_malloc(fb->size);
	if (!fb->buf) {
		return -1;
	}

	memset(fb->buf, 0, fb->size);

	return 0;
}

/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Monochrome Character Framebuffer API
 */

#ifndef __CFB_H__
#define __CFB_H__

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Public Monochrome Character Framebuffer API
 * @defgroup monochrome_character_framebuffer Monochrome Character Framebuffer
 * @ingroup utilities
 * @{
 */

enum cfb_display_param {
	CFB_DISPLAY_HEIGH		= 0,
	CFB_DISPLAY_WIDTH,
	CFB_DISPLAY_PPT,
	CFB_DISPLAY_ROWS,
	CFB_DISPLAY_COLS,
};

enum cfb_font_caps {
	CFB_FONT_MONO_VPACKED		= BIT(0),
	CFB_FONT_MONO_HPACKED		= BIT(1),
	CFB_FONT_MSB_FIRST		= BIT(2),
};

struct cfb_font {
	const void *data;
	enum cfb_font_caps caps;
	uint8_t width;
	uint8_t height;
	uint8_t first_char;
	uint8_t last_char;
};

struct cfb_position {
	int16_t x;
	int16_t y;
};

/**
 * A framebuffer definition in CFB.
 *
 * This is a private definition.
 * Don't access directly to members of this structure.
 */
struct cfb_framebuffer {
	/** Pointer to a buffer in RAM */
	uint8_t *buf;

	/** Size of the framebuffer */
	uint32_t size;

	/** Display pixel format */
	enum display_pixel_format pixel_format;

	/** Display screen info */
	enum display_screen_info screen_info;

	/** Framebuffer width in pixels */
	uint16_t width;

	/** Framebuffer height in pixels */
	uint16_t height;

	/** Number of pixels per tile, typically 8 */
	uint8_t ppt;

};

/**
 * A display definition in CFB.
 *
 * This is a private definition.
 * Don't access directly to members of this structure.
 */
struct cfb_display {
	/** Framebuffer */
	struct cfb_framebuffer fb;

	/** Pointer to device */
	const struct device *dev;

	/** Resolution of a framebuffer in pixels in X direction */
	uint16_t x_res;

	/** Resolution of a framebuffer in pixels in Y direction */
	uint16_t y_res;

	/** Current font index */
	uint8_t font_idx;

	/** Current font kerning */
	int8_t kerning;

	/** Inverted */
	bool inverted;
};

/**
 * @brief Macro for creating a font entry.
 *
 * @param _name   Name of the font entry.
 * @param _width  Width of the font in pixels
 * @param _height Height of the font in pixels.
 * @param _caps   Font capabilities.
 * @param _data   Raw data of the font.
 * @param _fc     Character mapped to first font element.
 * @param _lc     Character mapped to last font element.
 */
#define FONT_ENTRY_DEFINE(_name, _width, _height, _caps, _data, _fc, _lc)      \
	static const STRUCT_SECTION_ITERABLE(cfb_font, _name) = {	       \
		.data = _data,						       \
		.caps = _caps,						       \
		.width = _width,					       \
		.height = _height,					       \
		.first_char = _fc,					       \
		.last_char = _lc,					       \
	}

/**
 * @brief Print a string into the framebuffer.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_print(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y);

/**
 * @brief Print a string into the framebuffer.
 * For compare to cfb_print, cfb_draw_text accept non tile-aligned coords
 * and not line wrapping.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_text(struct cfb_framebuffer *fb, const char *const str, int16_t x, int16_t y);

/**
 * @brief Draw a point.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param pos position of the point
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_point(struct cfb_framebuffer *fb, const struct cfb_position *pos);

/**
 * @brief Draw a line.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param start start position of the line
 * @param end end position of the line
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_line(struct cfb_framebuffer *fb, const struct cfb_position *start,
		  const struct cfb_position *end);

/**
 * @brief Draw a rectangle.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param start Top-Left position of the rectangle
 * @param end Bottom-Right position of the rectangle
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_rect(struct cfb_framebuffer *fb, const struct cfb_position *start,
		  const struct cfb_position *end);

/**
 * @brief Clear framebuffer.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param clear_display Clear the display as well
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_clear(struct cfb_framebuffer *fb, bool clear_display);

/**
 * @brief Invert Pixels.
 *
 * @param fb Pointer to framebuffer to rendering
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_invert(struct cfb_framebuffer *fb);

/**
 * @brief Invert Pixels in selected area.
 *
 * @param fb Pointer to framebuffer to rendering
 * @param x Position in X direction of the beginning of area
 * @param y Position in Y direction of the beginning of area
 * @param width Width of area in pixels
 * @param height Height of area in pixels
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_invert_area(struct cfb_framebuffer *fb, int16_t x, int16_t y, uint16_t width,
		    uint16_t height);

/**
 * @brief Finalize framebuffer and write it to display RAM,
 * invert or reorder pixels if necessary.
 *
 * @param fb Pointer to framebuffer to rendering
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_finalize(struct cfb_framebuffer *fb);

/**
 * @brief Get display parameter.
 *
 * @param disp Pointer to display instance
 * @param cfb_display_param One of the display parameters
 *
 * @return Display parameter value
 */
int cfb_get_display_parameter(const struct cfb_display *disp, enum cfb_display_param);

/**
 * @brief Set font.
 *
 * @param fb Pointer to framebuffer instance
 * @param idx Font index
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_set_font(struct cfb_framebuffer *fb, uint8_t idx);

/**
 * @brief Set font kerning (spacing between individual letters).
 *
 * @param fb Pointer to framebuffer instance
 * @param kerning Font kerning
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_set_kerning(struct cfb_framebuffer *fb, int8_t kerning);

/**
 * @brief Get font size.
 *
 * Get width and height of font that is specified by idx.
 *
 * @param idx Font index
 * @param width Pointers to the variable where the font width will be stored.
 * @param height Pointers to the variable where the font height will be stored.
 *
 * @retval 0 on success
 * @retval -EINVAL If you specify idx for a font that does not exist
 */
int cfb_get_font_size(uint8_t idx, uint8_t *width, uint8_t *height);

/**
 * @brief Get number of fonts.
 *
 * @return number of fonts
 */
int cfb_get_numof_fonts(void);

/**
 * @brief Initialize framebuffer.
 *
 * @param disp Pointer to display instance to initialize
 * @param dev Pointer to device that use to displaying
 *
 * @return 0 on success
 */
int cfb_display_init(struct cfb_display *disp, const struct device *dev);

/**
 * @brief Deinitialize Character Framebuffer.
 *
 * @param disp Pointer to display instance to deinitialize
 */
void cfb_display_deinit(struct cfb_display *disp);

/**
 * @brief Get framebuffer subordinate to cfb_display.
 *
 * @param disp Pointer to display instance to retrieve framebuffer.
 */
struct cfb_framebuffer *cfb_display_get_framebuffer(struct cfb_display *disp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __CFB_H__ */

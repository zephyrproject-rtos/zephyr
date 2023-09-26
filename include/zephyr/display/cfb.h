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
	uint16_t x;
	uint16_t y;
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
 * @param dev Pointer to device structure for driver instance
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_print(const struct device *dev, const char *const str, uint16_t x, uint16_t y);

/**
 * @brief Print a string into the framebuffer.
 * For compare to cfb_print, cfb_draw_text accept non tile-aligned coords
 * and not line wrapping.
 *
 * @param dev Pointer to device structure for driver instance
 * @param str String to print
 * @param x Position in X direction of the beginning of the string
 * @param y Position in Y direction of the beginning of the string
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_text(const struct device *dev, const char *const str, int16_t x, int16_t y);

/**
 * @brief Draw a point.
 *
 * @param dev Pointer to device structure for driver instance
 * @param pos position of the point
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_point(const struct device *dev, const struct cfb_position *pos);

/**
 * @brief Draw a line.
 *
 * @param dev Pointer to device structure for driver instance
 * @param start start position of the line
 * @param end end position of the line
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_line(const struct device *dev, const struct cfb_position *start,
		  const struct cfb_position *end);

/**
 * @brief Draw a rectangle.
 *
 * @param dev Pointer to device structure for driver instance
 * @param start Top-Left position of the rectangle
 * @param end Bottom-Right position of the rectangle
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_draw_rect(const struct device *dev, const struct cfb_position *start,
		  const struct cfb_position *end);

/**
 * @brief Clear framebuffer.
 *
 * @param dev Pointer to device structure for driver instance
 * @param clear_display Clear the display as well
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_framebuffer_clear(const struct device *dev, bool clear_display);

/**
 * @brief Invert Pixels.
 *
 * @param dev Pointer to device structure for driver instance
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_framebuffer_invert(const struct device *dev);

/**
 * @brief Invert Pixels in selected area.
 *
 * @param dev Pointer to device structure for driver instance
 * @param x Position in X direction of the beginning of area
 * @param y Position in Y direction of the beginning of area
 * @param width Width of area in pixels
 * @param height Height of area in pixels
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_invert_area(const struct device *dev, uint16_t x, uint16_t y,
		    uint16_t width, uint16_t height);

/**
 * @brief Finalize framebuffer and write it to display RAM,
 * invert or reorder pixels if necessary.
 *
 * @param dev Pointer to device structure for driver instance
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_framebuffer_finalize(const struct device *dev);

/**
 * @brief Get display parameter.
 *
 * @param dev Pointer to device structure for driver instance
 * @param cfb_display_param One of the display parameters
 *
 * @return Display parameter value
 */
int cfb_get_display_parameter(const struct device *dev,
			      enum cfb_display_param);

/**
 * @brief Set font.
 *
 * @param dev Pointer to device structure for driver instance
 * @param idx Font index
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_framebuffer_set_font(const struct device *dev, uint8_t idx);

/**
 * @brief Set font kerning (spacing between individual letters).
 *
 * @param dev Pointer to device structure for driver instance
 * @param kerning Font kerning
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_set_kerning(const struct device *dev, int8_t kerning);

/**
 * @brief Get font size.
 *
 * @param dev Pointer to device structure for driver instance
 * @param idx Font index
 * @param width Pointers to the variable where the font width will be stored.
 * @param height Pointers to the variable where the font height will be stored.
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_get_font_size(const struct device *dev, uint8_t idx, uint8_t *width,
		      uint8_t *height);

/**
 * @brief Get number of fonts.
 *
 * @param dev Pointer to device structure for driver instance
 *
 * @return number of fonts
 */
int cfb_get_numof_fonts(const struct device *dev);

/**
 * @brief Initialize Character Framebuffer.
 *
 * @param dev Pointer to device structure for driver instance
 *
 * @return 0 on success, negative value otherwise
 */
int cfb_framebuffer_init(const struct device *dev);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __CFB_H__ */

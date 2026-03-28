/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX display list commands
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_DL_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_DL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT8xx display list commands
 * @defgroup ft8xx_dl FT8xx display list
 * @ingroup ft8xx_interface
 * @{
 */

/** Rectangular pixel arrays, in various color formats */
#define FT8XX_BITMAPS      1U
/** Anti-aliased points, point radius is 1-256 pixels */
#define FT8XX_POINTS       2U
/**
 * Anti-aliased lines, with width from 0 to 4095 1/16th of pixel units.
 * (width is from center of the line to boundary)
 */
#define FT8XX_LINES        3U
/** Anti-aliased lines, connected head-to-tail */
#define FT8XX_LINE_STRIP   4U
/** Edge strips for right */
#define FT8XX_EDGE_STRIP_R 5U
/** Edge strips for left */
#define FT8XX_EDGE_STRIP_L 6U
/** Edge strips for above */
#define FT8XX_EDGE_STRIP_A 7U
/** Edge strips for below */
#define FT8XX_EDGE_STRIP_B 8U
/**
 * Round-cornered rectangles, curvature of the corners can be adjusted using
 * FT8XX_LINE_WIDTH
 */
#define FT8XX_RECTS        9U

/**
 * @brief Begin drawing a graphics primitive
 *
 * The valid primitives are defined as:
 * - @ref FT8XX_BITMAPS
 * - @ref FT8XX_POINTS
 * - @ref FT8XX_LINES
 * - @ref FT8XX_LINE_STRIP
 * - @ref FT8XX_EDGE_STRIP_R
 * - @ref FT8XX_EDGE_STRIP_L
 * - @ref FT8XX_EDGE_STRIP_A
 * - @ref FT8XX_EDGE_STRIP_B
 * - @ref FT8XX_RECTS
 *
 * The primitive to be drawn is selected by the @ref FT8XX_BEGIN command. Once
 * the primitive is selected, it will be valid till the new primitive is
 * selected by the @ref FT8XX_BEGIN command.
 *
 * @note The primitive drawing operation will not be performed until
 *       @ref FT8XX_VERTEX2II or @ref FT8XX_VERTEX2F is executed.
 *
 * @param prim Graphics primitive
 */
#define FT8XX_BEGIN(prim) (0x1f000000 | ((prim) & 0x0f))

/**
 * @brief Clear buffers to preset values
 *
 * Setting @p c to true will clear the color buffer of the FT8xx to the preset
 * value. Setting this bit to false will maintain the color buffer of the FT8xx
 * with an unchanged value. The preset value is defined in command
 * @ref FT8XX_CLEAR_COLOR_RGB for RGB channel and FT8XX_CLEAR_COLOR_A for alpha
 * channel.
 *
 * Setting @p s to true will clear the stencil buffer of the FT8xx to the preset
 * value. Setting this bit to false will maintain the stencil buffer of the
 * FT8xx with an unchanged value. The preset value is defined in command
 * FT8XX_CLEAR_STENCIL.
 *
 * Setting @p t to true will clear the tag buffer of the FT8xx to the preset
 * value. Setting this bit to false will maintain the tag buffer of the FT8xx
 * with an unchanged value. The preset value is defined in command
 * FT8XX_CLEAR_TAG.
 *
 * @param c Clear color buffer
 * @param s Clear stencil buffer
 * @param t Clear tag buffer
 */
#define FT8XX_CLEAR(c, s, t) (0x26000000 | \
		((c) ? 0x04 : 0) | ((s) ? 0x02 : 0) | ((t) ? 0x01 : 0))

/**
 * @brief Specify clear values for red, green and blue channels
 *
 * Sets the color values used by a following @ref FT8XX_CLEAR.
 *
 * @param red Red value used when the color buffer is cleared
 * @param green Green value used when the color buffer is cleared
 * @param blue Blue value used when the color buffer is cleared
 */
#define FT8XX_CLEAR_COLOR_RGB(red, green, blue) (0x02000000 | \
		(((uint32_t)(red) & 0xff) << 16) | \
		(((uint32_t)(green) & 0xff) << 8) | \
		((uint32_t)(blue) & 0xff))

/**
 * @brief Set the current color red, green and blue
 *
 * Sets red, green and blue values of the FT8xx color buffer which will be
 * applied to the following draw operation.
 *
 * @param red Red value for the current color
 * @param green Green value for the current color
 * @param blue Blue value for the current color
 */
#define FT8XX_COLOR_RGB(red, green, blue) (0x04000000 | \
		(((uint32_t)(red) & 0xff) << 16) | \
		(((uint32_t)(green) & 0xff) << 8) | \
		((uint32_t)(blue) & 0xff))

/**
 * @brief End the display list
 *
 * FT8xx will ignore all the commands following this command.
 */
#define FT8XX_DISPLAY() 0

/**
 * @brief End drawing a graphics primitive
 *
 * It is recommended to have an @ref FT8XX_END for each @ref FT8XX_BEGIN.
 * Whereas advanced users can avoid the usage of @ref FT8XX_END in order to
 * save extra graphics instructions in the display list RAM.
 */
#define FT8XX_END() 0x21000000

/**
 * @brief Specify the width of lines to be drawn with primitive @ref FT8XX_LINES
 *
 * Sets the width of drawn lines. The width is the distance from the center of
 * the line to the outermost drawn pixel, in units of 1/16 pixel. The valid
 * range is from 16 to 4095 in terms of 1/16th pixel units.
 *
 * @note The @ref FT8XX_LINE_WIDTH command will affect the @ref FT8XX_LINES,
 *       @ref FT8XX_LINE_STRIP, @ref FT8XX_RECTS, @ref FT8XX_EDGE_STRIP_A /B/R/L
 *       primitives.
 *
 * @param width Line width in 1/16 pixel
 */
#define FT8XX_LINE_WIDTH(width) (0x0e000000 | ((uint32_t)(width) & 0xfff))

/**
 * @brief Attach the tag value for the following graphics objects.
 *
 * The initial value of the tag buffer of the FT8xx is specified by command
 * FT8XX_CLEAR_TAG and taken effect by command @ref FT8XX_CLEAR. @ref FT8XX_TAG
 * command can specify the value of the tag buffer of the FT8xx that applies to
 * the graphics objects when they are drawn on the screen. This tag value will
 * be assigned to all the following objects, unless the FT8XX_TAG_MASK command
 * is used to disable it.  Once the following graphics objects are drawn, they
 * are attached with the tag value successfully. When the graphics objects
 * attached with the tag value are touched, the register
 * @ref FT800_REG_TOUCH_TAG or @ref FT810_REG_TOUCH_TAG will be updated with the
 * tag value of the graphics object being touched. If there is no @ref FT8XX_TAG
 * commands in one display list, all the graphics objects rendered by the
 * display list will report tag value as 255 in @ref FT800_REG_TOUCH_TAG or
 * @ref FT810_REG_TOUCH_TAG when they were touched.
 *
 * @param s Tag value 1-255
 */
#define FT8XX_TAG(s) (0x03000000 | (uint8_t)(s))

/**
 * @brief Start the operation of graphics primitives at the specified coordinate
 *
 * The range of coordinates is from -16384 to +16383 in terms of 1/16th pixel
 * units.  The negative x coordinate value means the coordinate in the left
 * virtual screen from (0, 0), while the negative y coordinate value means the
 * coordinate in the upper virtual screen from (0, 0). If drawing on the
 * negative coordinate position, the drawing operation will not be visible.
 *
 * @param x Signed x-coordinate in 1/16 pixel precision
 * @param y Signed y-coordinate in 1/16 pixel precision
 */
#define FT8XX_VERTEX2F(x, y) (0x40000000 | \
		(((int32_t)(x) & 0x7fff) << 15) | \
		((int32_t)(y) & 0x7fff))

/**
 * @brief Start the operation of graphics primitive at the specified coordinates
 *
 * The valid range of @p handle is from 0 to 31. From 16 to 31 the bitmap handle
 * is dedicated to the FT8xx built-in font.
 *
 * Cell number is the index of bitmap with same bitmap layout and format.
 * For example, for handle 31, the cell 65 means the character "A" in the
 * largest built in font.
 *
 * @param x x-coordinate in pixels, from 0 to 511
 * @param y y-coordinate in pixels, from 0 to 511
 * @param handle Bitmap handle
 * @param cell Cell number
 */
#define FT8XX_VERTEX2II(x, y, handle, cell) (0x80000000 | \
		(((uint32_t)(x) & 0x01ff) << 21) | \
		(((uint32_t)(y) & 0x01ff) << 12) | \
		(((uint32_t)(handle) & 0x1f) << 7) | \
		((uint32_t)(cell) & 0x7f))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_DL_H_ */

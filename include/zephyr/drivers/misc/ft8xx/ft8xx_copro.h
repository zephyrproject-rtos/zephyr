/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX coprocessor functions
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COPRO_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COPRO_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT8xx co-processor engine functions
 * @defgroup ft8xx_copro FT8xx co-processor
 * @ingroup ft8xx_interface
 * @{
 */

/** Co-processor widget is drawn in 3D effect */
#define FT8XX_OPT_3D        0
/** Co-processor option to decode the JPEG image to RGB565 format */
#define FT8XX_OPT_RGB565    0
/** Co-processor option to decode the JPEG image to L8 format, i.e., monochrome */
#define FT8XX_OPT_MONO      1
/** No display list commands generated for bitmap decoded from JPEG image */
#define FT8XX_OPT_NODL      2
/** Co-processor widget is drawn without 3D effect */
#define FT8XX_OPT_FLAT      256
/** The number is treated as 32 bit signed integer */
#define FT8XX_OPT_SIGNED    256
/** Co-processor widget centers horizontally */
#define FT8XX_OPT_CENTERX   512
/** Co-processor widget centers vertically */
#define FT8XX_OPT_CENTERY   1024
/** Co-processor widget centers horizontally and vertically */
#define FT8XX_OPT_CENTER    1536
/** The label on the Coprocessor widget is right justified */
#define FT8XX_OPT_RIGHTX    2048
/** Co-processor widget has no background drawn */
#define FT8XX_OPT_NOBACK    4096
/** Co-processor clock widget is drawn without hour ticks.
 *  Gauge widget is drawn without major and minor ticks.
 */
#define FT8XX_OPT_NOTICKS   8192
/** Co-processor clock widget is drawn without hour and minutes hands,
 * only seconds hand is drawn
 */
#define FT8XX_OPT_NOHM      16384
/** The Co-processor gauge has no pointer */
#define FT8XX_OPT_NOPOINTER 16384
/** Co-processor clock widget is drawn without seconds hand */
#define FT8XX_OPT_NOSECS    32768
/** Co-processor clock widget is drawn without hour, minutes and seconds hands */
#define FT8XX_OPT_NOHANDS   49152

/**
 * @brief Execute a display list command by co-processor engine
 *
 * @param dev Device structure
 * @param cmd Display list command to execute
 */
void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd);

/**
 * @brief Start a new display list
 *
 * @param dev Device structure
 */
void ft8xx_copro_cmd_dlstart(const struct device *dev);

/**
 * @brief Swap the current display list
 *
 * @param dev Device structure
 */
void ft8xx_copro_cmd_swap(const struct device *dev);

/**
 * @brief Set the foreground color
 *
 * New foreground color, as a 24-bit RGB number. Red is the most significant 8
 * bits, blue is the least. So 0xff0000 is bright red. Foreground color is
 * applicable for things that the user can move such as handles and buttons
 * ("affordances").
 *
 * @param dev Device structure
 * @param color Color to set
 */
void ft8xx_copro_cmd_fgcolor(const struct device *dev, uint32_t color);

/**
 * @brief Set the background color
 *
 * New background color, as a 24-bit RGB number. Red is the most significant 8
 * bits, blue is the least. So 0xff0000 is bright red.
 * Background color is applicable for things that the user cannot move. Example
 * behind gauges and sliders etc.
 *
 * @param dev Device structure
 * @param color Color to set
 */
void ft8xx_copro_cmd_bgcolor(const struct device *dev, uint32_t color);

/**
 * @brief Draw a slider
 *
 * If width is greater than height, the scroll bar is drawn horizontally.
 * If height is greater than width, the scroll bar is drawn vertically.
 *
 * By default the slider is drawn with a 3D effect. @ref FT8XX_OPT_FLAT removes
 * the 3D effect.
 *
 * @param dev Device structure
 * @param x x-coordinate of slider top-left, in pixels
 * @param y y-coordinate of slider top-left, in pixels
 * @param width Width of slider, in pixels
 * @param height Height of slider, in pixels
 * @param options Options to apply
 * @param val Displayed value of slider, between 0 and range inclusive
 * @param range Maximum value
 */
void ft8xx_copro_cmd_slider(const struct device *dev,
			    int16_t x,
			    int16_t y,
			    int16_t width,
			    int16_t height,
			    uint16_t options,
			    uint16_t val,
			    uint16_t range);

/**
 * @brief Draw a toggle switch
 *
 * By default the toggle is drawn with a 3D effect and the value option is zero.
 * @ref FT8XX_OPT_FLAT removes the 3D effect and its value is 256.
 *
 * In @p string, a character value of 255 (it can be written as @c \\xff )
 * separates the off and on labels.
 *
 * @param dev Device structure
 * @param x x-coordinate of top-left of toggle, in pixels
 * @param y y-coordinate of top-left of toggle, in pixels
 * @param width Width of toggle, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param state State of the toggle: 0 is off, 65535 is on
 * @param string String label for toggle
 */
void ft8xx_copro_cmd_toggle(const struct device *dev,
			    int16_t x,
			    int16_t y,
			    int16_t width,
			    int16_t font,
			    uint16_t options,
			    uint16_t state,
			    const char *string);

/**
 * @brief Track touches for a graphics object
 *
 * This command will enable co-processor engine to track the touch on the
 * particular graphics object with one valid tag value assigned. Then,
 * co-processor engine will update the REG_TRACKER periodically with the frame
 * rate of LCD display panel.
 *
 * Co-processor engine tracks the graphics object in rotary tracker mode and
 * linear tracker mode:
 *
 * - Rotary tracker mode
 *   Track the angle between the touching point and the center of graphics
 *   object specified by @p tag value. The value is in units of 1/65536 of a
 *   circle. 0 means that the angle is straight down, 0x4000 left, 0x8000 up,
 *   and 0xC000 right from the center.
 * - Linear tracker mode
 *   If @p width is greater than @p height, track the relative distance of
 *   touching point to the width of graphics object specified by @p tag value.
 *   If @p width is not greater than @p height, track the relative distance of
 *   touching point to the height of graphics object specified by @p tag value.
 *   The value is in units of 1/65536 of the width or height of graphics object.
 *   The distance of touching point refers to the distance from the top left
 *   pixel of graphics object to the coordinate of touching point.
 *
 * For linear tracker functionality, @p x represents x-coordinate of track area
 * top-left and @p y represents y-coordinate of track area top-left. Both
 * parameters are in pixels.
 *
 * For rotary tracker functionality, @p x represents x-coordinate of track area
 * center and @p y represents y-coordinate of track area center. Both
 * parameters are in pixels.
 *
 * @note A @p width and @p height of (1,1) means that the tracker is rotary,
 *       and reports an angle value in REG_TRACKER. A @p width and @p height
 *       of (0,0) disables the track functionality of co-processor engine.
 *
 * @param dev Device structure
 * @param x x-coordinate
 * @param y y-coordinate
 * @param width Width of track area, in pixels.
 * @param height Height of track area, in pixels.
 * @param tag Tag of the graphics object to be tracked, 1-255
 */
void ft8xx_copro_cmd_track(const struct device *dev,
			   int16_t x,
			   int16_t y,
			   int16_t width,
			   int16_t height,
			   int16_t tag);

/**
 * @brief Draw text
 *
 * By default (@p x, @p y) is the top-left pixel of the text and the value of
 * @p options is zero. @ref FT8XX_OPT_CENTERX centers the text horizontally,
 * @ref FT8XX_OPT_CENTERY centers it vertically. @ref FT8XX_OPT_CENTER centers
 * the text in both directions. @ref FT8XX_OPT_RIGHTX right-justifies the text,
 * so that the @p x is the rightmost pixel.
 *
 * @param dev Device structure
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param string Character string to display, terminated with a null character
 */
void ft8xx_copro_cmd_text(const struct device *dev,
			  int16_t x,
			  int16_t y,
			  int16_t font,
			  uint16_t options,
			  const char *string);

/**
 * @brief Draw a decimal number
 *
 * By default (@p x, @p y) is the top-left pixel of the text.
 * @ref FT8XX_OPT_CENTERX centers the text horizontally, @ref FT8XX_OPT_CENTERY
 * centers it vertically. @ref FT8XX_OPT_CENTER centers the text in both
 * directions. @ref FT8XX_OPT_RIGHTX right-justifies the text, so that the @p x
 * is the rightmost pixel. By default the number is displayed with no leading
 * zeroes, but if a width 1-9 is specified in the @p options, then the number
 * is padded if necessary with leading zeroes so that it has the given width.
 * If @ref FT8XX_OPT_SIGNED is given, the number is treated as signed, and
 * prefixed by a minus sign if negative.
 *
 * @param dev Device structure
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param number The number to display.
 */
void ft8xx_copro_cmd_number(const struct device *dev,
			    int16_t x,
			    int16_t y,
			    int16_t font,
			    uint16_t options,
			    int32_t number);

/**
 * @brief Execute the touch screen calibration routine
 *
 * The calibration procedure collects three touches from the touch screen, then
 * computes and loads an appropriate matrix into REG_TOUCH_TRANSFORM_A-F. To
 * use it, create a display list and then use CMD_CALIBRATE. The co-processor
 * engine overlays the touch targets on the current display list, gathers the
 * calibration input and updates REG_TOUCH_TRANSFORM_A-F.
 *
 * @param dev Device structure
 * @param result Calibration result, written with 0 on failure of calibration
 */
void ft8xx_copro_cmd_calibrate(const struct device *dev, uint32_t *result);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COPRO_H_ */

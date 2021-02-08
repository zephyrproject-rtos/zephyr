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
 * @param cmd Display list command to execute
 */
void ft8xx_copro_cmd(uint32_t cmd);

/**
 * @brief Start a new display list
 */
void ft8xx_copro_cmd_dlstart(void);

/**
 * @brief Swap the current display list
 */
void ft8xx_copro_cmd_swap(void);

/**
 * @brief Draw text
 *
 * By default (@p x, @p y) is the top-left pixel of the text and the value of
 * @p options is zero. @ref FT8XX_OPT_CENTERX centers the text horizontally,
 * @ref FT8XX_OPT_CENTERY centers it vertically. @ref FT8XX_OPT_CENTER centers
 * the text in both directions. @ref FT8XX_OPT_RIGHTX right-justifies the text,
 * so that the @p x is the rightmost pixel.
 *
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param s Character string to display, terminated with a null character
 */
void ft8xx_copro_cmd_text(int16_t x,
			   int16_t y,
			   int16_t font,
			   uint16_t options,
			   const char *s);

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
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param n The number to display.
 */
void ft8xx_copro_cmd_number(int16_t x,
			     int16_t y,
			     int16_t font,
			     uint16_t options,
			     int32_t n);

/**
 * @brief Execute the touch screen calibration routine
 *
 * The calibration procedure collects three touches from the touch screen, then
 * computes and loads an appropriate matrix into REG_TOUCH_TRANSFORM_A-F. To
 * use it, create a display list and then use CMD_CALIBRATE. The co-processor
 * engine overlays the touch targets on the current display list, gathers the
 * calibration input and updates REG_TOUCH_TRANSFORM_A-F.
 *
 * @param result Calibration result, written with 0 on failure of calibration
 */
void ft8xx_copro_cmd_calibrate(uint32_t *result);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COPRO_H_ */

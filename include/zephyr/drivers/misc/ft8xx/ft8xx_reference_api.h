/*
 * Copyright (c) 2021 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX reference API
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_REFERENCE_API_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_REFERENCE_API_H_

#include <stdint.h>

#include <zephyr/drivers/misc/ft8xx/ft8xx_copro.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_dl.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_memory.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT8xx reference API
 *
 * API defined according to FT800 Programmers Guide API reference definition.
 *
 * @note Function names defined in this header may easily collide with names
 *       provided by other modules. Include this header with caution. If naming
 *       conflict occurs instead of including this header, use @c ft8xx_
 *       prefixed names.
 *
 * @defgroup ft8xx_reference_api FT8xx reference API
 * @ingroup ft8xx_interface
 * @{
 */

/**
 * @brief Write 1 byte (8 bits) to FT8xx memory
 *
 * @param address Memory address to write to
 * @param data Byte to write
 */
static inline void wr8(uint32_t address, uint8_t data)
{
	ft8xx_wr8(address, data);
}

/**
 * @brief Write 2 bytes (16 bits) to FT8xx memory
 *
 * @param address Memory address to write to
 * @param data Value to write
 */
static inline void wr16(uint32_t address, uint16_t data)
{
	ft8xx_wr16(address, data);
}

/**
 * @brief Write 4 bytes (32 bits) to FT8xx memory
 *
 * @param address Memory address to write to
 * @param data Value to write
 */
static inline void wr32(uint32_t address, uint32_t data)
{
	ft8xx_wr32(address, data);
}

/**
 * @brief Read 1 byte (8 bits) from FT8xx memory
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
static inline uint8_t rd8(uint32_t address)
{
	return ft8xx_rd8(address);
}

/**
 * @brief Read 2 bytes (16 bits) from FT8xx memory
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
static inline uint16_t rd16(uint32_t address)
{
	return ft8xx_rd16(address);
}

/**
 * @brief Read 4 bytes (32 bits) from FT8xx memory
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
static inline uint32_t rd32(uint32_t address)
{
	return ft8xx_rd32(address);
}


/** Co-processor widget is drawn in 3D effect */
#define OPT_3D        FT8XX_OPT_3D
/** Co-processor option to decode the JPEG image to RGB565 format */
#define OPT_RGB565    FT8XX_OPT_RGB565
/** Co-processor option to decode the JPEG image to L8 format, i.e., monochrome */
#define OPT_MONO      FT8XX_OPT_MONO
/** No display list commands generated for bitmap decoded from JPEG image */
#define OPT_NODL      FT8XX_OPT_NODL
/** Co-processor widget is drawn without 3D effect */
#define OPT_FLAT      FT8XX_OPT_FLAT
/** The number is treated as 32 bit signed integer */
#define OPT_SIGNED    FT8XX_OPT_SIGNED
/** Co-processor widget centers horizontally */
#define OPT_CENTERX   FT8XX_OPT_CENTERX
/** Co-processor widget centers vertically */
#define OPT_CENTERY   FT8XX_OPT_CENTERY
/** Co-processor widget centers horizontally and vertically */
#define OPT_CENTER    FT8XX_OPT_CENTER
/** The label on the Coprocessor widget is right justified */
#define OPT_RIGHTX    FT8XX_OPT_RIGHTX
/** Co-processor widget has no background drawn */
#define OPT_NOBACK    FT8XX_OPT_NOBACK
/** Co-processor clock widget is drawn without hour ticks.
 *  Gauge widget is drawn without major and minor ticks.
 */
#define OPT_NOTICKS   FT8XX_OPT_NOTICKS
/** Co-processor clock widget is drawn without hour and minutes hands,
 * only seconds hand is drawn
 */
#define OPT_NOHM      FT8XX_OPT_NOHM
/** The Co-processor gauge has no pointer */
#define OPT_NOPOINTER FT8XX_OPT_NOPOINTER
/** Co-processor clock widget is drawn without seconds hand */
#define OPT_NOSECS    FT8XX_OPT_NOSECS
/** Co-processor clock widget is drawn without hour, minutes and seconds hands */
#define OPT_NOHANDS   FT8XX_OPT_NOHANDS

/**
 * @brief Execute a display list command by co-processor engine
 *
 * @param command Display list command to execute
 */
static inline void cmd(uint32_t command)
{
	ft8xx_copro_cmd(command);
}

/**
 * @brief Start a new display list
 */
static inline void cmd_dlstart(void)
{
	ft8xx_copro_cmd_dlstart();
}

/**
 * @brief Swap the current display list
 */
static inline void cmd_swap(void)
{
	ft8xx_copro_cmd_swap();
}

/**
 * @brief Draw text
 *
 * By default (x,y) is the top-left pixel of the text and the value of
 * @p options is zero. OPT_CENTERX centers the text horizontally, OPT_CENTERY
 * centers it vertically. OPT_CENTER centers the text in both directions.
 * OPT_RIGHTX right-justifies the text, so that the x is the rightmost pixel.
 *
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param s Character string to display, terminated with a null character
 */
static inline void cmd_text(int16_t x,
			   int16_t y,
			   int16_t font,
			   uint16_t options,
			   const char *s)
{
	ft8xx_copro_cmd_text(x, y, font, options, s);
}

/**
 * @brief Draw a decimal number
 *
 * By default (@p x, @p y) is the top-left pixel of the text. OPT_CENTERX
 * centers the text horizontally, OPT_CENTERY centers it vertically. OPT_CENTER
 * centers the text in both directions. OPT_RIGHTX right-justifies the text, so
 * that the @p x is the rightmost pixel. By default the number is displayed
 * with no leading zeroes, but if a width 1-9 is specified in the @p options,
 * then the number is padded if necessary with leading zeroes so that it has
 * the given width. If OPT_SIGNED is given, the number is treated as signed,
 * and prefixed by a minus sign if negative.
 *
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param n The number to display.
 */
static inline void cmd_number(int16_t x,
			     int16_t y,
			     int16_t font,
			     uint16_t options,
			     int32_t n)
{
	ft8xx_copro_cmd_number(x, y, font, options, n);
}

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
static inline void cmd_calibrate(uint32_t *result)
{
	ft8xx_copro_cmd_calibrate(result);
}


/** Rectangular pixel arrays, in various color formats */
#define BITMAPS      FT8XX_BITMAPS
/** Anti-aliased points, point radius is 1-256 pixels */
#define POINTS       FT8XX_POINTS
/**
 * Anti-aliased lines, with width from 0 to 4095 1/16th of pixel units.
 * (width is from center of the line to boundary)
 */
#define LINES        FT8XX_LINES
/** Anti-aliased lines, connected head-to-tail */
#define LINE_STRIP   FT8XX_LINE_STRIP
/** Edge strips for right */
#define EDGE_STRIP_R FT8XX_EDGE_STRIP_R
/** Edge strips for left */
#define EDGE_STRIP_L FT8XX_EDGE_STRIP_L
/** Edge strips for above */
#define EDGE_STRIP_A FT8XX_EDGE_STRIP_A
/** Edge strips for below */
#define EDGE_STRIP_B FT8XX_EDGE_STRIP_B
/**
 * Round-cornered rectangles, curvature of the corners can be adjusted using
 * LINE_WIDTH
 */
#define RECTS        FT8XX_RECTS

/**
 * @brief Begin drawing a graphics primitive
 *
 * The valid primitives are defined as:
 * - @ref BITMAPS
 * - @ref POINTS
 * - @ref LINES
 * - @ref LINE_STRIP
 * - @ref EDGE_STRIP_R
 * - @ref EDGE_STRIP_L
 * - @ref EDGE_STRIP_A
 * - @ref EDGE_STRIP_B
 * - @ref RECTS
 *
 * The primitive to be drawn is selected by the @ref BEGIN command. Once the
 * primitive is selected, it will be valid till the new primitive is selected
 * by the @ref BEGIN command.
 *
 * @note The primitive drawing operation will not be performed until
 *       @ref VERTEX2II or @ref VERTEX2F is executed.
 *
 * @param prim Graphics primitive
 */
#define BEGIN(prim) FT8XX_BEGIN(prim)

/**
 * @brief Clear buffers to preset values
 *
 * Setting @p c to true will clear the color buffer of the FT8xx to the preset
 * value. Setting this bit to false will maintain the color buffer of the FT8xx
 * with an unchanged value. The preset value is defined in command
 * @ref CLEAR_COLOR_RGB for RGB channel and CLEAR_COLOR_A for alpha channel.
 *
 * Setting @p s to true will clear the stencil buffer of the FT8xx to the preset
 * value. Setting this bit to false will maintain the stencil buffer of the
 * FT8xx with an unchanged value. The preset value is defined in command
 * CLEAR_STENCIL.
 *
 * Setting @p t to true will clear the tag buffer of the FT8xx to the preset
 * value. Setting this bit to false will maintain the tag buffer of the FT8xx
 * with an unchanged value. The preset value is defined in command CLEAR_TAG.
 *
 * @param c Clear color buffer
 * @param s Clear stencil buffer
 * @param t Clear tag buffer
 */
#define CLEAR(c, s, t) FT8XX_CLEAR(c, s, t)

/**
 * @brief Specify clear values for red, green and blue channels
 *
 * Sets the color values used by a following @ref CLEAR.
 *
 * @param red Red value used when the color buffer is cleared
 * @param green Green value used when the color buffer is cleared
 * @param blue Blue value used when the color buffer is cleared
 */
#define CLEAR_COLOR_RGB(red, green, blue) FT8XX_CLEAR_COLOR_RGB(red, green, blue)

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
#define COLOR_RGB(red, green, blue) FT8XX_COLOR_RGB(red, green, blue)

/**
 * @brief End the display list
 *
 * FT8xx will ignore all the commands following this command.
 */
#define DISPLAY() FT8XX_DISPLAY()

/**
 * @brief End drawing a graphics primitive
 *
 * It is recommended to have an @ref END for each @ref BEGIN. Whereas advanced
 * users can avoid the usage of @ref END in order to save extra graphics
 * instructions in the display list RAM.
 */
#define END() FT8XX_END()

/**
 * @brief Specify the width of lines to be drawn with primitive @ref LINES
 *
 * Sets the width of drawn lines. The width is the distance from the center of
 * the line to the outermost drawn pixel, in units of 1/16 pixel. The valid
 * range is from 16 to 4095 in terms of 1/16th pixel units.
 *
 * @note The @ref LINE_WIDTH command will affect the @ref LINES,
 *       @ref LINE_STRIP, @ref RECTS, @ref EDGE_STRIP_A /B/R/L primitives.
 *
 * @param width Line width in 1/16 pixel
 */
#define LINE_WIDTH(width) FT8XX_LINE_WIDTH(width)

/**
 * @brief Attach the tag value for the following graphics objects.
 *
 * The initial value of the tag buffer of the FT8xx is specified by command
 * CLEAR_TAG and taken effect by command @ref CLEAR. @ref TAG command can
 * specify the value of the tag buffer of the FT8xx that applies to the graphics
 * objects when they are drawn on the screen. This @ref TAG value will be
 * assigned to all the following objects, unless the TAG_MASK command is used to
 * disable it.  Once the following graphics objects are drawn, they are attached
 * with the tag value successfully. When the graphics objects attached with the
 * tag value are touched, the register @ref REG_TOUCH_TAG will be updated with
 * the tag value of the graphics object being touched. If there is no @ref TAG
 * commands in one display list, all the graphics objects rendered by the
 * display list will report tag value as 255 in @ref REG_TOUCH_TAG when they
 * were touched.
 *
 * @param s Tag value 1-255
 */
#define TAG(s) FT8XX_TAG(s)

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
#define VERTEX2F(x, y) FT8XX_VERTEX2F(x, y)

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
#define VERTEX2II(x, y, handle, cell) FT8XX_VERTEX2II(x, y, handle, cell)


#if defined(CONFIG_FT800)
/** Main parts of FT800 memory map */
enum ft8xx_memory_map_t {
	RAM_G         = FT800_RAM_G,
	ROM_CHIPID    = FT800_ROM_CHIPID,
	ROM_FONT      = FT800_ROM_FONT,
	ROM_FONT_ADDR = FT800_ROM_FONT_ADDR,
	RAM_DL        = FT800_RAM_DL,
	RAM_PAL       = FT800_RAM_PAL,
	REG_          = FT800_REG_,
	RAM_CMD       = FT800_RAM_CMD
};
#else /* Definition of FT810 memory map */
/** Main parts of FT810 memory map */
enum ft8xx_memory_map_t {
	RAM_G         = FT810_RAM_G,
	RAM_DL        = FT810_RAM_DL,
	REG_          = FT810_REG_,
	RAM_CMD       = FT810_RAM_CMD
};
#endif

#if defined(CONFIG_FT800)
/** FT800 register addresses */
enum ft8xx_register_address_t {
	REG_ID         = FT800_REG_ID,
	REG_FRAMES     = FT800_REG_FRAMES,
	REG_CLOCK      = FT800_REG_CLOCK,
	REG_FREQUENCY  = FT800_REG_FREQUENCY,
	REG_RENDERMODE = FT800_REG_RENDERMODE,
	REG_SNAPY      = FT800_REG_SNAPY,
	REG_SNAPSHOT   = FT800_REG_SNAPSHOT,
	REG_CPURESET   = FT800_REG_CPURESET,
	REG_TAP_CRC    = FT800_REG_TAP_CRC,
	REG_TAP_MASK   = FT800_REG_TAP_MASK,
	REG_HCYCLE     = FT800_REG_HCYCLE,
	REG_HOFFSET    = FT800_REG_HOFFSET,
	REG_HSIZE      = FT800_REG_HSIZE,
	REG_HSYNC0     = FT800_REG_HSYNC0,
	REG_HSYNC1     = FT800_REG_HSYNC1,
	REG_VCYCLE     = FT800_REG_VCYCLE,
	REG_VOFFSET    = FT800_REG_VOFFSET,
	REG_VSIZE      = FT800_REG_VSIZE,
	REG_VSYNC0     = FT800_REG_VSYNC0,
	REG_VSYNC1     = FT800_REG_VSYNC1,
	REG_DLSWAP     = FT800_REG_DLSWAP,
	REG_ROTATE     = FT800_REG_ROTATE,
	REG_OUTBITS    = FT800_REG_OUTBITS,
	REG_DITHER     = FT800_REG_DITHER,
	REG_SWIZZLE    = FT800_REG_SWIZZLE,
	REG_CSPREAD    = FT800_REG_CSPREAD,
	REG_PCLK_POL   = FT800_REG_PCLK_POL,
	REG_PCLK       = FT800_REG_PCLK,
	REG_TAG_X      = FT800_REG_TAG_X,
	REG_TAG_Y      = FT800_REG_TAG_Y,
	REG_TAG        = FT800_REG_TAG,
	REG_VOL_PB     = FT800_REG_VOL_PB,
	REG_VOL_SOUND  = FT800_REG_VOL_SOUND,
	REG_SOUND      = FT800_REG_SOUND,
	REG_PLAY       = FT800_REG_PLAY,
	REG_GPIO_DIR   = FT800_REG_GPIO_DIR,
	REG_GPIO       = FT800_REG_GPIO,

	REG_INT_FLAGS  = FT800_REG_INT_FLAGS,
	REG_INT_EN     = FT800_REG_INT_EN,
	REG_INT_MASK   = FT800_REG_INT_MASK,
	REG_PLAYBACK_START = FT800_REG_PLAYBACK_START,
	REG_PLAYBACK_LENGTH = FT800_REG_PLAYBACK_LENGTH,
	REG_PLAYBACK_READPTR = FT800_REG_PLAYBACK_READPTR,
	REG_PLAYBACK_FREQ = FT800_REG_PLAYBACK_FREQ,
	REG_PLAYBACK_FORMAT = FT800_REG_PLAYBACK_FORMAT,
	REG_PLAYBACK_LOOP = FT800_REG_PLAYBACK_LOOP,
	REG_PLAYBACK_PLAY = FT800_REG_PLAYBACK_PLAY,
	REG_PWM_HZ     = FT800_REG_PWM_HZ,
	REG_PWM_DUTY   = FT800_REG_PWM_DUTY,
	REG_MACRO_0    = FT800_REG_MACRO_0,
	REG_MACRO_1    = FT800_REG_MACRO_1,

	REG_CMD_READ   = FT800_REG_CMD_READ,
	REG_CMD_WRITE  = FT800_REG_CMD_WRITE,
	REG_CMD_DL     = FT800_REG_CMD_DL,
	REG_TOUCH_MODE = FT800_REG_TOUCH_MODE,
	REG_TOUCH_ADC_MODE = FT800_REG_TOUCH_ADC_MODE,
	REG_TOUCH_CHARGE = FT800_REG_TOUCH_CHARGE,
	REG_TOUCH_SETTLE = FT800_REG_TOUCH_SETTLE,
	REG_TOUCH_OVERSAMPLE = FT800_REG_TOUCH_OVERSAMPLE,
	REG_TOUCH_RZTHRESH = FT800_REG_TOUCH_RZTHRESH,
	REG_TOUCH_RAW_XY = FT800_REG_TOUCH_RAW_XY,
	REG_TOUCH_RZ   = FT800_REG_TOUCH_RZ,
	REG_TOUCH_SCREEN_XY = FT800_REG_TOUCH_SCREEN_XY,
	REG_TOUCH_TAG_XY = FT800_REG_TOUCH_TAG_XY,
	REG_TOUCH_TAG  = FT800_REG_TOUCH_TAG,
	REG_TOUCH_TRANSFORM_A = FT800_REG_TOUCH_TRANSFORM_A,
	REG_TOUCH_TRANSFORM_B = FT800_REG_TOUCH_TRANSFORM_B,
	REG_TOUCH_TRANSFORM_C = FT800_REG_TOUCH_TRANSFORM_C,
	REG_TOUCH_TRANSFORM_D = FT800_REG_TOUCH_TRANSFORM_D,
	REG_TOUCH_TRANSFORM_E = FT800_REG_TOUCH_TRANSFORM_E,
	REG_TOUCH_TRANSFORM_F = FT800_REG_TOUCH_TRANSFORM_F,

	REG_TOUCH_DIRECT_XY = FT800_REG_TOUCH_DIRECT_XY,
	REG_TOUCH_DIRECT_Z1Z2 = FT800_REG_TOUCH_DIRECT_Z1Z2,

	REG_TRACKER    = FT800_REG_TRACKER
};
#else /* Definition of FT810 registers */
/** FT810 register addresses */
enum ft8xx_register_address_t {
	REG_TRIM       = FT810_REG_TRIM,

	REG_ID         = FT810_REG_ID,
	REG_FRAMES     = FT810_REG_FRAMES,
	REG_CLOCK      = FT810_REG_CLOCK,
	REG_FREQUENCY  = FT810_REG_FREQUENCY,
	REG_RENDERMODE = FT810_REG_RENDERMODE,
	REG_SNAPY      = FT810_REG_SNAPY,
	REG_SNAPSHOT   = FT810_REG_SNAPSHOT,
	REG_CPURESET   = FT810_REG_CPURESET,
	REG_TAP_CRC    = FT810_REG_TAP_CRC,
	REG_TAP_MASK   = FT810_REG_TAP_MASK,
	REG_HCYCLE     = FT810_REG_HCYCLE,
	REG_HOFFSET    = FT810_REG_HOFFSET,
	REG_HSIZE      = FT810_REG_HSIZE,
	REG_HSYNC0     = FT810_REG_HSYNC0,
	REG_HSYNC1     = FT810_REG_HSYNC1,
	REG_VCYCLE     = FT810_REG_VCYCLE,
	REG_VOFFSET    = FT810_REG_VOFFSET,
	REG_VSIZE      = FT810_REG_VSIZE,
	REG_VSYNC0     = FT810_REG_VSYNC0,
	REG_VSYNC1     = FT810_REG_VSYNC1,
	REG_DLSWAP     = FT810_REG_DLSWAP,
	REG_ROTATE     = FT810_REG_ROTATE,
	REG_OUTBITS    = FT810_REG_OUTBITS,
	REG_DITHER     = FT810_REG_DITHER,
	REG_SWIZZLE    = FT810_REG_SWIZZLE,
	REG_CSPREAD    = FT810_REG_CSPREAD,
	REG_PCLK_POL   = FT810_REG_PCLK_POL,
	REG_PCLK       = FT810_REG_PCLK,
	REG_TAG_X      = FT810_REG_TAG_X,
	REG_TAG_Y      = FT810_REG_TAG_Y,
	REG_TAG        = FT810_REG_TAG,
	REG_VOL_PB     = FT810_REG_VOL_PB,
	REG_VOL_SOUND  = FT810_REG_VOL_SOUND,
	REG_SOUND      = FT810_REG_SOUND,
	REG_PLAY       = FT810_REG_PLAY,
	REG_GPIO_DIR   = FT810_REG_GPIO_DIR,
	REG_GPIO       = FT810_REG_GPIO,
	REG_GPIOX_DIR  = FT810_REG_GPIOX_DIR,
	REG_GPIOX      = FT810_REG_GPIOX,

	REG_INT_FLAGS  = FT810_REG_INT_FLAGS,
	REG_INT_EN     = FT810_REG_INT_EN,
	REG_INT_MASK   = FT810_REG_INT_MASK,
	REG_PLAYBACK_START = FT810_REG_PLAYBACK_START,
	REG_PLAYBACK_LENGTH = FT810_REG_PLAYBACK_LENGTH,
	REG_PLAYBACK_READPTR = FT810_REG_PLAYBACK_READPTR,
	REG_PLAYBACK_FREQ = FT810_REG_PLAYBACK_FREQ,
	REG_PLAYBACK_FORMAT = FT810_REG_PLAYBACK_FORMAT,
	REG_PLAYBACK_LOOP = FT810_REG_PLAYBACK_LOOP,
	REG_PLAYBACK_PLAY = FT810_REG_PLAYBACK_PLAY,
	REG_PWM_HZ     = FT810_REG_PWM_HZ,
	REG_PWM_DUTY   = FT810_REG_PWM_DUTY,

	REG_CMD_READ   = FT810_REG_CMD_READ,
	REG_CMD_WRITE  = FT810_REG_CMD_WRITE,
	REG_CMD_DL     = FT810_REG_CMD_DL,
	REG_TOUCH_MODE = FT810_REG_TOUCH_MODE,
	REG_TOUCH_ADC_MODE = FT810_REG_TOUCH_ADC_MODE,
	REG_TOUCH_CHARGE = FT810_REG_TOUCH_CHARGE,
	REG_TOUCH_SETTLE = FT810_REG_TOUCH_SETTLE,
	REG_TOUCH_OVERSAMPLE = FT810_REG_TOUCH_OVERSAMPLE,
	REG_TOUCH_RZTHRESH = FT810_REG_TOUCH_RZTHRESH,
	REG_TOUCH_RAW_XY = FT810_REG_TOUCH_RAW_XY,
	REG_TOUCH_RZ   = FT810_REG_TOUCH_RZ,
	REG_TOUCH_SCREEN_XY = FT810_REG_TOUCH_SCREEN_XY,
	REG_TOUCH_TAG_XY = FT810_REG_TOUCH_TAG_XY,
	REG_TOUCH_TAG  = FT810_REG_TOUCH_TAG,
	REG_TOUCH_TRANSFORM_A = FT810_REG_TOUCH_TRANSFORM_A,
	REG_TOUCH_TRANSFORM_B = FT810_REG_TOUCH_TRANSFORM_B,
	REG_TOUCH_TRANSFORM_C = FT810_REG_TOUCH_TRANSFORM_C,
	REG_TOUCH_TRANSFORM_D = FT810_REG_TOUCH_TRANSFORM_D,
	REG_TOUCH_TRANSFORM_E = FT810_REG_TOUCH_TRANSFORM_E,
	REG_TOUCH_TRANSFORM_F = FT810_REG_TOUCH_TRANSFORM_F,
	REG_TOUCH_CONFIG = FT810_REG_TOUCH_CONFIG,

	REG_SPI_WIDTH  = FT810_REG_SPI_WIDTH,

	REG_TOUCH_DIRECT_XY = FT810_REG_TOUCH_DIRECT_XY,
	REG_TOUCH_DIRECT_Z1Z2 = FT810_REG_TOUCH_DIRECT_Z1Z2,

	REG_CMDB_SPACE = FT810_REG_CMDB_SPACE,
	REG_CMDB_WRITE = FT810_REG_CMDB_WRITE,

	REG_TRACKER    = FT810_REG_TRACKER,
	REG_TRACKER1   = FT810_REG_TRACKER1,
	REG_TRACKER2   = FT810_REG_TRACKER2,
	REG_TRACKER3   = FT810_REG_TRACKER3,
	REG_TRACKER4   = FT810_REG_TRACKER4,
	REG_MEDIAFIFO_READ = FT810_REG_MEDIAFIFO_READ,
	REG_MEDIAFIFO_WRITE = FT810_REG_MEDIAFIFO_WRITE,
};
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_REFERENCE_API_H_ */

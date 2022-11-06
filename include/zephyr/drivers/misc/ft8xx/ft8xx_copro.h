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
 * @param dev Device pointer
 * @param cmd Display list command to execute
 */
void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd);

/**
 * @brief Execute a display list command, with uint32 parameter by co-processor engine
 *
 * @param dev Device pointer
 * @param cmd Display list command to execute
 * @param param command parameter
 */
void ft8xx_copro_cmd_uint(const struct device *dev, uint32_t cmd, uint32_t param);

/**
 * @brief Execute a display list command, with two uint32 parameter by co-processor engine
 *
 * @param dev Device pointer
 * @param cmd Display list command to execute
 * @param param  1st command parameter
 * @param param2 2nd command parameter
 */
void ft8xx_copro_cmd_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2);

/**
 * @brief Set API Level (bt817/8)
 *
 * @param dev Device pointer
 * @param level API level
 * @return 0 or -ENOTSUP if not supported by device 
 */
int ft8xx_copro_cmd_apilevel(const struct device *dev, uint32_t level );

/**
 * @brief Start a new display list
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_dlstart(const struct device *dev);

/**
 * @brief Sets co-processor engine to reset default states.
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_coldstart(const struct device *dev);

/**
 * @brief triggers interrupt INT_CMDFLAG.
 *
 * @param dev Device pointer
 * @param ms Delay before interrupt triggers, in milliseconds.
 
 */
void ft8xx_copro_cmd_interrupt(const struct device *dev, uint32_t ms );

/**
 * @brief Swap the current display list
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_swap(const struct device *dev);

/**
 * @brief - append memory to display list.
 *
 * @param dev Device pointer
 * @param ptr Start of source commands in device main memory
 * @param num Number of bytes to copy. This must be a multiple of 4.
 * @return 0 or -ENOTSUP if invalid number of bytes, or outside memory.
 */
int ft8xx_copro_cmd_append(const struct device *dev, uint32_t ptr, 
			uint32_t num );

/**
 * @brief - read a register value.
 *
 * @param dev Device pointer
 * @param ptr Address of register to read
 * @param result The register value to be read at ptr address.
 */
void ft8xx_copro_cmd_regread(const struct device *dev, uint32_t ptr,
			uint32_t result );


/**
 * @brief - write bytes into memory.
 *
 * @param dev Device pointer
 * @param ptr Address of memory to write to
 * @param num Number of bytes to be written
 * @param src pointer to data
 * @return 0 or -ENOTSUP if outside memory.
 */
int ft8xx_copro_cmd_memwrite(const struct device *dev, uint32_t ptr, 
			uint32_t num, uint8_t *src );


/**
 * @brief - decompress data into memory.
 *
 * @param dev Device pointer
 * @param ptr Address of memory to write to
 * @param src Pointer to DEFLATE commpressed data
 * @param len length of compressed data
 */
void ft8xx_copro_cmd_inflate(const struct device *dev, uint32_t ptr,
			uint32_t *src, uint32_t len );

/**
 * @brief - decompress data into memory. with options
 *
 * @param dev Device pointer
 * @param ptr Address of memory to write to
 * @param options options for inflate command
 * @param src Pointer to DEFLATE commpressed data
 * @param len length of compressed data
* @return 0 or -ENOTSUP not supported by device.
 */
int ft8xx_copro_cmd_inflate2(const struct device *dev, uint32_t ptr,
			uint32_t options, uint32_t *src, uint32_t len );


/**
 * @brief - load a JPEG image into memory.
 *
 * @param dev Device pointer
 * @param ptr Address of memory to write to
 * @param options image processing options 
 * @param src Pointer to JPEG(JFIF)/PNG image data
 * @param len length of JPEG(JFIF)/PNG image data
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_loadimage(const struct device *dev, uint32_t ptr,
				uint32_t options, uint32_t *src, uint32_t len );

/**
 * @brief - setup media streaming FIFO (ft81x)
 *
 * @param dev Device pointer
 * @param ptr Address of FIFO start in memory (4 byte aligned)
 * @param size size of FIFO (4 byte aligned)
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_mediafifo(const struct device *dev, uint32_t ptr,
				uint32_t size);

/**
 * @brief - play video
 *
 * @param dev Device pointer
 * @param options video playback options
 * @param src Pointer to video data (ignored if using fifo or flash)
 * @param len length of video data (ignored if using fifo or flash)
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_playvideo(const struct device *dev )uint32_t options,
				uint32_t *src, uint32_t len) ;



/**
 * @brief - Computes a CRC-32 for a block of FT8XX memory
 *
 * @param dev Device pointer
 * @param ptr Start address of memory block
 * @param num Size of memory block
 * @param result address for crc output
 */
void ft8xx_copro_cmd_memcrc(const struct device *dev, uint32_t ptr,
				uint32_t num, uint32_t result );

/**
 * @brief - write zero to a block of memory
 *
 * @param dev Device pointer
 * @param ptr Start address of memory block
 * @param num Size of memory block
 */
void ft8xx_copro_cmd_memzero(const struct device *dev, uint32_t ptr,
				uint32_t num);

/**
 * @brief Write a value to a block of memory
 *
 * @param dev Device pointer
 * @param ptr Start address of memory block
 * @param value value to be written
 * @param num Size of memory block
 */
void ft8xx_copro_cmd_memset(const struct device *dev, uint32_t ptr,
				uint32_t value, uint32_t num);

/**
 * @brief - copy block of memory
 *
 * @param dev Device pointer
 * @param dest Start address of destination memory block
 * @param src Start address of source memory block
 * @param num Size of memory block
 */
void ft8xx_copro_cmd_memcpy(const struct device *dev, uint32_t dest, 
				uint32_t src, uint32_t num);

/**
 * @brief Draw a button
 *
 * @param dev Device pointer
 * @param x x-coordinate of button top-left, in pixels
 * @param y y-coordinate of button top-left, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param s Button label, terminated with a null character
 */
void ft8xx_copro_cmd_button(const struct device *dev, ( int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				int16_t font,
				uint16_t options,
				const char* s );





/**
 * @brief Draw text
 *
 * By default (@p x, @p y) is the top-left pixel of the text and the value of
 * @p options is zero. @ref FT8XX_OPT_CENTERX centers the text horizontally,
 * @ref FT8XX_OPT_CENTERY centers it vertically. @ref FT8XX_OPT_CENTER centers
 * the text in both directions. @ref FT8XX_OPT_RIGHTX right-justifies the text,
 * so that the @p x is the rightmost pixel.
 *
 * @param dev Device pointer
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param s Character string to display, terminated with a null character
 */
void ft8xx_copro_cmd_text(const struct device *dev, int16_t x,
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
 * @param dev Device pointer
 * @param x x-coordinate of text base, in pixels
 * @param y y-coordinate of text base, in pixels
 * @param font Font to use for text, 0-31. 16-31 are ROM fonts
 * @param options Options to apply
 * @param n The number to display.
 */





void ft8xx_copro_cmd_number(const struct device *dev, int16_t x,
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
 * @param dev Device pointer
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

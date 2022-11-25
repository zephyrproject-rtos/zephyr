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





struct ft8xx_bitmap_transform_matrix_t
{
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
    int32_t e;
    int32_t f;  	
};

/**
 * @brief Execute a coprocessor command by co-processor engine
 *
 * @param dev Device pointer
 * @param cmd coprocessor command to execute
 */
void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd);

/**
 * @brief Execute a coprocessor command, with uint32 parameter by co-processor engine
 *
 * @param dev Device pointer
 * @param cmd coprocessor command to execute
 * @param param command parameter
 */
void ft8xx_copro_cmd_uint(const struct device *dev, uint32_t cmd, uint32_t param);

/**
 * @brief Execute a coprocessor command, with two uint32 parameter by co-processor engine
 *
 * @param dev Device pointer
 * @param cmd coprocessor command to execute
 * @param param  1st command parameter
 * @param param2 2nd command parameter
 */
void ft8xx_copro_cmd_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2);

/**
 * @brief Execute a coprocessor command, with three uint32 parameter by co-processor engine
 *
 * @param dev Device pointer
 * @param cmd coprocessor command to execute
 * @param param  1st command parameter
 * @param param2 2nd command parameter
 */
void ft8xx_copro_cmd_uint_uint_uint(const struct device *dev, uint32_t cmd, uint32_t param, uint32_t param2);


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
 * @param result The register value read from ptr address.
 */
void ft8xx_copro_cmd_regread(const struct device *dev, uint32_t ptr,
			uint32_t *result );


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
int ft8xx_copro_cmd_playvideo(const struct device *dev, uint32_t options,
				uint32_t *src, uint32_t len) ;

/**
 * @brief - initalize video frame decoder
 *
 * @param dev Device pointer
 * */
void ft8xx_copro_cmd_videostart(const struct device) ;

/**
 * @brief - load next video frame
 *
 * @param dev Device pointer
 * @param dst Location in ram to load frame
 * @param ptr location in ram for completion value
 * */
void ft8xx_copro_cmd_videoframe(const struct device, uint32_t dst,
				uint32_t ptr) ;

/**
 * @brief - Computes a CRC-32 for a block of FT8XX memory
 *
 * @param dev Device pointer
 * @param ptr Start address of memory block
 * @param num Size of memory block
 * @param result crc output
 */
void ft8xx_copro_cmd_memcrc(const struct device *dev, uint32_t ptr,
				uint32_t num, uint32_t *result );

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
void ft8xx_copro_cmd_button(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				int16_t font,
				uint16_t options,
				const char* s );

/**
 * @brief Draw a analogue clock 
 *
 * @param dev Device pointer
 * @param x x-coordinate of clock center, in pixels
 * @param y y-coordinate of clock center, in pixels
 * @param r the radius if the clock
 * @param options Options to apply
 * @param h hours
 * @param m minutes
 * @param s seconds
 * @param ms milliseconds
 */
void ft8xx_copro_cmd_clock(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t r,
				uint16_t options,
				uint16_t h,
				uint16_t m,
				uint16_t s,
				uint16_t ms )

/**
 * @brief - set foreground color
 *
 * @param dev Device pointer
 * @param c 	Foreground color as a 24bit RGB number
 */
void ft8xx_copro_cmd_fgcolor(const struct device *dev, uint32_t c);

/**
 * @brief - set background color
 *
 * @param dev Device pointer
 * @param c 	Foreground color as a 24bit RGB number
 */
void ft8xx_copro_cmd_bgcolor(const struct device *dev, uint32_t c);

/**
 * @brief - set background color
 *
 * @param dev Device pointer
 * @param c Highlight gradient color as a 24bit RGB number
 */
void ft8xx_copro_cmd_gradcolor(const struct device *dev, uint32_t c);

/**
 * @brief Draw a gauge
 *
 * @param dev Device pointer
 * @param x x-coordinate of gauge center, in pixels
 * @param y y-coordinate of gauge center, in pixels
 * @param r radius of the gauge
 * @param options Options to apply
 * @param major number of major subdivisions
 * @param minor number of minor subdivisions
 * @param val value of the gauge
 * @param range maximum value
 */
void ft8xx_copro_cmd_gauge(const struct device *dev, int16_t x,
			   int16_t y,
			   int16_t r,
			   uint16_t options,
			   uint16_t major,
			   uint16_t minor,
			   uint16_t val,
			   uint16_t range);

/**
 * @brief Draw a gradient
 *
 * @param dev Device pointer
 * @param x0 x-coordinate of point 0, in pixels
 * @param y0 y-coordinate of point 0, in pixels
 * @param rbg0 color at point 0 as 24bit rgb
 * @param x1 x-coordinate of point 1, in pixels
 * @param y1 y-coordinate of point 1, in pixels
 * @param rbg1 color at point 1 as 24bit rgb
 *  */
void ft8xx_copro_cmd_gradient(const struct device *dev, int16_t x0,
			   int16_t y0,
			   uint32_t rgb0,
			   int16_t x1,
			   int16_t y1,
			   uint32_t rgb1);

/**
 * @brief Draw a gradient with transparancy
 *
 * @param dev Device pointer
 * @param x0 x-coordinate of point 0, in pixels
 * @param y0 y-coordinate of point 0, in pixels
 * @param arbg0 color at point 0 as 32bit argb
 * @param x1 x-coordinate of point 1, in pixels
 * @param y1 y-coordinate of point 1, in pixels
 * @param arbg1 color at point 1 as 32bit argb
 *  */
void ft8xx_copro_cmd_gradientA(const struct device *dev, int16_t x0,
			   int16_t y0,
			   uint32_t argb0,
			   int16_t x1,
			   int16_t y1,
			   uint32_t argb1);


/**
 * @brief Draw a row of keys
 *
 * @param dev Device pointer
 * @param x x-coordinate of button top-left, in pixels
 * @param y y-coordinate of button top-left, in pixels
 * @param w width of keys
 * @param h height of keys
 * @param font Font to use for key label, 0-31. 
 * @param options Options to apply
 * @param s key labels, oone per key, terminated with a null character
 */
void ft8xx_copro_cmd_keys(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				int16_t font,
				uint16_t options,
				const char* s );

/**
 * @brief Draw a progress bar
 *
 * @param dev Device pointer
 * @param x x-coordinate of bar top-left, in pixels
 * @param y y-coordinate of bar top-left, in pixels
 * @param w width of bar
 * @param h height of bar
 * @param options Options to apply
 * @param val value of progress bar
 * @param range maximum value
 */
void ft8xx_copro_cmd_progress(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				uint16_t options,
				uint16_t val,
				uint16_t range);

/**
 * @brief Draw a scroll bar
 *
 * @param dev Device pointer
 * @param x x-coordinate of bar top-left, in pixels
 * @param y y-coordinate of bar top-left, in pixels
 * @param w width of bar
 * @param h height of bar
 * @param options Options to apply
 * @param size size of scroll bar
 * @param val value of progress bar
 * @param range maximum value
 */
void ft8xx_copro_cmd_scrollbar(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				uint16_t options,
				uint16_t val,
				uint16_t size,
				uint16_t range);

/**
 * @brief Draw a slider
 *
 * @param dev Device pointer
 * @param x x-coordinate of silder top-left, in pixels
 * @param y y-coordinate of silder top-left, in pixels
 * @param w width of silder
 * @param h height of silder
 * @param options Options to apply
 * @param val value of silder
 * @param range maximum value
 */
void ft8xx_copro_cmd_silder(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t h,
				uint16_t options,
				uint16_t val,
				uint16_t range);

/**
 * @brief Draw a dial
 *
 * @param dev Device pointer
 * @param x x-coordinate of dial center, in pixels
 * @param y y-coordinate of dial center, in pixels
 * @param r radius of dial
 * @param options Options to apply
 * @param val value of dial
 */
void ft8xx_copro_cmd_dial(const struct device *dev,  int16_t x,
				int16_t y,
				int16_t w,
				int16_t r,
				uint16_t options,
				uint16_t val);

/**
 * @brief Draw toggle
 *
 *
 * @param dev Device pointer
 * @param x x-coordinate of toggle top-left, in pixels
 * @param y y-coordinate of toggle top-left, in pixels
 * @param w width of toggle, in pixels
 * @param font Font to use for text, 0-31.
 * @param options Options to apply
 * @param state state of toggle,  0 is off, 65535 is on
 * @param s string for toggle labels,A character value of 255 separates the two labels.
 */
void ft8xx_copro_cmd_toggle(const struct device *dev, int16_t x,
			   int16_t y,
			   int16_t w,
			   int16_t font,
			   uint16_t options,
			   uint16_t state,
			   const char *s);

/**
 * @brief - set pixel fill width
 *
 *  For CMD_TEXT,CMD_BUTTON,CMD_BUTTON with the OPT_FILL option.
 *
 * @param dev Device pointer
 * @param s line fill width, in pixels
 */
void ft8xx_copro_cmd_fillwidth(const struct device *dev, uint32_t s);






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
 * @brief set current matrix to identity matrix
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_loadidentity(const struct device *dev);

/**
 * @brief set current matrix to bitmap transform matrix
 *
* @param dev Device pointer
 */
void ft8xx_copro_cmd_setmatrix(const struct device *dev);

/**
 * @brief get current matrix
 *
 * @param dev Device pointer
 * @param matrix pointer to bitmap transform matrix struct
 */
void ft8xx_copro_cmd_getmatrix(const struct device *dev, 
			const struct ft8xx_bitmap_transform_matrix_t *matrix );

/**
 * @brief get memory unallocated pointer
 *
 * At API level 1, the allocation pointer is advanced by cmd_inflate 
 * and cmd_inflate2
 *
 * At API level 2, the allocation pointer is also advanced by cmd_loadimage
 * cmd_playvideo, cmd_videoframe and cmd_endlist.
 *
 * @param dev Device pointer
 * @param result pointer to result 
 */
void ft8xx_copro_cmd_getptr(const struct device *dev, uint32_t *result );

/**
 * @brief get prperties of loaded image
 *
 * the Value of ptr differs based upon api lvl
 * At API level 1,
 *	For JPEG, it is the source address of the decoded image data in RAM_G
 *	For PNG, it is the first unused address in RAM_G after decoding process
 *
 * At API level 2, the source address of the decoded image data in RAM_G
 *
 * @param dev Device pointer
 * @param ptr pointer to result 
 * @param width pointer to width 
 * @param height pointer to height  
 */

void ft8xx_copro_cmd_getprops(const struct device *dev, uint32_t *ptr, uint32_t *width, 
			uint32_t *height );

/**
 * @brief scale current matrix
 *
 *	65536 is scale of 1 (or no change)
 *
 * @param dev Device pointer
 * @param sx x scale factor, in signed 16. 16 bit fixed-point Form. 
 * @param sy y scale factor, in signed 16. 16 bit fixed-point Form.  
 */

void ft8xx_copro_cmd_scale(const struct device *dev, int32_t sx, int32_t sy );

/**
 * @brief rotate current matrix
 *
 *
 * @param dev Device pointer
 * @param a Clockwise rotation angle, in units of 1/65536 of a circle
 */

void ft8xx_copro_cmd_rotate(const struct device *dev, int32_t a );


/**
 * @brief rotate current matrix around specific coordinate
 *
 *
 * @param dev Device pointer
 * @param x center of rotation/scaling, x-coordinate
 * @param y center of rotation/scaling, y-coordinate
 * @param a Clockwise rotation angle, in units of 1/65536 of a circle
 * @param s scale factor, in signed 16.16 bit fixed-point form
 */
void ft8xx_copro_cmd_rotatearound(const struct device *dev, int32_t x, int32_t y,
			int32_t a,
			int32_t s);

/**
 * @brief  apply a translation to the current matrix
 *
 *
 * @param dev Device pointer
 * @param x x translate factor, in signed 16.16 bit fixed-point Form
 * @param y y translate factor, in signed 16.16 bit fixed-point Form
 */
void ft8xx_copro_cmd_translate(const struct device *dev, int32_t x, int32_t y);

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
 * @brief Execute the touch screen calibration routine
 *
 * This command is used to execute the touch screen calibration routine for 
 * a sub-window. Like CMD_CALIBRATE, except that instead of using the whole 
 * screen area, uses a smaller sub-window specified for the command. This is 
 * intended for panels which do not use the entire defined surface.
 *
 * @param dev Device pointer
 * @param x x-coordinate of top-left of subwindow, in pixels.
 * @param y y-coordinate of top-left of subwindow, in pixels.
 * @param w width of subwindow, in pixels.
 * @param h height of subwindow, in pixels.
 * @param result Calibration result, written with 0 on failure of calibration
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_calibratesub(const struct device *dev, uint16_t x, uint16_t y,
			uint16_t w,
			uint16_t h,
			uint32_t *result);

/**
 * @brief Rotate Screen
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param r screen oriemtation (0-7)
 */
void ft8xx_copro_cmd_setrotation(const struct device *dev, uint32_t r);



/**
 * @brief display animated spinner
 *
 *
 * @param dev Device pointer
 * @param x x-coordinate of top-left of spinner, in pixels.
 * @param y y-coordinate of top-left of spinner, in pixels.
 * @param style spinner style (0-3)
 * @param scale scale factor, 0 is no scaling
 */
void ft8xx_copro_cmd_spinner(const struct device *dev, int16_t x, int16_t y,
			uint16_t style,
			uint16_t scale);

/**
 * @brief display screensaver
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_screensaver(const struct device *dev);

/**
 * @brief sketch bitmap from touchscreen input
 *
 * This command is used to start a continuous sketch update. The Coprocessor 
 * engine continuously  * samples the touch inputs and paints pixels into a 
 * bitmap, according to the given touch (x, y). This means that the user touch
 * inputs are drawn into the bitmap without any need for MCU work.
 * CMD_STOP is to be sent to stop the sketch process.
 *
 * @param dev Device pointer
 * @param x x-coordinate of sketch area top-left, in pixels
 * @param y y-coordinate of sketch area top-left, in pixels
 * @param w Width of sketch area, in pixels
 * @param h Height of sketch area, in pixels
 * @param ptr Base address of sketch bitmap
 * @param format Format of sketch bitmap, either L1 or L8
 */
void ft8xx_copro_cmd_sketch(const struct device *dev, int16_t x,
			int16_t y,
			uint16_t w,
			uint16_t h,
			uint32_t ptr,
			uint16_t format );


/**
 * @brief stop periodic operations
 *
 * This command is to inform the coprocessor engine to stop the 
 * periodic operation, which is triggered by CMD_SKETCH, 
 * CMD_SPINNER or CMD_SCREENSAVER
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_stop(const struct device *dev);


/**
 * @brief register custom font
 *
 * @param dev Device pointer
 * @param font The bitmap handle from 0 to 31
 * @param ptr The metrics block address in RAM_G. 4 bytes aligned is required.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_setfont(const struct device *dev, uint32_t font, uint32_t ptr);


/**
 * @brief register custom font
 *
 * @param dev Device pointer
 * @param font The bitmap handle from 0 to 31
 * @param ptr 32 bit aligned memory address in RAM_G of font metrics block
 * @param firstchar The ASCII value of first character in the font. For an extended font block, this should be zero.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_setfont2(const struct device *dev, uint32_t font, uint32_t ptr,
			uint32_t firstchar);


/**
 * @brief set scratch bitmap
 *
 * Graphical objects use a bitmap handle for rendering. By default this is 
 * bitmap handle 15. This command allows it to be set to any bitmap handle.
 * This command enables user to utilize bitmap handle 15 safely
 *
 * @param dev Device pointer
 * @param handle bitmap handle number, 0~3
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_setscratch(const struct device *dev, uint32_t handle);


/**
 * @brief load romfront into bitmap handle
 *
 * By default ROM fonts 16-31 are loaded into bitmap handles 16-31. This 
 * command allows any ROM font 16-34 to be loaded into any bitmap handle
 *
 * @param dev Device pointer
 * @param font bitmap handle number, 0~31
 * @param romslot ROM font number, 16~34
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_romfont(const struct device *dev, uint32_t font,
			uint32_t romslot );


/**
 * @brief resets bitmap handles 16-31 to to default fonts  
 *
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_resetfonts(const struct device *dev);


/**
 * @brief track touches for graphics object
 *
 * tracked values are reported in REG_TRACKER (and REG_TRACKER_1-4
 * where supported by touch pannel)
 *
 * Note: Multiple touch points are only available in FT81X/BT81X Series with 
 * capacitive displays connected.
 *
 * For a rotary tracker this value is the angle of the touch point 
 * relative to the object center, in units of 1/65536 of a circle.
 *
 * For a linear tracker  this value is the distance along the tracked
 * object, from 0 to 65535. 0 is straignt down
 *
 * for linear tracker cordinates are for top-left
 * for rotary tracker cordinates are for center
 *
 * @param dev Device pointer
 * @param x x-coordinate of track area, in pixels
 * @param y y-coordinate of track area, in pixels
 * @param w Width of track area, in pixels
 * @param h Height of track area, in pixels
 * @param tag tag of the graphics object to be tracked, 1-255
 */
void ft8xx_copro_cmd_track(const struct device *dev, int16_t x,
			int16_t y,
			int16_t w,
			int16_t h,
			int16_t tag);

/**
 * @brief take snapshot of screen  
 *
 * This command causes the coprocessor engine to take a snapshot of the 
 * current screen, and write the result into RAM_G as an ARGB4 bitmap. 
 * The size of the bitmap is the size of the screen, given by the REG_HSIZE 
 * and REG_VSIZE registers.
 *
 * During the snapshot, the display should be disabled by setting REG_PCLK
 * to 0 to avoid display glitch. 
 *
 * @param dev Device pointer
 * @param ptr Snapshot destination address, in RAM_G
 */
void ft8xx_copro_cmd_snapshot(const struct device *dev,  uint32_t ptr);


/**
 * @brief take snapshot of part of screen  
 *
 * This command causes the coprocessor engine to take a snapshot of the 
 * current screen, and write the result into RAM_G as in specified format bitmap. 
 *
 * @param dev Device pointer
 * @param ptr Snapshot destination address, in RAM_G
 * @param fmt Output bitmap format, one of RGB565, ARGB4 or ARGB8 format snapshot.
 * @param ptr Snapshot destination address, in RAM_G
 * @param x x-coordinate of snapshot area top-left, in pixels
 * @param y y-coordinate of snapshot area top-left, in pixels
 * @param w Width of snapshot area, in pixels
 * @param h Height of snapshot area, in pixels
 */
void ft8xx_copro_cmd_snapshot2(const struct device *dev,  uint32_t ptr);

/**
 * @brief generate display list comands for bitmap 
 *
 * This command will generate the corresponding display list commands for given 
 * bitmap information.
 *
 * @param dev Device pointer
 * @param source source address, in RAM_G or flash memory, 
 * @param fmt Bitmap format, see the definition in BITMAP_EXT_FORMAT.
 * @param width Width of bitmap, in pixels.
 * @param height Height of bitmap, in pixels.
 */
void ft8xx_copro_cmd_setbitmap(const struct device *dev,  uint32_t source,
			uint16_t fmt,
			uint16_t width,
			uint16_t height );

/**
 * @brief Display animated device logo  
 *
 * This command causes the coprocessor engine to play animation of device logo
 * MCU should not interact with Display lists durinng playback.
 * after 2.5 seconds normal operation may resume.
 *
 * Logo varies by device 
 *
 * @param dev Device pointer
 */
void ft8xx_copro_cmd_logo(const struct device *dev);

/**
 * @brief Erases attached Flash storage
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flasherase(const struct device *dev);

/**
 * @brief writes data to attached Flash storage
 *
 * @param dev Device pointer
 * @param ptr Destination address in flash memory, 256-byte aligned, first address block at zero
 * @param num Number of bytes to write, multiple of 256
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashwrite(const struct device *dev, uint32_t ptr,
			uint32_t num);

/**
 * @brief writes data to blank Flash storage from RAM
 *
 * @param dev Device pointer
 * @param dest Destination address in flash memory, 4096-byte aligned, first address block at zero
 * @param src Source data in main memory. Must be 4-byte aligned
 * @param num Number of bytes to write, multiple of 4096.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashprogram(const struct device *dev, uint32_t dest,
			uint32_t src,
			uint32_t num);

/**
 * @brief read data to from Flash memory to RAM
 *
 * @param dev Device pointer
 * @param dest Destination address in main memory, 4-byte aligned
 * @param src Source data in flash memory. Must be 64-byte aligned, first address block at zero
 * @param num Number of bytes to write, multiple of 4, automatically expands to minimum required.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashread(const struct device *dev, uint32_t dest,
			uint32_t src,
			uint32_t num);

/**
 * @brief appends data from flash to next available location in display list RAM_DL
 *
 * @param dev Device pointer
 * @param ptr source address in flash memory, 64-byte aligned, first address block at zero
 * @param num Number of bytes to copy, multiple of 4.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_appendf(const struct device *dev, uint32_t ptr,
			uint32_t num);


/**
 * @brief writes data to Flash storage from RAM
 *
 * @param dev Device pointer
 * @param dest Destination address in flash memory, 4096-byte aligned, first address block at zero
 * @param src Source data in RAM_G. Must be 4-byte aligned
 * @param num Number of bytes to write, multiple of 4096.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashupdate(const struct device *dev, uint32_t dest,
			uint32_t src,
			uint32_t num);

/**
 * @brief detach from Flash
 *
 * the only valid flash operations when detached are the low-level SPI access commands as following:
 * CMD_FLASHSPIDESEL, CMD_FLASHSPITX, CMD_FLASHSPIRX, CMD_FLASHATTACH
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashdetach(const struct device *dev);

/**
 * @brief attach to Flash
 *
 * attach to flash allowing high level flash dependent commands 
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashattach(const struct device *dev);

/**
 * @brief Switch flash to full-speed mode
 *
 * Error Codes
 * 0xE001 flash is not supported
 * 0xE002 no header detected in sector 0 – is flash blank?
 * 0xE003 sector 0 data failed integrity check
 * 0xE004 device/blob mismatch – was correct blob loaded?
 * 0xE005 failed full-speed test – check board wiring
 *
 * @param dev Device pointer
 * @param result pointer to success or error code result 
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashfast(const struct device *dev, uint32_t *result);

/**
 * @brief de-asserts the SPI CS signal.
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashspidesel(const struct device *dev);

/**
 * @brief transmit bytes over flash spi interface.
 *
 * requires flash to be detatched with CMD_FLASHDETACH.
 *
 * @param dev Device pointer
 * @param num number of bytes
 * @param src pointer to bytes to write
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashspitx(const struct device *dev, uint32_t num,
			uint8_t *src );

/**
 * @brief read bytes from flash spi interface into RAM_G
 *
 * requires flash to be detatched with CMD_FLASHDETACH.
 *
 * @param dev Device pointer
 * @param ptr destination address in RAM_G 
 * @param num number of bytes
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashspirx(const struct device *dev, uint32_t ptr,
			uint32_t num)



/**
 * @brief clear graphics engine internal flash cache
 *
 * should be executed after modifying graphics data in flash by 
 * CMD_FLASHUPDATE or CMD_FLASHWRITE.
 *
 * display list must be empty, immediately after a CMD_DLSTART 
 *
 * @param dev Device pointer
 * @param ptr destination address in RAM_G 
 * @param num number of bytes
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_clearcache(const struct device *dev);


/**
 * @brief set flash data source address
 *
 * for flash data loaded by the CMD_LOADIMAGE, CMD_PLAYVIDEO, 
 * CMD_VIDEOSTARTF and CMD_INFLATE2 commands with the OPT_FLASH
 * option.
 *
 * @param dev Device pointer
 * @param ptr flash address, 64 byte aligned, first address block at zero
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_flashsource(const struct device *dev, uint32_t ptr);

/**
 * @brief process video header information video previously set by CMD_FLASHSOURCE
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_videostartf(const struct device *dev);


/**
 * @brief start an animation from flash
 *
 * If no channel is available, then an “out of channels” exception is raised
 *
 * ANIM_ONCE plays the animation once, then cancel it. 
 * ANIM_LOOP pays the animation in a loop. 
 * ANIM_HOLD plays the animation once, then displays the final frame
 *
 *
 * @param dev Device pointer
 * @param ch Animation channel, 0-31. 
 * @param aoptr The address of the animation object in flash memory.
 * @param loop Loop flags.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_animstart(const struct device *dev, int32_t ch,
			uint32_t aoptr,
			uint32_t loop);

/**
 * @brief start an animation from G_RAM
 *
 * If no channel is available, then an “out of channels” exception is raised
 *
 * ANIM_ONCE plays the animation once, then cancel it. 
 * ANIM_LOOP pays the animation in a loop. 
 * ANIM_HOLD plays the animation once, then displays the final frame
 *
 *
 * @param dev Device pointer
 * @param ch Animation channel, 0-31. 
 * @param aoptr The address of the animation object in RAM. 64-byte aligned
 * @param loop Loop flags.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_animstartram(const struct device *dev, int32_t ch,
			uint32_t aoptr,
			uint32_t loop);

/**
 * @brief run animation until complete
 *
 * this command is used to Play/run animations until complete. Playback ends 
 * when either a specified animation completes, or when host MCU writes to a
 * control byte.
 *
 * Animation ends when the logical AND of waitmask and REG_ANIM_ACTIVE is zero.
 *
 * @param dev Device pointer
 * @param waitmask 32-bit mask specify animation channels to wait for.
 * @param play address of play control byte (0xFFFFFFFF for no control byte)
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_animstartram(const struct device *dev, uint32_t waitmask,
			uint32_t play);

/**
 * @brief stop one or more active animations
 *
 * @param dev Device pointer
 * @param ch Animation channel, 0-31. -1 for all channels
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_animstop(const struct device *dev, int32_t ch);

/**
 * @brief set coordinates of animation
 *
 * NOTE: If the pixel precision is not set to 1/16 in current graphics 
 * context, a VERTEX_FOMART(4) is mandatory to precede this command
 *
 * @param dev Device pointer
 * @param ch Animation channel, 0-31.
 * @param x x-coordinate of animation center, in pixels
 * @param y y-coordinate of animation center, in pixels
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_animxy(const struct device *dev, int32_t ch, int16_t x,
			int16_t y);


/**
 * @brief stop one or more active animations
 *
 * @param dev Device pointer
 * @param ch Animation channel, 0-31. -1 for all undrawn in ascending order
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_animdraw(const struct device *dev, int32_t ch)



/**
 * @brief draw specified animation frame from flash
 *
 * NOTE: If the pixel precision is not set to 1/16 in current graphics 
 * context, a VERTEX_FOMART(4) is mandatory to precede this command
 * @param dev Device pointer
 * @param x x-coordinate of animation center, in pixels
 * @param y y-coordinate of animation center, in pixels
 * @param aoptr The address of the animation object in flash.
 * @param frame frame number to draw.
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_animframe(const struct device *dev, int16_t x,
			int16_t y, 
			uint32_t aoptr,
			uint32_t frame)

/**
 * @brief draw specified animation frame from RAM_G
 *
 * Note: If the pixel precision is not set to 1/16 in current graphics 
 * context, a VERTEX_FOMART(4) is mandatory to precede this command
 * 
 * @param dev Device pointer
 * @param x x-coordinate of animation center, in pixels
 * @param y y-coordinate of animation center, in pixels
 * @param aoptr The address of the animation object in RAM_G. 64-byte aligned
 * @param frame frame number to draw.
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_animframeram(const struct device *dev, int16_t x,
			int16_t y, 
			uint32_t aoptr,
			uint32_t frame)

/**
 * @brief  wait for the end of the video scan out period
 *
 *
 * 
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_sync(const struct device *dev);


/**
 * @brief  calculate bitmap transform 
 *
 * 
 * @param dev Device pointer
 * @param x0 Point 0 screen x coordinate, in pixels
 * @param y0 Point 0 screen y coordinate, in pixels
 * @param x1 Point 1 screen x coordinate, in pixels
 * @param y1 Point 1 screen y coordinate, in pixels
 * @param x2 Point 2 screen x coordinate, in pixels
 * @param y2 Point 2 screen y coordinate, in pixels
 * @param tx0 Point 0 bitmap x coordinate, in pixels
 * @param ty0 Point 0 bitmap y coordinate, in pixels
 * @param tx1 Point 1 bitmap x coordinate, in pixels
 * @param ty1 Point 1 bitmap y coordinate, in pixels
 * @param tx2 Point 2 bitmap x coordinate, in pixels
 * @param ty2 Point 2 bitmap y coordinate, in pixels
 * @param result pointer to result, -1 on success, or 0 failed to find solution matrix.
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_bitmap_transform(const struct device *dev, int32_t x0,
			int32_t y0,
			int32_t x1,
			int32_t y1,
			int32_t x2,
			int32_t y2,
			int32_t tx0,
			int32_t ty0,
			int32_t tx1,
			int32_t ty1,
			int32_t tx2,
			int32_t ty2,
			uint16_t *result);


/**
 * @brief  Display Testcard
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_testcard(const struct device *dev);

/**
 * @brief wait for number of microseconds
 *
 * Delays of more than 1s (1000000 μs) are not supported.
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param us Number of microseconds to wait
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_wait(const struct device *dev, uint32_t us);

/**
 * @brief Start new command list in RAM_G
 *
 * Instead of being executed, the following commands are appended to the list, 
 * until the following CMD_ENDLIST. 
 * The list can then be called with CMD_CALLIST.
 *
 * CMD_FLASHSPITX, CMD_FLASHWRITE, CMD_INFLATE, CMD_NEWLIST
 * are not supported in command lists
 *
 * CMD_INFLATE2, CMD_LOADIMAGE, CMD_PLAYVIDEO
 * are only supported using option OPT_MEDIAFIFO
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param a memory address of start of command list
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_newlist(const struct device *dev, uint32_t a);



/**
 * @brief Completes creation of command list
 *
 * CMD_GETPTR can be used to find the first unused memory address 
 * following the command list.
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_endlist(const struct device *dev)

/**
 * @brief Calls a previosly created command list
 *
 * Command Lists can be nested upto 4 levels
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param a memory address of start of command list
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_calllist(const struct device *dev, uint32_t a)

/**
 * @brief ends a commandlist
 *
 * this is done automatically by CMD_ENDLIST
 *
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_return(const struct device *dev)


/**
 * @brief enables font cache.
 *
 * loads all the bitmaps (glyph) used by a flash-based font into a RAM_G area
 * the area must be able to hold all bitmaps for two consecutive frames
 * applies to ASTC based custom font only
 *
 * font of 255 disable cache
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param font font handle to cache. Must be an extended format font.
 * @param ptr Start of cache area, 64-byte aligned
 * @param num Size of cache area in bytes, 4 byte aligned. At minimum 16 Kbytes.
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_fontcache(const struct device *dev, uint32_t font,
			int32_t ptr,
			uint32_t num )


/**
 * @brief query the capacity and utilization of the font cache.
 *
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param total font handle to cache. Must be an extended format font.
 * @param used Start of cache area, 64-byte aligned
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_fontcachequery(const struct device *dev, uint32_t *total,
			int32_t used)

/**
 * @brief query the capacity and utilization of the font cache.
 *
 * get attributes for bitmap created by MD_LOADIMAGE, CMD_PLAYVIDEO, CMD_VIDEOSTART
 * or CMD_VIDEOSTARTF
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param source pointer to output for source address of bitmap 
 * @param fmt pointer to output for bitmap format
 * @param w pointer to output for Width of bitmap, in pixels
 * @param h pointer to output for Height of bitmap, in pixels
 * @param palette pointer to output for Palette data of the bitmap if fmt is PALETTED565 or PALETTED4444. Otherwise zero
 
 * @return 0 or -ENOTSUP if invalid options for device.
 */
int ft8xx_copro_cmd_fontcachequery(const struct device *dev, uint32_t *source,
			int32_t *fmt,
			int32_t *w,
			int32_t *h,
			int32_t *palette)



/**
 * @brief set pixel width
 *
 * used in conjunmction with REG_HSIZE to adjust for non-square pixels
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param w Output pixel width, which must be less than REG_HSIZE. 0 disables HSF.
 * @param palette pointer to output for Palette data of the bitmap if fmt is PALETTED565 or PALETTED4444. Otherwise zero
 
 * @return 0 or -ENOTSUP if invalid options for device.
 */


int ft8xx_copro_cmd_hsf(const struct device *dev, uint32_t w)



/**
 * @brief set set pclk frequency
 *
 * used in conjunmction with REG_HSIZE to adjust for non-square pixels
 *
 * Note: BT817/8 specific command
 *
 * @param dev Device pointer
 * @param ftarget Target frequency, in Hz.
  * @param rounding Approximation mode. Valid values are 0, -1, 1:
 * @param factual pointer to output for Actual frequency achieved, or zero on failure
 * @return 0 or -ENOTSUP if invalid options for device.
 */

int ft8xx_copro_cmd_pclkfreq(const struct device *dev, uint32_t ftarget,
			int32_t rounding,
			uint32_t factual );



/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COPRO_H_ */

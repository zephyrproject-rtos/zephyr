/*
 * Copyright (c) 2022
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX public API
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_API_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

#include <zephyr/drivers/flash.h>

/**
 * @brief FT8xx driver public APIs
 * @defgroup ft8xx_interface FT8xx driver APIs
 * @ingroup misc_interfaces
 * @{
 */

enum ft81x_options
{
    FT81x_OPT_3D            = 0,
    FT81x_OPT_RGB565        = 0,
    FT81x_OPT_MONO          = 1,
    FT81x_OPT_NODL          = 2,
    FT81x_OPT_FLAT          = 256,
    FT81x_OPT_CENTERX       = 512,
    FT81x_OPT_CENTERY       = 1024,
    FT81x_OPT_CENTER        = 1536,
    FT81x_OPT_RIGHTX        = 2048,
    FT81x_OPT_NOBACK        = 4096,
    FT81x_OPT_FILL          = 8192,
    FT81x_OPT_FLASH         = 64,
    FT81x_OPT_FORMAT        = 4096,
    FT81x_OPT_NOTICKS       = 8192,
    FT81x_OPT_NOHM          = 16384,
    FT81x_OPT_NOPOINTER     = 16384,
    FT81x_OPT_NOSECS        = 32768,
    FT81x_OPT_NOHANDS       = 49152,
    FT81x_OPT_NOTEAR        = 4,
    FT81x_OPT_FULLSCREEN    = 8,
    FT81x_OPT_MEDIAFIFO     = 16,
    FT81x_OPT_OVERLAY       = 128,
    FT81x_OPT_SOUND         = 32,
    FT81x_OPT_DITHER        = 256,

}

enum ft81x_formats
{
	FT81x_FORMAT_ARGB1555		= 0,
	FT81x_FORMAT_L1	 			= 1,
	FT81x_FORMAT_L2				= 2,
	FT81x_FORMAT_L8				= 3,
	FT81x_FORMAT_RGB332			= 4,
	FT81x_FORMAT_ARGB2			= 5,
	FT81x_FORMAT_ARGB4			= 6,
	FT81x_FORMAT_RGB565			= 7,
	FT81x_FORMAT_TEXT8X8		= 9,
	FT81x_FORMAT_TEXTVGA		= 10,
	FT81x_FORMAT_BARGRAPH		= 11,
	FT81x_FORMAT_PALETTED565	= 14,
	FT81x_FORMAT_PALETTED4444	= 15,
	FT81x_FORMAT_PALETTED8		= 16,
	FT81x_FORMAT_L2				= 17,
	FT81x_FORMAT_ARGB8			= 20,
	FT81x_FORMAT_GLFORMAT		= 31,
}


/**
 * @struct ft8xx_touch_transform
 * @brief Structure holding touchscreen calibration data
 *
 * The content of this structure is filled by ft8xx_calibrate().
 */
struct ft8xx_touch_transform {
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
	uint32_t e;
	uint32_t f;
};


/**
 * @typedef ft8xx_int_callback
 * @brief Callback API to inform API user that FT8xx triggered interrupt
 *
 * This callback is called from IRQ context.
 */
typedef void (*ft8xx_int_callback)(void);


// basic commands
typedef void (*ft8xx_calibrate_t)(const struct device *dev, struct ft8xx_touch_transform *trform)
typedef void (*ft8xx_touch_transform_set_t)(const struct device *dev, const struct ft8xx_touch_transform *trform)
typedef void (*ft8xx_touch_transform_get_t)(const struct device *dev, const struct ft8xx_touch_transform *trform)
typedef int (*ft8xx_get_touch_tag_t)(const struct device *dev);
typedef void (*ft8xx_register_int_t)(const struct device *dev, ft8xx_int_callback callback);
typedef void (*ft8xx_host_command_t)(const struct device *dev, uint8_t cmd)
typedef void (*ft8xx_command_t)(const struct device *dev, uint32_t command)



// display list commands



// audio commands
typedef int (*ft8xx_audio_load_t)(const struct device *dev, uint32_t start_address, uint8_t* sample, uint32_t sample_length);
typedef int (*ft8xx_audio_play_t)(const struct device *dev, uint32_t start_address, uint32_t sample_length, uint8_t audio_format, uint16_t sample_freq, uint8_t vol, bool loop);
typedef int (*ft8xx_audio_get_status_t)(const struct device *dev);
typedef int (*ft8xx_audio_stop_t)(const struct device *dev);


typedef int (*ft8xx_audio_synth_start_t)(const struct device *dev, uint8_t sound, uint8_t note);
typedef int (*ft8xx_audio_synth_get_status_t)(const struct device *dev);
typedef int (*ft8xx_audio_synth_stop_t)(const struct device *dev);


// touch commands



// coprocessor commands
typedef void (*ft8xx_copro_cmd_apilevel_t)(const struct device *dev, uint32_t level);   //bt817/8 only
typedef void (*ft8xx_copro_cmd_dlstart_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_coldstart_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_interrupt_t)(const struct device *dev, uint32_t ms );
typedef void (*ft8xx_copro_cmd_swap_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_append_t)(const struct device *dev, uint32_t ptr, uint32_t num );
typedef void (*ft8xx_copro_cmd_regread_t)(const struct device *dev, uint32_t ptr, uint32_t result );
typedef void (*ft8xx_copro_cmd_memwrite_t)(const struct device *dev, uint32_t ptr, uint32_t num, uint32_t *src);
typedef void (*ft8xx_copro_cmd_inflate_t)(const struct device *dev, uint32_t ptr );
typedef void (*ft8xx_copro_cmd_inflate2_t)(const struct device *dev, uint32_t ptr, uint32_t options );
typedef void (*ft8xx_copro_cmd_loadimage_t)(const struct device *dev, uint32_t ptr, uint32_t options );
typedef void (*ft8xx_copro_cmd_mediafifo_t)(const struct device *dev, uint32_t ptr, uint32_t size ); // ft81x  
typedef void (*ft8xx_copro_cmd_playvideo_t)(const struct device *dev, uint32_t opts, uint32_t ptr, uint32_t size ); // ft81x  
typedef void (*ft8xx_copro_cmd_videostart_t)(const struct device *dev); // ft81x  
typedef void (*ft8xx_copro_cmd_videoframe_t)(const struct device *dev,uint32_t dst, uint32_t ptr ) ); // ft81x  
typedef void (*ft8xx_copro_cmd_memcrc_t)(const struct device *dev,uint32_t ptr, uint32_t num, uint32_t result );
typedef void (*ft8xx_copro_cmd_memzero_t)(const struct device *dev,uint32_t ptr, uint32_t num );
typedef void (*ft8xx_copro_cmd_memset_t)(const struct device *dev,uint32_t ptr, uint32_t value, uint32_t num );
typedef void (*ft8xx_copro_cmd_memcpy_t)(const struct device *dev,uint32_t dest, uint32_t src, uint32_t num );
typedef void (*ft8xx_copro_cmd_button_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, uint16_t options, const char* s );
typedef void (*ft8xx_copro_cmd_clock_t)(const struct device *dev,int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms );
typedef void (*ft8xx_copro_cmd_fgcolor_t)(const struct device *dev,uint32_t c );
typedef void (*ft8xx_copro_cmd_bgcolor_t)(const struct device *dev,uint32_t c );
typedef void (*ft8xx_copro_cmd_gradcolor_t)(const struct device *dev,uint32_t c );
typedef void (*ft8xx_copro_cmd_gauge_t)(const struct device *dev,int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t major, uint16_t minor, uint16_t val, uint16_t range );
typedef void (*ft8xx_copro_cmd_gradient_t)(const struct device *dev,int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1 );
typedef void (*ft8xx_copro_cmd_gradienta_t)(const struct device *dev,int16_t x0, int16_t y0, uint32_t argb0, int16_t x1, int16_t y1, uint32_t argb1 ); //bt81x
typedef void (*ft8xx_copro_cmd_keys_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t h, int16_t font, uint16_t options, const char* s );
typedef void (*ft8xx_copro_cmd_progress_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range )
typedef void (*ft8xx_copro_cmd_scrollbar_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t size, uint16_t range );
typedef void (*ft8xx_copro_cmd_slider_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range );
typedef void (*ft8xx_copro_cmd_dial_t)(const struct device *dev,int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val );
typedef void (*ft8xx_copro_cmd_toggle_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t font, uint16_t options, uint16_t state, const char* s );
typedef void (*ft8xx_copro_cmd_fillwidth_t)(const struct device *dev, uint32_t s ); //bt81x
typedef void (*ft8xx_copro_cmd_setbase_t)(const struct device *dev, uint32_t b ); 

typedef void (*ft8xx_copro_cmd_text_t)(const struct device *dev,int16_t x, int16_t y, int16_t font, uint16_t options, const char* s );
typedef void (*ft8xx_copro_cmd_number_t)(const struct device *dev,int16_t x, int16_t y, int16_t font, uint16_t options, int32_t n );
typedef void (*ft8xx_copro_cmd_loadidentity_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_setmatrix_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_getmatrix_t)(const struct device *dev,int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f );
typedef void (*ft8xx_copro_cmd_getptr_t)(const struct device *dev,uint32_t result);
typedef void (*ft8xx_copro_cmd_getprops_t)(const struct device *dev,uint32_t &ptr, uint32_t &width, uint32_t &height);
typedef void (*ft8xx_copro_cmd_scale_t)(const struct device *dev,int32_t sx, int32_t sy );
typedef void (*ft8xx_copro_cmd_rotate_t)(const struct device *dev, int32_t a );
typedef void (*ft8xx_copro_cmd_rotatearound_t)(const struct device *dev, int32_t x, int32_t y, int32_t a, int32_t s); //bt81x
typedef void (*ft8xx_copro_cmd_translate_t)(const struct device *dev,int32_t tx, int32_t ty );
typedef void (*ft8xx_copro_cmd_calibrate_t)(const struct device *dev,uint32_t result );
typedef void (*ft8xx_copro_cmd_calibratesub_t)(const struct device *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t result ); //bt817/8
typedef void (*ft8xx_copro_cmd_setrotate_t)(const struct device *dev, uint32_t r); //ft81x
typedef void (*ft8xx_copro_cmd_spinner_t)(const struct device *dev,int16_t x, int16_t y, uint16_t style, uint16_t scale );
typedef void (*ft8xx_copro_cmd_screensaver_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_sketch_t)(const struct device *dev,int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t ptr, uint16_t format );
typedef void (*ft8xx_copro_cmd_stop_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_setfont_t)(const struct device *dev,uint32_t font, uint32_t ptr );
typedef void (*ft8xx_copro_cmd_setfont2_t)(const struct device *dev,uint32_t font, uint32_t ptr, uint32_t firstchar ); //ft81x
typedef void (*ft8xx_copro_cmd_setscratch_t)(const struct device *dev, uint32_t handle ); //ft81x
typedef void (*ft8xx_copro_cmd_romfont_t)(const struct device *dev, uint32_t font, uint32_t romslot ); //ft81x
typedef void (*ft8xx_copro_cmd_resetfonts_t)(const struct device *dev); //bt81x
typedef void (*ft8xx_copro_cmd_track_t)(const struct device *dev,int16_t x, int16_t y, int16_t w, int16_t h, int16_t tag );
typedef void (*ft8xx_copro_cmd_snapshot_t)(const struct device *dev,uint32_t ptr );
typedef void (*ft8xx_copro_cmd_snapshot2_t)(const struct device *dev,  uint32_t fmt, uint32_t ptr, int16_t x, int16_t y, int16_t w, int16_t h); //ft81x
typedef void (*ft8xx_copro_cmd_setbitmap_t)(const struct device *dev, uint32_t source, uint16_t fmt, uint16_t width, uint16_t height); //ft81x
typedef void (*ft8xx_copro_cmd_logo_t)(const struct device *dev);

//bt81x
typedef void (*ft8xx_copro_cmd_flasherase_t)(const struct device *dev); 
typedef void (*ft8xx_copro_cmd_flashwrite_t)(const struct device *dev, uint32_t ptr, uint32_t num );
typedef void (*ft8xx_copro_cmd_flashprogram_t)(const struct device *dev, uint32_t dest, uint32_t src, uint32_t num );
typedef void (*ft8xx_copro_cmd_flashread_t)(const struct device *dev, uint32_t dest, uint32_t src, uint32_t num );
typedef void (*ft8xx_copro_cmd_appendf_t)(const struct device *dev, uint32_t ptr,uint32_t num);
typedef void (*ft8xx_copro_cmd_flashupdate_t)(const struct device *dev, uint32_t dest, uint32_t src, uint32_t num );
typedef void (*ft8xx_copro_cmd_flashdetach_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_flashattach_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_flashfast_t)(const struct device *dev, uint32_t result );
typedef void (*ft8xx_copro_cmd_flashspidesel_t)(const struct device *dev);
typedef void (*ft8xx_copro_cmd_flashspitx_t)(const struct device *dev, uint32_t num, unt8_t *data );
typedef void (*ft8xx_copro_cmd_flashspirx_t)(const struct device *dev, uint32_t ptr, uint32_t num );
typedef void (*ft8xx_copro_cmd_clearcache_t)(const struct device *dev);

//bt81x
typedef void (*ft8xx_copro_cmd_flashsource_t)(const struct device *dev, uint32_t ptr);
typedef void (*ft8xx_copro_cmd_videostartf_t)(const struct device *dev,);
typedef void (*ft8xx_copro_cmd_animstart_t)(const struct device *dev, int32_t ch, uint32_t aoptr, uint32_t loop);
typedef void (*ft8xx_copro_cmd_animstartram_t)(const struct device *dev, int32_t ch, uint32_t aoptr, uint32_t loop);
typedef void (*ft8xx_copro_cmd_animstop_t)(const struct device *dev, int32_t ch);
typedef void (*ft8xx_copro_cmd_animxy_t)(const struct device *dev, int32_t ch, int16_t x, int16_t y );
typedef void (*ft8xx_copro_cmd_animdraw_t)(const struct device *dev, int32_t ch);
typedef void (*ft8xx_copro_cmd_animframe_t)(const struct device *dev, int16_t x, int16_t y, uint32_t aoptr, uint32_t frame );
typedef void (*ft8xx_copro_cmd_animframeram_t)(const struct device *dev, int16_t x, int16_t y, uint32_t aoptr, uint32_t frame );


typedef void (*ft8xx_copro_cmd_sync_t)(const struct device *dev); //bt81x
typedef void (*ft8xx_copro_cmd_bitmap_transform_t)(const struct device *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t tx0, int32_t ty0, int32_t tx1, int32_t ty1, int32_t tx2, int32_t ty2, uint16_t result ); //bt81x
typedef void (*ft8xx_copro_cmd_testcard_t)(const struct device *dev); //bt817/8
typedef void (*ft8xx_copro_cmd_wait_t)(const struct device *dev, uint32_t us); //bt817/8
typedef void (*ft8xx_copro_cmd_fontcache_t)(const struct device *dev, uint32_t font, int32_t ptr, uint32_t num); //bt817/8
typedef void (*ft8xx_copro_cmd_fontcachequery_t)(const struct device *dev, uint32_t total, int32_t used);  //bt817/8
typedef void (*ft8xx_copro_cmd_hsf_t)(const struct device *dev, uint32_t w );  //bt817/8
typedef void (*ft8xx_copro_cmd_pclkfreq_t)(const struct device *dev, uint32_t ftarget, int32_t rounding, uint32_t factual ); //bt817/8






//  memory access api (equivelent to flash ram api)

typedef int (*ft8xx_ram_g_read_t)(const struct device *dev, off_t offset,void *data,size_t len);      
typedef int (*ft8xx_ram_g_write_t)(const struct device *dev, off_t offset,const void *data, size_t len);      
typedef int (*ft8xx_ram_g_erase_t)(const struct device *dev, off_t offset,size_t size);     
typedef const struct flash_parameters* (*ft8xx_ram_g_parameters_t)(const struct device *dev); 


struct ft8xx_api {

//  

    ft8xx_calibrate_t               ft8xx_calibrate,
    ft8xx_touch_transform_get_t     ft8xx_get_transform,
    ft8xx_touch_transform_set_t     ft8xx_set_transform,
	ft8xx_get_touch_tag_t			ft8xx_get_touch_tag,
	ft8xx_register_int_t			ft8xx_register_int,
    ft8xx_host_command_t            ft8xx_host_command,
    ft8xx_command_t                 ft8xx_command,

// audio functions

    ft8xx_audio_load_t              ft8xx_audio_load,
    ft8xx_audio_play_t              ft8xx_audio_play,
    ft8xx_audio_get_status_t        ft8xx_audio_get_status,
    ft8xx_audio_stop_t              ft8xx_audio_stop,

    ft8xx_audio_synth_start_t       ft8xx_audio_synth_start,
    ft8xx_audio_synth_get_status_t  ft8xx_audio_synth_get_status,
    ft8xx_audio_synth_stop_t        ft8xx_audio_synth_stop,


// coprocessor commands
// coprocessor command list functions (bt817/8)

    ft8xx_copro_cmd_newlist_t       ft8xx_copro_cmd_newlist, 
    ft8xx_copro_cmd_endlist_t       ft8xx_copro_cmd_endlist, 
    ft8xx_copro_cmd_calllist_t      ft8xx_copro_cmd_calllist, 
    ft8xx_copro_cmd_return_t        ft8xx_copro_cmd_return, 


// coprocessor displaylist functions

    ft8xx_copro_cmd_dlstart_t       ft8xx_copro_cmd_dlstart, 
    ft8xx_copro_cmd_swap_t          ft8xx_copro_cmd_swap,

// coprocessor graphics objects

    ft8xx_copro_cmd_text_t          ft8xx_copro_cmd_text,       //utf8 on bt81x
    ft8xx_copro_cmd_button_t        ft8xx_copro_cmd_button,     //utf8 on bt81x
    ft8xx_copro_cmd_clock_t         ft8xx_copro_cmd_clock,
    ft8xx_copro_cmd_bgcolor_t       ft8xx_copro_cmd_bgcolor,
    ft8xx_copro_cmd_fgcolor_t       ft8xx_copro_cmd_fgcolor,
    ft8xx_copro_cmd_gradcolor_t     ft8xx_copro_cmd_gradcolor,
    ft8xx_copro_cmd_gauge_t         ft8xx_copro_cmd_gauge,
    ft8xx_copro_cmd_gradient_t      ft8xx_copro_cmd_gradient,
    ft8xx_copro_cmd_gradienta_t     ft8xx_copro_cmd_gradienta   //ft81x
    ft8xx_copro_cmd_keys_t          ft8xx_copro_cmd_keys,
    ft8xx_copro_cmd_progress_t      ft8xx_copro_cmd_progress,
    ft8xx_copro_cmd_scrollbar_t     ft8xx_copro_cmd_scrollbar,
    ft8xx_copro_cmd_slider_t        ft8xx_copro_cmd_slider,
    ft8xx_copro_cmd_dial_t          ft8xx_copro_cmd_dial,
    ft8xx_copro_cmd_toggle_t        ft8xx_copro_cmd_toggle,     //utf8 on bt81x
    ft8xx_copro_cmd_number_t        ft8xx_copro_cmd_number,
    ft8xx_copro_cmd_fillwidth_t     ft8xx_copro_cmd_fillwidth,  //bt81x only
    ft8xx_copro_cmd_setbase_t       ft8xx_copro_cmd_setbase,    //ft81x
    ft8xx_copro_cmd_setscratch_t    ft8xx_copro_cmd_setscratch,
    ft8xx_copro_cmd_romfont_t       ft8xx_copro_cmd_romfont,
    ft8xx_copro_cmd_resetfonts_t    ft8xx_copro_cmd_resetfonts,
    ft8xx_copro_cmd_setbitmap_t     ft8xx_copro_cmd_setbitmap,

//  coprocessor RAM operation functions

    ft8xx_copro_cmd_memcrc_t        ft8xx_copro_cmd_memcrc,
    ft8xx_copro_cmd_memzero_t       ft8xx_copro_cmd_memzero,
    ft8xx_copro_cmd_memset_t        ft8xx_copro_cmd_memset,
    ft8xx_copro_cmd_memwrite_t      ft8xx_copro_cmd_memwrite,
    ft8xx_copro_cmd_memcpy_t        ft8xx_copro_cmd_memcpy,
    ft8xx_copro_cmd_append_t        ft8xx_copro_cmd_append,

//  coprocessor image/RAM_G loading functions

    ft8xx_copro_cmd_inflate_t       ft8xx_copro_cmd_inflate,
    ft8xx_copro_cmd_inflate2_t      ft8xx_copro_cmd_inflate2,   //bt81x only
    ft8xx_copro_cmd_loadimage_t     ft8xx_copro_cmd_loadimage,
    ft8xx_copro_cmd_mediafifo_t     ft8xx_copro_cmd_mediafifo,  //ft81x 
    ft8xx_copro_cmd_videoframe_t    ft8xx_copro_cmd_videoframe, //ft81x
    ft8xx_copro_cmd_getptr_t        ft8xx_copro_cmd_getptr,
    ft8xx_copro_cmd_getprops_t      ft8xx_copro_cmd_getprops,
    ft8xx_copro_cmd_getimage_t      ft8xx_copro_cmd_getimage,   //bt81x only

// coprocessor bitmap transform functions

    ft8xx_copro_cmd_loadidentity_t  ft8xx_copro_cmd_loadidentity,
    ft8xx_copro_cmd_translate_t     ft8xx_copro_cmd_translate,
    ft8xx_copro_cmd_scale_t         ft8xx_copro_cmd_scale,
    ft8xx_copro_cmd_rotate_t        ft8xx_copro_cmd_rotate,
    ft8xx_copro_cmd_rotatearound_t  ft8xx_copro_cmd_rotatearound,
    ft8xx_copro_cmd_setmatrix_t     ft8xx_copro_cmd_setmatrix,
    ft8xx_copro_cmd_getmatrix_t     ft8xx_copro_cmd_getmatrix,

// coprocessor other functions

    ft8xx_copro_cmd_coldstart_t     ft8xx_copro_cmd_coldstart,
    ft8xx_copro_cmd_interrupt_t     ft8xx_copro_cmd_interrupt,
    ft8xx_copro_cmd_regread_t       ft8xx_copro_cmd_regread,
    ft8xx_copro_cmd_calibrate_t     ft8xx_copro_cmd_calibrate,
    ft8xx_copro_cmd_calibratesub_t  ft8xx_copro_cmd_calibratesub,
    ft8xx_copro_cmd_spinner_t       ft8xx_copro_cmd_spinner,
    ft8xx_copro_cmd_screensaver_t   ft8xx_copro_cmd_screensaver,
    ft8xx_copro_cmd_sketch_t        ft8xx_copro_cmd_sketch,
    ft8xx_copro_cmd_stop_t          ft8xx_copro_cmd_stop,
    ft8xx_copro_cmd_snapshot_t      ft8xx_copro_cmd_snapshot,
    ft8xx_copro_cmd_snapshot2_t     ft8xx_copro_cmd_snapshot2,
    ft8xx_copro_cmd_logo_t          ft8xx_copro_cmd_logo,
    ft8xx_copro_cmd_setfont_t       ft8xx_copro_cmd_setfont,
    ft8xx_copro_cmd_setfont2_t      ft8xx_copro_cmd_setfont2,
    ft8xx_copro_cmd_track_t         ft8xx_copro_cmd_track,
    ft8xx_copro_cmd_setrotate_t     ft8xx_copro_cmd_setrotate,

    ft8xx_copro_cmd_sync_t              ft8xx_copro_cmd_sync,            //bt81x
    ft8xx_copro_cmd_bitmap_transform_t  ft8xx_copro_cmd_bitmap_transform //bt81x
    ft8xx_copro_cmd_testcard_t          ft8xx_copro_cmd_testcard,        //bt817/8 only
    ft8xx_copro_cmd_wait_t              ft8xx_copro_cmd_wait,            //bt817/8 only
    ft8xx_copro_cmd_apilevel_t          ft8xx_copro_cmd_apilevel,        //bt817/8 only
    ft8xx_copro_cmd_fontcache_t         ft8xx_copro_cmd_fontcache,       //bt817/8 only  
    ft8xx_copro_cmd_fontcachequery_t    ft8xx_copro_cmd_fontcachequery,  //bt817/8 only
    ft8xx_copro_cmd_hsf_t               ft8xx_copro_cmd_hsf,             //bt817/8 only
    ft8xx_copro_cmd_pclkfreq_t          ft8xx_copro_cmd_pclkfreq,        //bt817/8 only

// flash operation (bt81x)

    ft8xx_copro_cmd_flasherase_t	ft8xx_copro_cmd_flasherase,
    ft8xx_copro_cmd_flashwrite_t	ft8xx_copro_cmd_flashwrite,
    ft8xx_copro_cmd_flashwprogram_t	ft8xx_copro_cmd_flashprogram,
    ft8xx_copro_cmd_flashread_t	    ft8xx_copro_cmd_flashread,
    ft8xx_copro_cmd_appendf_t	    ft8xx_copro_cmd_appendf,
    ft8xx_copro_cmd_flashupdate_t	ft8xx_copro_cmd_flashupdate,
    ft8xx_copro_cmd_flashdetach_t	ft8xx_copro_cmd_flashdetach,
    ft8xx_copro_cmd_flashattach_t	ft8xx_copro_cmd_flashattach,
    ft8xx_copro_cmd_flashfast_t	    ft8xx_copro_cmd_flashfast,
    ft8xx_copro_cmd_flashspidesel_t	ft8xx_copro_cmd_flashspidesel,
    ft8xx_copro_cmd_flashspitx_t	ft8xx_copro_cmd_flashtx,
    ft8xx_copro_cmd_flashspirx_t	ft8xx_copro_cmd_flashrx,
    ft8xx_copro_cmd_clearcache_t	ft8xx_copro_cmd_clearcache,

// video playback 
    ft8xx_copro_cmd_videostart_t	ft8xx_copro_cmd_videostart, //ft81x
    ft8xx_copro_cmd_flashsource_t	ft8xx_copro_cmd_flashsource, //bt81x
    ft8xx_copro_cmd_videostartf_t	ft8xx_copro_cmd_videostartf, //bt81x
    ft8xx_copro_cmd_videoframe_t	ft8xx_copro_cmd_videoframe, //ft81x
    ft8xx_copro_cmd_playvideo_t	    ft8xx_copro_cmd_playvideo,	//ft81x
    ft8xx_copro_cmd_animframe_t	    ft8xx_copro_cmd_animframe,  //bt81x
    ft8xx_copro_cmd_animframeram_t	ft8xx_copro_cmd_animframeram, //bt81x
    ft8xx_copro_cmd_animstart_t	    ft8xx_copro_cmd_animstart, //bt81x
    ft8xx_copro_cmd_animstop_t	    ft8xx_copro_cmd_animstop, //bt81x
    ft8xx_copro_cmd_animxy_t	    ft8xx_copro_cmd_animxy, //bt81x
    ft8xx_copro_cmd_animdraw_t	    ft8xx_copro_cmd_animdraw, //bt81x


//  memory access api (equivelent to flash ram api)

	ft8xx_ram_g_read_t              read,
	ft8xx_ram_g_write_t             write,
	ft8xx_ram_g_erase_t             erase,
	ft8xx_ram_g_parameters_t        get_parameters,





}



/**
 * @brief Calibrate touchscreen
 *
 * Run touchscreen calibration procedure that collects three touches from touch
 * screen. Computed calibration result is automatically applied to the
 * touchscreen processing and returned with @p trform.
 *
 * The content of @p trform may be stored and used after reset in
 * ft8xx_touch_transform_set() to avoid calibrating touchscreen after each
 * device reset.
 *
 * @param data Pointer to touchscreen transform structure to populate
 */
void ft8xx_calibrate(const struct device *dev, struct ft8xx_touch_transform *trform);

/**
 * @brief Set touchscreen calibration data
 *
 * Apply given touchscreen transform data to the touchscreen processing.
 * Data is to be obtained from calibration procedure started with
 * ft8xx_calibrate().
 *
 * @param trform Pointer to touchscreen transform structure to apply
 */
void ft8xx_touch_transform_set(const struct device *dev, const struct ft8xx_touch_transform *trform);

/**
 * @brief Get tag of recently touched element
 *
 * @return Tag value 0-255 of recently touched element
 */
int ft8xx_get_touch_tag(const struct device *dev);

/**
 * @brief Set callback executed when FT8xx triggers interrupt
 *
 * This function configures FT8xx to trigger interrupt when touch event changes
 * tag value.
 *
 * When touch event changes tag value, FT8xx activates INT line. The line is
 * kept active until ft8xx_get_touch_tag() is called. It results in single
 * execution of @p callback until ft8xx_get_touch_tag() is called.
 *
 * @param callback Pointer to function called when FT8xx triggers interrupt
 */
void ft8xx_register_int(const struct device *dev, ft8xx_int_callback callback);









/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_API_H_ */
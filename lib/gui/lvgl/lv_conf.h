/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_GUI_LVGL_LV_CONF_H_
#define ZEPHYR_LIB_GUI_LVGL_LV_CONF_H_

/* Graphical settings */

#define LV_HOR_RES_MAX	CONFIG_LVGL_HOR_RES
#define LV_VER_RES_MAX	CONFIG_LVGL_VER_RES

#ifdef CONFIG_LVGL_COLOR_DEPTH_32
#define LV_COLOR_DEPTH	32
#elif defined(CONFIG_LVGL_COLOR_DEPTH_16)
#define LV_COLOR_DEPTH	16
#elif defined(CONFIG_LVGL_COLOR_DEPTH_8)
#define LV_COLOR_DEPTH	8
#elif defined(CONFIG_LVGL_COLOR_DEPTH_1)
#define LV_COLOR_DEPTH	1
#endif

#ifdef CONFIG_LVGL_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#else
#define LV_COLOR_16_SWAP 0
#endif

#ifdef CONFIG_LVGL_COLOR_SCREEN_TRANSP
#define LV_COLOR_SCREEN_TRANSP 1
#else
#define LV_COLOR_SCREEN_TRANSP 0
#endif

#ifdef CONFIG_LVGL_CHROMA_KEY_RED
#define LV_COLOR_TRANSP LV_COLOR_RED
#elif defined(CONFIG_LVGL_CHROMA_KEY_GREEN)
#define LV_COLOR_TRANSP LV_COLOR_LIME
#elif defined(CONFIG_LVGL_CHROMA_KEY_BLUE)
#define LV_COLOR_TRANSP LV_COLOR_BLUE
#elif defined(CONFIG_LVGL_CHROMA_KEY_CUSTOM)
#define LV_COLOR_TRANSP LV_COLOR_MAKE(CONFIG_LVGL_CUSTOM_CHROMA_KEY_RED, \
		CONFIG_LVGL_CUSTOM_CHROMA_KEY_GREEN, \
		CONFIG_LVGL_CUSTOM_CHROMA_KEY_BLUE)
#endif

#ifdef CONFIG_LVGL_ANTIALIAS
#define LV_ANTIALIAS 1
#else
#define LV_ANTIALIAS 0
#endif

#define LV_DISP_DEF_REFR_PERIOD	CONFIG_LVGL_SCREEN_REFRESH_PERIOD

#define LV_DPI CONFIG_LVGL_DPI

typedef short lv_coord_t;

/* Memory manager settings */

#define LV_MEM_CUSTOM 1

#ifdef CONFIG_LVGL_MEM_POOL_HEAP_KERNEL

#define LV_MEM_CUSTOM_INCLUDE	"kernel.h"
#define LV_MEM_CUSTOM_ALLOC	k_malloc
#define LV_MEM_CUSTOM_FREE	k_free

#elif defined(CONFIG_LVGL_MEM_POOL_HEAP_LIB_C)

#define LV_MEM_CUSTOM_INCLUDE	"stdlib.h"
#define LV_MEM_CUSTOM_ALLOC	malloc
#define LV_MEM_CUSTOM_FREE	free

#else

#define LV_MEM_CUSTOM_INCLUDE	"lvgl_mem.h"
#define LV_MEM_CUSTOM_ALLOC	lvgl_malloc
#define LV_MEM_CUSTOM_FREE	lvgl_free

#endif

#define LV_ENABLE_GC 0

/* Input device settings */

#define LV_INDEV_DEF_READ_PERIOD CONFIG_LVGL_INPUT_REFRESH_PERIOD

#define LV_INDEV_DEF_DRAG_LIMIT CONFIG_LVGL_INPUT_DRAG_THRESHOLD

#define LV_INDEV_DEF_DRAG_THROW CONFIG_LVGL_INPUT_DRAG_THROW_SLOW_DOWN

#define LV_INDEV_DEF_LONG_PRESS_TIME CONFIG_LVGL_INPUT_LONG_PRESS_TIME

#define LV_INDEV_DEF_LONG_PRESS_REP_TIME \
	CONFIG_LVGL_INPUT_LONG_RESS_REPEAT_TIME

/* Feature usage */

#ifdef CONFIG_LVGL_ANIMATION
#define LV_USE_ANIMATION 1
#else
#define LV_USE_ANIMATION 0
#endif

#if LV_USE_ANIMATION
typedef void *lv_anim_user_data_t;
#endif

#ifdef CONFIG_LVGL_SHADOW
#define LV_USE_SHADOW 1
#else
#define LV_USE_SHADOW 0
#endif

#ifdef CONFIG_LVGL_GROUP
#define LV_USE_GROUP 1
#else
#define LV_USE_GROUP 0
#endif

#if LV_USE_GROUP
typedef void *lv_group_user_data_t;
#endif

#ifdef CONFIG_LVGL_GPU
#define LV_USE_GPU 1
#else
#define LV_USE_GPU 0
#endif

#ifdef CONFIG_LVGL_FILESYSTEM
#define LV_USE_FILESYSTEM 1
#else
#define LV_USE_FILESYSTEM 0
#endif

#if LV_USE_FILESYSTEM
typedef void *lv_fs_drv_user_data_t;
#endif

#define LV_USE_USER_DATA 1

/* Image decoder and cache */

#ifdef CONFIG_LVGL_IMG_CF_INDEXED
#define LV_IMG_CF_INDEXED 1
#else
#define LV_IMG_CF_INDEXED 0
#endif

#ifdef CONFIG_LVGL_IMG_CF_ALPHA
#define LV_IMG_CF_ALPHA 1
#else
#define LV_IMG_CF_ALPHA 0
#endif

#define LV_IMG_CACHE_DEF_SIZE CONFIG_LVGL_IMG_CACHE_DEF_SIZE

typedef void *lv_img_decoder_user_data_t;

/* Compiler settings */

#define LV_ATTRIBUTE_TICK_INC

#define LV_ATTRIBUTE_TASK_HANDLER

#define LV_ATTRIBUTE_MEM_ALIGN

#define LV_ATTRIBUTE_LARGE_CONST

/* HAL settings */

#define LV_TICK_CUSTOM			1
#define LV_TICK_CUSTOM_INCLUDE		"kernel.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR	(k_uptime_get_32())

typedef void *lv_disp_drv_user_data_t;
typedef void *lv_indev_drv_user_data_t;

/* Log settings */

#if CONFIG_LVGL_LOG_LEVEL == 0
#define LV_USE_LOG 0
#else
#define LV_USE_LOG 1

#if CONFIG_LVGL_LOG_LEVEL == 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_ERROR
#elif CONFIG_LVGL_LOG_LEVEL == 2
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#elif CONFIG_LVGL_LOG_LEVEL == 3
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
#elif CONFIG_LVGL_LOG_LEVEL == 4
#define LV_LOG_LEVEL LV_LOG_LEVEL_TRACE
#endif

#define LV_LOG_PRINTF 0
#endif

/* THEME USAGE */

#ifdef CONFIG_LVGL_THEMES

#define LV_THEME_LIVE_UPDATE	CONFIG_LVGL_THEME_LIVE_UPDATE

#define LV_USE_THEME_TEMPL 0

#ifdef CONFIG_LVGL_THEME_DEFAULT
#define LV_USE_THEME_DEFAULT 1
#else
#define LV_USE_THEME_DEFAULT 0
#endif

#ifdef CONFIG_LVGL_THEME_ALIEN
#define LV_USE_THEME_ALIEN 1
#else
#define LV_USE_THEME_ALIEN 0
#endif

#ifdef CONFIG_LVGL_THEME_NIGHT
#define LV_USE_THEME_NIGHT 1
#else
#define LV_USE_THEME_NIGHT 0
#endif

#ifdef CONFIG_LVGL_THEME_MONO
#define LV_USE_THEME_MONO 1
#else
#define LV_USE_THEME_MONO 0
#endif

#ifdef CONFIG_LVGL_THEME_MATERIAL
#define LV_USE_THEME_MATERIAL 1
#else
#define LV_USE_THEME_MATERIAL 0
#endif

#ifdef CONFIG_LVGL_THEME_ZEN
#define LV_USE_THEME_ZEN 1
#else
#define LV_USE_THEME_ZEN 0
#endif

#ifdef CONFIG_LVGL_THEME_NEMO
#define LV_USE_THEME_NEMO 1
#else
#define LV_USE_THEME_NEMO 0
#endif

#else

#define LV_THEME_LIVE_UPDATE 0

#define LV_USE_THEME_TEMPL 0
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_ALIEN 0
#define LV_USE_THEME_NIGHT 0
#define LV_USE_THEME_MONO 0
#define LV_USE_THEME_MATERIAL 0
#define LV_USE_THEME_ZEN 0
#define LV_USE_THEME_NEMO 0

#endif

/* FONT USAGE */

#ifdef CONFIG_LVGL_BUILD_IN_FONT_ROBOTO_12
#define LV_FONT_ROBOTO_12 1
#else
#define LV_FONT_ROBOTO_12 0
#endif

#ifdef CONFIG_LVGL_BUILD_IN_FONT_ROBOTO_16
#define LV_FONT_ROBOTO_16 1
#else
#define LV_FONT_ROBOTO_16 0
#endif

#ifdef CONFIG_LVGL_BUILD_IN_FONT_ROBOTO_22
#define LV_FONT_ROBOTO_22 1
#else
#define LV_FONT_ROBOTO_22 0
#endif

#ifdef CONFIG_LVGL_BUILD_IN_FONT_ROBOTO_28
#define LV_FONT_ROBOTO_28 1
#else
#define LV_FONT_ROBOTO_28 0
#endif

#ifdef CONFIG_LVGL_BUILD_IN_FONT_UNSCII_8
#define LV_FONT_UNSCII_8 1
#else
#define LV_FONT_UNSCII_8 0
#endif

#define LV_FONT_CUSTOM_DECLARE

#ifdef CONFIG_LVGL_DEFAULT_FONT_BUILD_IN_ROBOTO_12
#define LV_FONT_DEFAULT		(&lv_font_roboto_12)
#elif defined(CONFIG_LVGL_DEFAULT_FONT_BUILD_IN_ROBOTO_16)
#define LV_FONT_DEFAULT		(&lv_font_roboto_16)
#elif defined(CONFIG_LVGL_DEFAULT_FONT_BUILD_IN_ROBOTO_22)
#define LV_FONT_DEFAULT		(&lv_font_roboto_22)
#elif defined(CONFIG_LVGL_DEFAULT_FONT_BUILD_IN_ROBOTO_28)
#define LV_FONT_DEFAULT		(&lv_font_roboto_28)
#elif defined(CONFIG_LVGL_DEFAULT_FONT_BUILD_IN_UNSCII_8)
#define LV_FONT_DEFAULT		(&lv_font_unscii_8)
#elif defined(CONFIG_LVGL_DEFAULT_FONT_CUSTOM)
extern void *lv_default_font_custom_ptr;
#define LV_FONT_DEFAULT ((lv_font_t *) lv_default_font_custom_ptr)
#endif

typedef void *lv_font_user_data_t;

/* Text settings */

#ifdef CONFIG_LVGL_TXT_ENC_ASCII
#define LV_TXT_ENC LV_TXT_ENC_ASCII
#elif defined(CONFIG_LVGL_TXT_ENC_UTF8)
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#endif

#define LV_TXT_BREAK_CHARS	CONFIG_LVGL_TEXT_BREAK_CHARACTERS

/* LV_OBJ SETTINGS */

typedef void *lv_obj_user_data_t;

#ifdef CONFIG_LVGL_OBJ_REALIGN
#define LV_USE_OBJ_REALIGN 1
#else
#define LV_USE_OBJ_REALIGN 0
#endif

#if defined(CONFIG_LVGL_EXT_CLICK_AREA_OFF)
#define LV_USE_EXT_CLICK_AREA  LV_EXT_CLICK_AREA_OFF
#elif defined(CONFIG_LVGL_EXT_CLICK_AREA_TINY)
#define LV_USE_EXT_CLICK_AREA  LV_EXT_CLICK_AREA_TINY
#elif defined(CONFIG_LVGL_EXT_CLICK_AREA_FULL)
#define LV_USE_EXT_CLICK_AREA  LV_EXT_CLICK_AREA_FULL
#endif

/* LV OBJ X USAGE */

#ifdef CONFIG_LVGL_OBJ_ARC
#define LV_USE_ARC 1
#else
#define LV_USE_ARC 0
#endif

#ifdef CONFIG_LVGL_OBJ_BAR
#define LV_USE_BAR 1
#else
#define LV_USE_BAR 0
#endif

#ifdef CONFIG_LVGL_OBJ_BUTTON
#define LV_USE_BTN 1
#else
#define LV_USE_BTN 0
#endif

#if LV_USE_BTN != 0

#ifdef CONFIG_LVGL_OBJ_BUTTON_INK_EFFECT
#define LV_BTN_INK_EFFECT 1
#else
#define LV_BTN_INK_EFFECT 0
#endif

#endif

#ifdef CONFIG_LVGL_OBJ_BUTTON_MATRIX
#define LV_USE_BTNM 1
#else
#define LV_USE_BTNM 0
#endif

#ifdef CONFIG_LVGL_OBJ_CALENDAR
#define LV_USE_CALENDAR 1
#else
#define LV_USE_CALENDAR 0
#endif

#ifdef CONFIG_LVGL_OBJ_CANVAS
#define LV_USE_CANVAS 1
#else
#define LV_USE_CANVAS 0
#endif

#ifdef CONFIG_LVGL_OBJ_CHECK_BOX
#define LV_USE_CB 1
#else
#define LV_USE_CB 0
#endif

#ifdef CONFIG_LVGL_OBJ_CHART
#define LV_USE_CHART 1
#else
#define LV_USE_CHART 0
#endif
#if LV_USE_CHART
#define LV_CHART_AXIS_TICK_LABEL_MAX_LEN \
	CONFIG_LVGL_OBJ_CHART_AXIS_TICK_LABEL_MAX_LEN
#endif

#ifdef CONFIG_LVGL_OBJ_CONTAINER
#define LV_USE_CONT 1
#else
#define LV_USE_CONT 0
#endif

#ifdef CONFIG_LVGL_OBJ_DROP_DOWN_LIST
#define LV_USE_DDLIST 1
#else
#define LV_USE_DDLIST 0
#endif

#if LV_USE_DDLIST != 0
#define LV_DDLIST_DEF_ANIM_TIME	CONFIG_LVGL_OBJ_DROP_DOWN_LIST_ANIM_TIME
#endif

#ifdef CONFIG_LVGL_OBJ_GAUGE
#define LV_USE_GAUGE 1
#else
#define LV_USE_GAUGE 0
#endif

#ifdef CONFIG_LVGL_OBJ_IMAGE
#define LV_USE_IMG 1
#else
#define LV_USE_IMG 0
#endif

#ifdef CONFIG_LVGL_OBJ_IMG_BUTTON
#define LV_USE_IMGBTN 1
#else
#define LV_USE_IMGBTN 0
#endif

#if LV_USE_IMGBTN
#ifdef CONFIG_LVGL_OBJ_IMG_BUTTON_TILED
#define LV_IMGBTN_TILED 1
#else
#define LV_IMGBTN_TILED 0
#endif
#endif

#ifdef CONFIG_LVGL_OBJ_KEYBOARD
#define LV_USE_KB 1
#else
#define LV_USE_KB 0
#endif

#ifdef CONFIG_LVGL_OBJ_LABEL
#define LV_USE_LABEL 1
#else
#define LV_USE_LABEL 0
#endif

#if LV_USE_LABEL != 0
#define LV_LABEL_DEF_SCROLL_SPEED CONFIG_LVGL_OBJ_LABEL_SCROLL_SPEED
#define LV_LABEL_WAIT_CHAR_COUNT \
	CONFIG_LVGL_OBJ_LABEL_WAIT_CHAR_COUNT

#ifdef CONFIG_LVGL_OBJ_LABEL_TEXT_SEL
#define LV_LABEL_TEXT_SEL 1
#else
#define LV_LABEL_TEXT_SEL 0
#endif

#ifdef CONFIG_LVGL_OBJ_LABEL_LONG_TXT_HINT
#define LV_LABEL_LONG_TXT_HINT 1
#else
#define LV_LABEL_LONG_TXT_HINT 0
#endif

#endif

#ifdef CONFIG_LVGL_OBJ_LED
#define LV_USE_LED 1
#else
#define LV_USE_LED 0
#endif

#ifdef CONFIG_LVGL_OBJ_LINE
#define LV_USE_LINE 1
#else
#define LV_USE_LINE 0
#endif

#ifdef CONFIG_LVGL_OBJ_LIST
#define LV_USE_LIST 1
#else
#define LV_USE_LIST 0
#endif

#if LV_USE_LIST != 0
#define LV_LIST_DEF_ANIM_TIME CONFIG_LVGL_OBJ_LIST_FOCUS_TIME
#endif

#ifdef CONFIG_LVGL_OBJ_LINE_METER
#define LV_USE_LMETER 1
#else
#define LV_USE_LMETER 0
#endif

#ifdef CONFIG_LVGL_OBJ_MSG_BOX
#define LV_USE_MBOX 1
#else
#define LV_USE_MBOX 0
#endif

#ifdef CONFIG_LVGL_OBJ_PAGE
#define LV_USE_PAGE 1
#else
#define LV_USE_PAGE 0
#endif

#if LV_USE_PAGE != 0
#define LV_PAGE_DEF_ANIM_TIME CONFIG_LVGL_OBJ_PAGE_DEF_ANIM_TIME
#endif

#ifdef CONFIG_LVGL_OBJ_PRELOAD
#define LV_USE_PRELOAD 1
#else
#define LV_USE_PRELOAD 0
#endif

#if LV_USE_PRELOAD != 0
#define LV_PRELOAD_DEF_ARC_LENGTH CONFIG_LVGL_OBJ_PRELOAD_DEF_ARC_LENGTH
#define LV_PRELOAD_DEF_SPIN_TIME CONFIG_LVGL_OBJ_PRELOAD_DEF_SPIN_TIME
#ifdef LVGL_OBJ_PRELOAD_DEF_ANIMATION_SPIN_ARC
#define LV_PRELOAD_DEF_ANIM LV_PRELOAD_TYPE_SPINNING_ARC
#endif
#ifdef LVGL_OBJ_PRELOAD_DEF_ANIMATION_FILL
#define LV_PRELOAD_DEF_ANIM LV_PRELOAD_TYPE_FILLSPIN_ARC
#endif
#endif

#ifdef CONFIG_LVGL_OBJ_ROLLER
#define LV_USE_ROLLER 1
#else
#define LV_USE_ROLLER 0
#endif

#if LV_USE_ROLLER != 0
#define LV_ROLLER_DEF_ANIM_TIME	CONFIG_LVGL_OBJ_ROLLER_ANIM_TIME
#define LV_ROLLER_INF_PAGES	CONFIG_LVGL_OBJ_ROLLER_INF_PAGES
#endif

#ifdef CONFIG_LVGL_OBJ_SLIDER
#define LV_USE_SLIDER 1
#else
#define LV_USE_SLIDER 0
#endif

#ifdef CONFIG_LVGL_OBJ_SPINBOX
#define LV_USE_SPINBOX 1
#else
#define LV_USE_SPINBOX 0
#endif

#ifdef CONFIG_LVGL_OBJ_SWITCH
#define LV_USE_SW 1
#else
#define LV_USE_SW 0
#endif

#ifdef CONFIG_LVGL_OBJ_TEXT_AREA
#define LV_USE_TA 1
#else
#define LV_USE_TA 0
#endif

#if LV_USE_TA != 0
#define LV_TA_DEF_CURSOR_BLINK_TIME \
	CONFIG_LVGL_OBJ_TEXT_AREA_CURSOR_BLINK_TIME
#define LV_TA_DEF_PWD_SHOW_TIME \
	CONFIG_LVGL_OBJ_TEXT_AREA_PWD_SHOW_TIME
#endif

#ifdef CONFIG_LVGL_OBJ_TABLE
#define LV_USE_TABLE 1
#else
#define LV_USE_TABLE 0
#endif

#if LV_USE_TABLE
#define LV_TABLE_COL_MAX CONFIG_LVGL_OBJ_TABLE_COLUMN_MAX
#endif

#ifdef CONFIG_LVGL_OBJ_TAB_VIEW
#define LV_USE_TABVIEW 1
#else
#define LV_USE_TABVIEW 0
#endif

#if LV_USE_TABVIEW != 0
#define LV_TABVIEW_DEF_ANIM_TIME CONFIG_LVGL_OBJ_TAB_VIEW_ANIMATION_TIME
#endif

#ifdef CONFIG_LVGL_OBJ_TILE_VIEW
#define LV_USE_TILEVIEW 1
#else
#define LV_USE_TILEVIEW 0
#endif

#if LV_USE_TILEVIEW
#define LV_TILEVIEW_DEF_ANIM_TIME CONFIG_LVGL_OBJ_TILE_VIEW_ANIMATION_TIME
#endif

#ifdef CONFIG_LVGL_OBJ_WINDOW
#define LV_USE_WIN 1
#else
#define LV_USE_WIN 0
#endif

#endif /* ZEPHYR_LIB_GUI_LVGL_LV_CONF_H_ */

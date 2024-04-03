/*
 * Copyright (c) 2018-2020 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_LV_CONF_H_
#define ZEPHYR_MODULES_LVGL_LV_CONF_H_

/* Memory manager settings */

#define LV_USE_STDLIB_MALLOC LV_STDLIB_CUSTOM

#if defined(CONFIG_LV_Z_MEM_POOL_HEAP_LIB_C)
#define LV_STDLIB_INCLUDE "stdlib.h"
#define lv_malloc_core    malloc
#define lv_realloc_core   realloc
#define lv_free_core      free
#else
#define LV_STDLIB_INCLUDE "lvgl_mem.h"
#define lv_malloc_core    lvgl_malloc
#define lv_realloc_core   lvgl_realloc
#define lv_free_core      lvgl_free
#endif

/* Misc settings */
#define lv_snprintf  snprintf
#define lv_vsnprintf vsnprintf

#if CONFIG_LV_COLOR_DEPTH == 1
#define LV_COLOR_FORMAT_NATIVE LV_COLOR_FORMAT_RAW
#endif /* CONFIG_LV_COLOR_DEPTH == 1 */

/*
 * Needed because of a workaround for a GCC bug,
 * see https://github.com/lvgl/lvgl/issues/3078
 */
#define LV_CONF_SUPPRESS_DEFINE_CHECK 1

#endif /* ZEPHYR_MODULES_LVGL_LV_CONF_H_ */

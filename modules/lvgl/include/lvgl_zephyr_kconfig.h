/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_LVGL_ZEPHYR_KCONFIG_H_
#define ZEPHYR_MODULES_LVGL_LVGL_ZEPHYR_KCONFIG_H_

#include <zephyr/autoconf.h>
#include <zephyr/toolchain.h>

/*
 * Mapping of the Zephyr LVGL Kconfig options to the configuration macros
 * used by LVGL. lv_conf_internal.h includes lv_conf.h, and therefore this
 * file, before it evaluates the options of the LVGL module Kconfig, so the
 * definitions below take precedence over those.
 */

/*
 * Default color format. LV_COLOR_DEPTH is derived from the color format of
 * the LVGL module Kconfig otherwise, so it is defined here as well. The
 * UNSCII 8 font is enabled for the I1 and L8 formats, matching the implied
 * default of the LVGL module Kconfig for those formats.
 */
#if defined(CONFIG_LV_Z_COLOR_FORMAT_I1)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_I1
#define LV_COLOR_DEPTH          1
#define LV_FONT_UNSCII_8        1
#elif defined(CONFIG_LV_Z_COLOR_FORMAT_L8)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_L8
#define LV_COLOR_DEPTH          8
#define LV_FONT_UNSCII_8        1
#elif defined(CONFIG_LV_Z_COLOR_FORMAT_RGB565)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565
#define LV_COLOR_DEPTH          16
#elif defined(CONFIG_LV_Z_COLOR_FORMAT_RGB565_SWAPPED)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565_SWAPPED
#define LV_COLOR_DEPTH          16
#elif defined(CONFIG_LV_Z_COLOR_FORMAT_RGB888)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB888
#define LV_COLOR_DEPTH          24
#elif defined(CONFIG_LV_Z_COLOR_FORMAT_XRGB8888)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_XRGB8888
#define LV_COLOR_DEPTH          32
#elif defined(CONFIG_LV_Z_COLOR_FORMAT_ARGB8888)
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_ARGB8888
#define LV_COLOR_DEPTH          32
#endif

/*
 * The default color format option of the LVGL module Kconfig has no effect.
 * Reject configurations selecting a non-default format there that differs
 * from the Zephyr option, as the static render buffers are sized for the
 * format selected by CONFIG_LV_Z_COLOR_FORMAT_*.
 */
#if (defined(CONFIG_LV_COLOR_FORMAT_I1) && !defined(CONFIG_LV_Z_COLOR_FORMAT_I1)) ||             \
	(defined(CONFIG_LV_COLOR_FORMAT_L8) && !defined(CONFIG_LV_Z_COLOR_FORMAT_L8)) ||         \
	(defined(CONFIG_LV_COLOR_FORMAT_RGB565_SWAPPED) &&                                        \
	 !defined(CONFIG_LV_Z_COLOR_FORMAT_RGB565_SWAPPED)) ||                                    \
	(defined(CONFIG_LV_COLOR_FORMAT_RGB888) && !defined(CONFIG_LV_Z_COLOR_FORMAT_RGB888)) || \
	(defined(CONFIG_LV_COLOR_FORMAT_XRGB8888) &&                                              \
	 !defined(CONFIG_LV_Z_COLOR_FORMAT_XRGB8888)) ||                                          \
	(defined(CONFIG_LV_COLOR_FORMAT_ARGB8888) &&                                              \
	 !defined(CONFIG_LV_Z_COLOR_FORMAT_ARGB8888)) ||                                          \
	defined(CONFIG_LV_COLOR_FORMAT_ARGB8888_PREMULTIPLIED)
#error "CONFIG_LV_COLOR_FORMAT_* has no effect, use CONFIG_LV_Z_COLOR_FORMAT_* instead"
#endif

/* Provide definition to align LVGL buffers */
#define LV_ATTRIBUTE_MEM_ALIGN __aligned(CONFIG_LV_ATTRIBUTE_MEM_ALIGN_SIZE)

#define LV_COLOR_16_SWAP_DISABLE_WARNING 1
#ifdef CONFIG_LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#endif /* CONFIG_LV_COLOR_16_SWAP */

#ifdef CONFIG_LV_Z_USE_OSAL
#define LV_USE_OS            LV_OS_CUSTOM
#define LV_OS_CUSTOM_INCLUDE "lvgl_zephyr_osal.h"
#endif /* CONFIG_LV_Z_USE_OSAL */

#endif /* ZEPHYR_MODULES_LVGL_LVGL_ZEPHYR_KCONFIG_H_ */

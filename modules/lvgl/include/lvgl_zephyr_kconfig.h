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

#define LV_DPI_DEF CONFIG_LV_Z_DPI_DEF
#define LV_DRAW_BUF_ALIGN CONFIG_LV_Z_DRAW_BUF_ALIGN

#if defined(CONFIG_LV_USE_DRAW_DMA2D) && !defined(CONFIG_LV_Z_DRAW_DMA2D)
#error "CONFIG_LV_USE_DRAW_DMA2D has no effect; use CONFIG_LV_Z_DRAW_DMA2D instead"
#endif
#if defined(CONFIG_LV_USE_DRAW_DAVE2D) && !defined(CONFIG_LV_Z_DRAW_DAVE2D)
#error "CONFIG_LV_USE_DRAW_DAVE2D has no effect; use CONFIG_LV_Z_DRAW_DAVE2D instead"
#endif
#if defined(CONFIG_LV_USE_DRAW_PXP) && !defined(CONFIG_LV_Z_DRAW_PXP)
#error "CONFIG_LV_USE_DRAW_PXP has no effect; use CONFIG_LV_Z_DRAW_PXP instead"
#endif
#if defined(CONFIG_LV_USE_NEMA_GFX) && !defined(CONFIG_LV_Z_USE_NEMA_GFX)
#error "CONFIG_LV_USE_NEMA_GFX has no effect; use CONFIG_LV_Z_USE_NEMA_GFX instead"
#endif
#if defined(CONFIG_LV_USE_NEMA_VG) && !defined(CONFIG_LV_Z_USE_NEMA_VG)
#error "CONFIG_LV_USE_NEMA_VG has no effect; use CONFIG_LV_Z_USE_NEMA_VG instead"
#endif

#ifdef CONFIG_LV_Z_DRAW_DMA2D
#define LV_USE_DRAW_DMA2D 1
#else
#define LV_USE_DRAW_DMA2D 0
#endif
#ifdef CONFIG_LV_Z_DRAW_DMA2D_INTERRUPT
#define LV_USE_DRAW_DMA2D_INTERRUPT 1
#else
#define LV_USE_DRAW_DMA2D_INTERRUPT 0
#endif
#ifdef CONFIG_LV_Z_DRAW_DMA2D_HAL_INCLUDE
#define LV_DRAW_DMA2D_HAL_INCLUDE CONFIG_LV_Z_DRAW_DMA2D_HAL_INCLUDE
#else
#define LV_DRAW_DMA2D_HAL_INCLUDE "stm32h7xx_hal.h"
#endif
#ifdef CONFIG_LV_Z_DRAW_DAVE2D
#define LV_USE_DRAW_DAVE2D 1
#else
#define LV_USE_DRAW_DAVE2D 0
#endif
#ifdef CONFIG_LV_Z_DRAW_PXP
#define LV_USE_DRAW_PXP 1
#else
#define LV_USE_DRAW_PXP 0
#endif

#ifdef CONFIG_LV_Z_USE_NEMA_GFX
#define LV_USE_NEMA_GFX 1
#else
#define LV_USE_NEMA_GFX 0
#endif
#ifdef CONFIG_LV_Z_NEMA_USE_CACHE
#define LV_NEMA_USE_CACHE 1
#else
#define LV_NEMA_USE_CACHE 0
#endif
#ifdef CONFIG_LV_Z_NEMA_CACHE_HAL_INCLUDE
#define LV_NEMA_CACHE_HAL_INCLUDE CONFIG_LV_Z_NEMA_CACHE_HAL_INCLUDE
#else
#define LV_NEMA_CACHE_HAL_INCLUDE "stm32u5xx_hal.h"
#endif
#ifdef CONFIG_LV_Z_USE_NEMA_VG
#define LV_USE_NEMA_VG 1
#else
#define LV_USE_NEMA_VG 0
#endif
#ifdef CONFIG_LV_Z_NEMA_GFX_MAX_RESX
#define LV_NEMA_GFX_MAX_RESX CONFIG_LV_Z_NEMA_GFX_MAX_RESX
#else
#define LV_NEMA_GFX_MAX_RESX 800
#endif
#ifdef CONFIG_LV_Z_NEMA_GFX_MAX_RESY
#define LV_NEMA_GFX_MAX_RESY CONFIG_LV_Z_NEMA_GFX_MAX_RESY
#else
#define LV_NEMA_GFX_MAX_RESY 600
#endif
#if defined(CONFIG_LV_Z_NEMA_LIB_M33_REVC)
#define LV_USE_NEMA_LIB 1
#elif defined(CONFIG_LV_Z_NEMA_LIB_M33_NEMAPVG)
#define LV_USE_NEMA_LIB 2
#elif defined(CONFIG_LV_Z_NEMA_LIB_M55)
#define LV_USE_NEMA_LIB 3
#elif defined(CONFIG_LV_Z_NEMA_LIB_M7)
#define LV_USE_NEMA_LIB 4
#else
#define LV_USE_NEMA_LIB 0
#endif
#if defined(CONFIG_LV_Z_NEMA_HAL_STM32)
#define LV_USE_NEMA_HAL 1
#ifdef CONFIG_LV_Z_NEMA_STM32_HAL_INCLUDE
#define LV_NEMA_STM32_HAL_INCLUDE CONFIG_LV_Z_NEMA_STM32_HAL_INCLUDE
#else
#define LV_NEMA_STM32_HAL_INCLUDE "stm32u5xx_hal.h"
#endif
#else
#define LV_USE_NEMA_HAL 0
#define LV_NEMA_STM32_HAL_INCLUDE "stm32u5xx_hal.h"
#endif

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
#define LV_ATTRIBUTE_MEM_ALIGN __aligned(CONFIG_LV_Z_ATTRIBUTE_MEM_ALIGN_SIZE)

#define LV_COLOR_16_SWAP_DISABLE_WARNING 1
#ifdef CONFIG_LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#endif /* CONFIG_LV_COLOR_16_SWAP */

#ifdef CONFIG_LV_Z_USE_OSAL
#define LV_USE_OS            LV_OS_CUSTOM
#define LV_OS_CUSTOM_INCLUDE "lvgl_zephyr_osal.h"
#endif /* CONFIG_LV_Z_USE_OSAL */

#endif /* ZEPHYR_MODULES_LVGL_LVGL_ZEPHYR_KCONFIG_H_ */

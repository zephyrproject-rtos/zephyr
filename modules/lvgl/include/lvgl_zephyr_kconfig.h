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

/* Provide definition to align LVGL buffers */
#define LV_ATTRIBUTE_MEM_ALIGN __aligned(CONFIG_LV_ATTRIBUTE_MEM_ALIGN_SIZE)

#ifdef CONFIG_LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#endif /* CONFIG_LV_COLOR_16_SWAP */

#ifdef CONFIG_LV_Z_USE_OSAL
#define LV_USE_OS            LV_OS_CUSTOM
#define LV_OS_CUSTOM_INCLUDE "lvgl_zephyr_osal.h"
#endif /* CONFIG_LV_Z_USE_OSAL */

#endif /* ZEPHYR_MODULES_LVGL_LVGL_ZEPHYR_KCONFIG_H_ */

/*
 * Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_INCLUDE_LVGL_ZEPHYR_CONFIG_H_
#define ZEPHYR_MODULES_LVGL_INCLUDE_LVGL_ZEPHYR_CONFIG_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>

#define LV_ASSERT_HANDLER __ASSERT_NO_MSG(false)
#define LV_ATTRIBUTE_MEM_ALIGN __aligned(CONFIG_LV_ATTRIBUTE_MEM_ALIGN_SIZE)

#endif /* ZEPHYR_MODULES_LVGL_INCLUDE_LVGL_ZEPHYR_CONFIG_H_ */

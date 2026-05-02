/*
 * Copyright 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_LVGL_SUPPORT_H_
#define ZEPHYR_MODULES_LVGL_LVGL_SUPPORT_H_

#include <zephyr/cache.h>

static ALWAYS_INLINE void DEMO_CleanInvalidateCacheByAddr(void *addr, uint16_t size)
{
	sys_cache_data_invd_range(addr, size);
}

#endif /* ZEPHYR_MODULES_LVGL_LVGL_SUPPORT_H_ */

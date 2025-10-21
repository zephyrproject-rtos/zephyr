/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_DISPLAY_H_
#define ZEPHYR_MODULES_LVGL_DISPLAY_H_

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lvgl_disp_data {
	const struct device *display_dev;
	struct display_capabilities cap;
	bool blanking_on;
	lv_draw_buf_t *display_buffer;
	struct k_mutex display_buffer_lock;
};

int lvgl_display_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_DISPLAY_H_ */

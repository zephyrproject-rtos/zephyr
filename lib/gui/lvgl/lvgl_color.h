/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_GUI_LVGL_LVGL_COLOR_H_
#define ZEPHYR_LIB_GUI_LVGL_LVGL_COLOR_H_

#include <display.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct device *lvgl_display_dev;

void *get_disp_flush(void);
void *get_vdb_write(void);
void *get_round_func(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LIB_GUI_LVGL_LVGL_COLOR_H */

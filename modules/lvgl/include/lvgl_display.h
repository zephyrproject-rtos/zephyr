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
};

struct lvgl_display_flush {
	lv_display_t *display;
	uint16_t x;
	uint16_t y;
	struct display_buffer_descriptor desc;
	void *buf;
};

void lvgl_flush_cb_mono(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
void lvgl_flush_cb_16bit(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
void lvgl_flush_cb_24bit(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);
void lvgl_flush_cb_32bit(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

void lvgl_rounder_cb_mono(lv_event_t *e);
void lvgl_set_mono_conversion_buffer(uint8_t *buffer, uint32_t buffer_size);

int set_lvgl_rendering_cb(lv_display_t *display);

void lvgl_flush_display(struct lvgl_display_flush *request);

#ifdef CONFIG_LV_Z_USE_ROUNDER_CB
void lvgl_rounder_cb(lv_event_t *e);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_DISPLAY_H_ */

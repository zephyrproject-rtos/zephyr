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
	lv_disp_drv_t *disp_drv;
	uint16_t x;
	uint16_t y;
	struct display_buffer_descriptor desc;
	void *buf;
};

void lvgl_flush_cb_mono(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
void lvgl_flush_cb_16bit(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
void lvgl_flush_cb_24bit(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
void lvgl_flush_cb_32bit(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

void lvgl_set_px_cb_mono(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			 lv_coord_t y, lv_color_t color, lv_opa_t opa);
void lvgl_set_px_cb_16bit(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			  lv_coord_t y, lv_color_t color, lv_opa_t opa);
void lvgl_set_px_cb_24bit(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			  lv_coord_t y, lv_color_t color, lv_opa_t opa);
void lvgl_set_px_cb_32bit(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x,
			  lv_coord_t y, lv_color_t color, lv_opa_t opa);

void lvgl_rounder_cb_mono(lv_disp_drv_t *disp_drv, lv_area_t *area);

int set_lvgl_rendering_cb(lv_disp_drv_t *disp_drv);

void lvgl_flush_display(struct lvgl_display_flush *request);

#ifdef CONFIG_LV_Z_USE_ROUNDER_CB
void lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_DISPLAY_H_ */

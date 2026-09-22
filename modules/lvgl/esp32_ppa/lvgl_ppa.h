/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_ESP32_PPA_LVGL_PPA_H_
#define ZEPHYR_MODULES_LVGL_ESP32_PPA_LVGL_PPA_H_

#include <stdbool.h>
#include <stdint.h>

#include <lvgl.h>
#include <lvgl_private.h>

#include <driver/ppa.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LVGL_PPA_DRAW_UNIT_ID 80
#define LVGL_PPA_PREFERENCE   70

struct lvgl_ppa_unit {
	lv_draw_unit_t base_unit;
	lv_draw_task_t *task_act;
	ppa_client_handle_t fill_client;
	ppa_client_handle_t blend_client;
	bool sw_fallback;
};

void lvgl_ppa_init(void);
void lvgl_ppa_buf_init_handlers(void);

void lvgl_ppa_fill(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords);
void lvgl_ppa_img(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords);

static inline bool lvgl_ppa_dest_cf_supported(lv_color_format_t cf)
{
	switch (cf) {
	case LV_COLOR_FORMAT_RGB565:
	case LV_COLOR_FORMAT_RGB888:
	case LV_COLOR_FORMAT_ARGB8888:
		return true;
	default:
		return false;
	}
}

static inline ppa_fill_color_mode_t lvgl_ppa_fill_cm(lv_color_format_t cf)
{
	switch (cf) {
	case LV_COLOR_FORMAT_RGB888:
		return PPA_FILL_COLOR_MODE_RGB888;
	case LV_COLOR_FORMAT_ARGB8888:
		return PPA_FILL_COLOR_MODE_ARGB8888;
	default:
		return PPA_FILL_COLOR_MODE_RGB565;
	}
}

static inline ppa_blend_color_mode_t lvgl_ppa_blend_cm(lv_color_format_t cf)
{
	switch (cf) {
	case LV_COLOR_FORMAT_RGB888:
		return PPA_BLEND_COLOR_MODE_RGB888;
	case LV_COLOR_FORMAT_ARGB8888:
		return PPA_BLEND_COLOR_MODE_ARGB8888;
	default:
		return PPA_BLEND_COLOR_MODE_RGB565;
	}
}

static inline int32_t lvgl_ppa_pic_w(uint32_t stride, int32_t w, lv_color_format_t cf)
{
	uint8_t px_size;

	if (stride == LV_STRIDE_AUTO) {
		return w;
	}

	px_size = lv_color_format_get_size(cf);
	if (px_size == 0U || (stride % px_size) != 0U) {
		return 0;
	}

	return (int32_t)(stride / px_size);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_ESP32_PPA_LVGL_PPA_H_ */

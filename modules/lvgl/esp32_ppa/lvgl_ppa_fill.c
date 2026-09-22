/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_ppa.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lvgl_ppa, CONFIG_LV_Z_LOG_LEVEL);

void lvgl_ppa_fill(lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)t->draw_unit;
	lv_draw_buf_t *draw_buf = t->target_layer->draw_buf;
	ppa_fill_oper_config_t cfg = {0};
	lv_area_t rel_coords = *coords;
	lv_area_t rel_clip = t->clip_area;
	lv_area_t blend_area;
	int32_t pic_w;
	esp_err_t err;

	lv_area_move(&rel_coords, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);
	lv_area_move(&rel_clip, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	if (!lv_area_intersect(&blend_area, &rel_coords, &rel_clip)) {
		return;
	}

	pic_w = lvgl_ppa_pic_w(draw_buf->header.stride, draw_buf->header.w, draw_buf->header.cf);
	if (pic_w == 0) {
		u->sw_fallback = true;
		lv_draw_sw_fill(t, dsc, coords);
		return;
	}

	cfg.fill_argb_color.val = lv_color_to_u32(dsc->color);
	cfg.fill_block_w = lv_area_get_width(&blend_area);
	cfg.fill_block_h = lv_area_get_height(&blend_area);
	cfg.out.buffer = draw_buf->data;
	cfg.out.buffer_size = draw_buf->data_size;
	cfg.out.pic_w = pic_w;
	cfg.out.pic_h = draw_buf->header.h;
	cfg.out.block_offset_x = blend_area.x1;
	cfg.out.block_offset_y = blend_area.y1;
	cfg.out.fill_cm = lvgl_ppa_fill_cm(draw_buf->header.cf);
	cfg.mode = PPA_TRANS_MODE_BLOCKING;

	err = ppa_do_fill(u->fill_client, &cfg);
	if (err != ESP_OK) {
		LOG_ERR("Fill failed (%d)", err);
	}
}

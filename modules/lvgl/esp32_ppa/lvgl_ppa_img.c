/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_ppa.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(lvgl_ppa, CONFIG_LV_Z_LOG_LEVEL);

static void lvgl_ppa_img_core(lv_draw_task_t *t, const lv_draw_image_dsc_t *draw_dsc,
			      const lv_image_decoder_dsc_t *decoder_dsc, lv_draw_image_sup_t *sup,
			      const lv_area_t *img_coords, const lv_area_t *clipped_img_area)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)t->draw_unit;
	lv_draw_buf_t *draw_buf = t->target_layer->draw_buf;
	const lv_draw_buf_t *decoded = decoder_dsc->decoded;
	lv_color_format_t src_cf = draw_dsc->header.cf;
	lv_color_format_t dest_cf = draw_buf->header.cf;
	ppa_blend_oper_config_t cfg = {0};
	lv_area_t rel_clip = *clipped_img_area;
	lv_area_t rel_coords = *img_coords;
	lv_area_t src_area;
	lv_area_t dest_area;
	int32_t src_pic_w;
	int32_t dest_pic_w;
	esp_err_t err;

	ARG_UNUSED(sup);

	lv_area_move(&rel_clip, -img_coords->x1, -img_coords->y1);
	lv_area_move(&rel_coords, -img_coords->x1, -img_coords->y1);

	if (!lv_area_intersect(&src_area, &rel_clip, &rel_coords)) {
		return;
	}

	dest_area = *clipped_img_area;
	lv_area_move(&dest_area, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	src_pic_w = lvgl_ppa_pic_w(decoded->header.stride, draw_dsc->header.w, src_cf);
	dest_pic_w = lvgl_ppa_pic_w(draw_buf->header.stride, draw_buf->header.w, dest_cf);
	if (src_pic_w == 0 || dest_pic_w == 0) {
		if (u->sw_fallback) {
			return;
		}
		u->sw_fallback = true;
		LOG_DBG("Stride is not a whole number of pixels, drawing in software");
		lv_draw_sw_image(t, draw_dsc, &t->area);
		return;
	}

	cfg.in_bg.buffer = (void *)decoded->data;
	cfg.in_bg.pic_w = src_pic_w;
	cfg.in_bg.pic_h = draw_dsc->header.h;
	cfg.in_bg.block_w = lv_area_get_width(clipped_img_area);
	cfg.in_bg.block_h = lv_area_get_height(clipped_img_area);
	cfg.in_bg.block_offset_x = src_area.x1;
	cfg.in_bg.block_offset_y = src_area.y1;
	cfg.in_bg.blend_cm = lvgl_ppa_blend_cm(src_cf);
	cfg.bg_alpha_update_mode = PPA_ALPHA_FIX_VALUE;
	cfg.bg_alpha_fix_val = 0xFF;

	cfg.in_fg.buffer = draw_buf->data;
	cfg.in_fg.pic_w = dest_pic_w;
	cfg.in_fg.pic_h = draw_buf->header.h;
	cfg.in_fg.block_w = lv_area_get_width(clipped_img_area);
	cfg.in_fg.block_h = lv_area_get_height(clipped_img_area);
	cfg.in_fg.block_offset_x = dest_area.x1;
	cfg.in_fg.block_offset_y = dest_area.y1;
	cfg.in_fg.blend_cm = lvgl_ppa_blend_cm(dest_cf);
	cfg.fg_alpha_update_mode = PPA_ALPHA_FIX_VALUE;
	cfg.fg_alpha_fix_val = 0x00;

	cfg.out.buffer = draw_buf->data;
	cfg.out.buffer_size = draw_buf->data_size;
	cfg.out.pic_w = dest_pic_w;
	cfg.out.pic_h = draw_buf->header.h;
	cfg.out.block_offset_x = dest_area.x1;
	cfg.out.block_offset_y = dest_area.y1;
	cfg.out.blend_cm = lvgl_ppa_blend_cm(dest_cf);
	cfg.mode = PPA_TRANS_MODE_BLOCKING;

	err = ppa_do_blend(u->blend_client, &cfg);
	if (err != ESP_OK) {
		LOG_ERR("Blend failed (%d)", err);
	}
}

void lvgl_ppa_img(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc, const lv_area_t *coords)
{
	if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
		return;
	}

	lv_draw_image_normal_helper(t, dsc, coords, lvgl_ppa_img_core, NULL);
}

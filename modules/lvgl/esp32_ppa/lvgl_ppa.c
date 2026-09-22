/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_ppa.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lvgl_ppa, CONFIG_LV_Z_LOG_LEVEL);

static int32_t lvgl_ppa_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *t);
static int32_t lvgl_ppa_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer);
static int32_t lvgl_ppa_delete(lv_draw_unit_t *draw_unit);

static bool other_unit_busy(const lv_layer_t *layer, const lv_draw_unit_t *self)
{
	lv_draw_task_t *t = layer->draw_task_head;

	while (t != NULL) {
		if (t->state == LV_DRAW_TASK_STATE_IN_PROGRESS && t->draw_unit != self) {
			return true;
		}
		t = t->next;
	}

	return false;
}

static void lvgl_ppa_execute(struct lvgl_ppa_unit *u)
{
	lv_draw_task_t *t = u->task_act;
	lv_layer_t *layer = t->target_layer;
	lv_draw_buf_t *buf = layer->draw_buf;
	lv_area_t area;
	lv_area_t buf_area;

	if (!lv_area_intersect(&area, &t->area, &t->clip_area)) {
		return;
	}

	buf_area = area;
	lv_area_move(&buf_area, -layer->buf_area.x1, -layer->buf_area.y1);

	lv_draw_buf_flush_cache(buf, &buf_area);

	switch (t->type) {
	case LV_DRAW_TASK_TYPE_FILL:
		lvgl_ppa_fill(t, (lv_draw_fill_dsc_t *)t->draw_dsc, &area);
		break;
	case LV_DRAW_TASK_TYPE_IMAGE:
		lvgl_ppa_img(t, (lv_draw_image_dsc_t *)t->draw_dsc, &t->area);
		break;
	default:
		return;
	}

	if (!u->sw_fallback) {
		lv_draw_buf_invalidate_cache(buf, &buf_area);
	}
}

static int32_t lvgl_ppa_evaluate(lv_draw_unit_t *draw_unit, lv_draw_task_t *t)
{
	const struct lvgl_ppa_unit *u = (const struct lvgl_ppa_unit *)draw_unit;
	const lv_draw_dsc_base_t *base = (lv_draw_dsc_base_t *)t->draw_dsc;

	if (u->fill_client == NULL) {
		return 0;
	}

	if (!lvgl_ppa_dest_cf_supported(base->layer->color_format)) {
		return 0;
	}

	switch (t->type) {
	case LV_DRAW_TASK_TYPE_FILL: {
		const lv_draw_fill_dsc_t *dsc = (lv_draw_fill_dsc_t *)t->draw_dsc;

		if (dsc->radius != 0 || dsc->grad.dir != LV_GRAD_DIR_NONE) {
			return 0;
		}
		if (dsc->opa <= (lv_opa_t)LV_OPA_MAX) {
			return 0;
		}
		break;
	}
#if defined(CONFIG_LV_Z_USE_ESP32_PPA_IMG)
	case LV_DRAW_TASK_TYPE_IMAGE: {
		const lv_draw_image_dsc_t *dsc = (lv_draw_image_dsc_t *)t->draw_dsc;

		if (dsc->header.cf >= LV_COLOR_FORMAT_PROPRIETARY_START || dsc->clip_radius != 0 ||
		    dsc->bitmap_mask_src != NULL || dsc->sup != NULL || dsc->tile != 0 ||
		    dsc->blend_mode != LV_BLEND_MODE_NORMAL || dsc->recolor_opa > LV_OPA_MIN ||
		    dsc->opa < (lv_opa_t)LV_OPA_MAX || dsc->skew_x != 0 || dsc->skew_y != 0 ||
		    dsc->scale_x != 256 || dsc->scale_y != 256 || dsc->rotation != 0 ||
		    lv_image_src_get_type(dsc->src) != LV_IMAGE_SRC_VARIABLE) {
			return 0;
		}
		if (dsc->header.cf != LV_COLOR_FORMAT_RGB888 &&
		    dsc->header.cf != LV_COLOR_FORMAT_RGB565) {
			return 0;
		}
		if (base->layer->color_format != LV_COLOR_FORMAT_RGB888 &&
		    base->layer->color_format != LV_COLOR_FORMAT_RGB565) {
			return 0;
		}
		break;
	}
#endif /* CONFIG_LV_Z_USE_ESP32_PPA_IMG */
	default:
		return 0;
	}

	if (t->preference_score > LVGL_PPA_PREFERENCE) {
		t->preference_score = LVGL_PPA_PREFERENCE;
		t->preferred_draw_unit_id = LVGL_PPA_DRAW_UNIT_ID;
	}

	return 1;
}

static int32_t lvgl_ppa_dispatch(lv_draw_unit_t *draw_unit, lv_layer_t *layer)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)draw_unit;
	lv_draw_task_t *t;

	if (u->fill_client == NULL || u->task_act != NULL) {
		return LV_DRAW_UNIT_IDLE;
	}

	/* Row maintenance would discard another unit's pixels in the same rows */
	if (other_unit_busy(layer, draw_unit)) {
		return LV_DRAW_UNIT_IDLE;
	}

	t = lv_draw_get_available_task(layer, NULL, LVGL_PPA_DRAW_UNIT_ID);
	if (t == NULL || t->preferred_draw_unit_id != LVGL_PPA_DRAW_UNIT_ID) {
		return LV_DRAW_UNIT_IDLE;
	}

	if (lv_draw_layer_alloc_buf(layer) == NULL) {
		return LV_DRAW_UNIT_IDLE;
	}

	t->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
	t->draw_unit = draw_unit;
	u->task_act = t;
	u->sw_fallback = false;

	lvgl_ppa_execute(u);

	u->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
	u->task_act = NULL;
	lv_draw_dispatch_request();

	return 1;
}

static int32_t lvgl_ppa_delete(lv_draw_unit_t *draw_unit)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)draw_unit;

	if (u->fill_client != NULL) {
		(void)ppa_unregister_client(u->fill_client);
	}
	if (u->blend_client != NULL) {
		(void)ppa_unregister_client(u->blend_client);
	}

	return 0;
}

void lvgl_ppa_init(void)
{
	ppa_client_config_t cfg = {
		.max_pending_trans_num = 1,
		.data_burst_length = PPA_DATA_BURST_LENGTH_128,
	};
	struct lvgl_ppa_unit *u;
	esp_err_t err;

	lvgl_ppa_buf_init_handlers();

	u = lv_draw_create_unit(sizeof(struct lvgl_ppa_unit));
	if (u == NULL) {
		LOG_ERR("Failed to create the PPA draw unit");
		return;
	}

	u->base_unit.evaluate_cb = lvgl_ppa_evaluate;
	u->base_unit.dispatch_cb = lvgl_ppa_dispatch;
	u->base_unit.delete_cb = lvgl_ppa_delete;
	u->base_unit.name = "ESP_PPA";

	cfg.oper_type = PPA_OPERATION_FILL;
	err = ppa_register_client(&cfg, &u->fill_client);
	if (err != ESP_OK) {
		LOG_ERR("Failed to register the fill client (%d)", err);
		return;
	}

	cfg.oper_type = PPA_OPERATION_BLEND;
	err = ppa_register_client(&cfg, &u->blend_client);
	if (err != ESP_OK) {
		LOG_ERR("Failed to register the blend client (%d)", err);
		(void)ppa_unregister_client(u->fill_client);
		u->fill_client = NULL;
	}
}

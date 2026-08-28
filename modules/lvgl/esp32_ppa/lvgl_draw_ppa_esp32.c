/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>

#include <lvgl.h>
#include <draw/lv_draw_private.h>
#include <draw/lv_draw_image_private.h>
#include <draw/lv_image_decoder_private.h>
#include <misc/lv_area_private.h>

#include "lvgl_display.h"

#include <driver/ppa.h>

LOG_MODULE_REGISTER(lvgl_ppa, CONFIG_LV_Z_LOG_LEVEL);

/* Picked so the accelerator is preferred over the software renderer for the
 * tasks it can take, while still losing to a unit that scores itself lower.
 */
#define LVGL_PPA_PREFERENCE_SCORE 80
#define LVGL_PPA_DRAW_UNIT_ID     8

#ifdef CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC
#define LVGL_PPA_TRANS_MODE PPA_TRANS_MODE_NON_BLOCKING
#else
#define LVGL_PPA_TRANS_MODE PPA_TRANS_MODE_BLOCKING
#endif

/* The scaling registers hold the factor as a 8.4 fixed-point value, so the
 * engine can only express whole sixteenths and cannot scale down.
 */
#define LVGL_PPA_SCALE_FRAC 16
#define LVGL_PPA_SCALE_MAX  256
#define LVGL_PPA_SCALE_UNIT 256

/* Setting up a transfer costs a fixed amount of descriptor writes and a wait
 * for the engine, so a small enough block is drawn faster in software than it
 * takes to hand over. Measured on a 1024x600 RGB888 panel, where compositing
 * the benchmark's 160x160 logos through the engine took about twice as long
 * as the software renderer.
 */
#define LVGL_PPA_MIN_BLEND_PX (192 * 192)

struct lvgl_ppa_unit {
	lv_draw_unit_t base;
	lv_draw_task_t *task_act;
	ppa_client_handle_t fill_client;
	ppa_client_handle_t blend_client;
	ppa_client_handle_t srm_client;
	void *srm_buf;
	size_t srm_buf_size;
	/* Transfers handed to the accelerator for the task being dispatched.
	 * An operation can decide it has nothing to do, and then there is no
	 * completion to wait for.
	 */
	uint32_t submitted;
#ifdef CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC
	/* The rows in flight, so they can be written back once the transfer
	 * reports it is done. The accelerator signals from an interrupt, where
	 * neither a cache operation nor LVGL may be called, so the flag is all
	 * that is set there.
	 */
	lv_draw_buf_t *pend_buf;
	lv_area_t pend_area;
	atomic_t pend_done;
#endif
};

static bool lvgl_ppa_cf_supported(lv_color_format_t cf)
{
	return cf == LV_COLOR_FORMAT_RGB888 || cf == LV_COLOR_FORMAT_RGB565;
}

/* Formats the engine can read as a blend or transform source. The destination
 * stays restricted to the opaque formats a layer buffer actually uses.
 */
static bool lvgl_ppa_src_cf_supported(lv_color_format_t cf)
{
	return cf == LV_COLOR_FORMAT_RGB888 || cf == LV_COLOR_FORMAT_RGB565 ||
	       cf == LV_COLOR_FORMAT_ARGB8888 || cf == LV_COLOR_FORMAT_XRGB8888;
}

static bool lvgl_ppa_cf_has_alpha(lv_color_format_t cf)
{
	return cf == LV_COLOR_FORMAT_ARGB8888;
}

static ppa_fill_color_mode_t lvgl_ppa_fill_cm(lv_color_format_t cf)
{
	return (cf == LV_COLOR_FORMAT_RGB565) ? PPA_FILL_COLOR_MODE_RGB565
					      : PPA_FILL_COLOR_MODE_RGB888;
}

static ppa_blend_color_mode_t lvgl_ppa_blend_cm(lv_color_format_t cf)
{
	switch (cf) {
	case LV_COLOR_FORMAT_RGB565:
		return PPA_BLEND_COLOR_MODE_RGB565;
	case LV_COLOR_FORMAT_ARGB8888:
	case LV_COLOR_FORMAT_XRGB8888:
		return PPA_BLEND_COLOR_MODE_ARGB8888;
	default:
		return PPA_BLEND_COLOR_MODE_RGB888;
	}
}

static ppa_srm_color_mode_t lvgl_ppa_srm_cm(lv_color_format_t cf)
{
	switch (cf) {
	case LV_COLOR_FORMAT_RGB565:
		return PPA_SRM_COLOR_MODE_RGB565;
	case LV_COLOR_FORMAT_ARGB8888:
	case LV_COLOR_FORMAT_XRGB8888:
		return PPA_SRM_COLOR_MODE_ARGB8888;
	default:
		return PPA_SRM_COLOR_MODE_RGB888;
	}
}

static bool lvgl_ppa_rotation_to_srm(int32_t rotation, ppa_srm_rotation_angle_t *angle)
{
	/* LVGL measures rotation clockwise in tenths of a degree, the engine
	 * counter-clockwise in quarter turns.
	 */
	switch (rotation) {
	case 0:
		*angle = PPA_SRM_ROTATION_ANGLE_0;
		return true;
	case 900:
		*angle = PPA_SRM_ROTATION_ANGLE_270;
		return true;
	case 1800:
		*angle = PPA_SRM_ROTATION_ANGLE_180;
		return true;
	case 2700:
		*angle = PPA_SRM_ROTATION_ANGLE_90;
		return true;
	default:
		return false;
	}
}

static bool lvgl_ppa_scale_supported(int32_t scale)
{
	/* Only whole sixteenths, no reduction, and within the register range. */
	return scale >= LVGL_PPA_SCALE_UNIT && scale <= LVGL_PPA_SCALE_UNIT * LVGL_PPA_SCALE_MAX &&
	       (scale % (LVGL_PPA_SCALE_UNIT / LVGL_PPA_SCALE_FRAC)) == 0;
}

/* The accelerator reads and writes memory over 2D-DMA, so the CPU's view of a
 * buffer has to be written back before a transfer and dropped after one.
 */
static void lvgl_ppa_sync_range(const void *addr, size_t size)
{
	sys_cache_data_flush_and_invd_range((void *)addr, size);
}

/* Sync only the rows the operation touches. The draw buffer is a whole
 * 1024x600 frame in external RAM, so writing all of it back around every
 * small blit costs more than the blit saves.
 */
static void lvgl_ppa_sync_area(const lv_draw_buf_t *buf, const lv_area_t *area)
{
	uint32_t stride = buf->header.stride;
	uint32_t row_first = (uint32_t)area->y1;
	uint32_t row_last = (uint32_t)area->y2;
	size_t offset;
	size_t size;

	if (area->y2 < area->y1 || stride == 0) {
		return;
	}

	offset = (size_t)row_first * stride;
	size = ((size_t)(row_last - row_first) + 1U) * stride;

	if (offset >= buf->data_size) {
		return;
	}

	size = MIN(size, buf->data_size - offset);

	lvgl_ppa_sync_range((uint8_t *)buf->data + offset, size);
}

/* Scratch for the two-pass transform of a source that carries alpha, grown on
 * demand so a draw task never allocates.
 */
static void *lvgl_ppa_srm_buf(struct lvgl_ppa_unit *u, size_t size)
{
	if (u->srm_buf_size >= size) {
		return u->srm_buf;
	}

	if (u->srm_buf != NULL) {
		lv_free(u->srm_buf);
	}

	u->srm_buf = lv_malloc(size);
	u->srm_buf_size = (u->srm_buf != NULL) ? size : 0;

	return u->srm_buf;
}

static void lvgl_ppa_fill(struct lvgl_ppa_unit *u, lv_draw_task_t *t, const lv_draw_fill_dsc_t *dsc,
			  const lv_area_t *coords)
{
	lv_draw_buf_t *draw_buf = t->target_layer->draw_buf;
	ppa_fill_oper_config_t cfg = {0};
	lv_area_t rel_coords;
	lv_area_t rel_clip;
	lv_area_t fill_area;
	int ret;

	lv_area_copy(&rel_coords, coords);
	lv_area_move(&rel_coords, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	lv_area_copy(&rel_clip, &t->clip_area);
	lv_area_move(&rel_clip, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	if (!lv_area_intersect(&fill_area, &rel_coords, &rel_clip)) {
		return;
	}

	cfg.fill_argb_color.val = lv_color_to_u32(dsc->color);
	cfg.fill_block_w = lv_area_get_width(&fill_area);
	cfg.fill_block_h = lv_area_get_height(&fill_area);
	cfg.out.buffer = draw_buf->data;
	cfg.out.buffer_size = draw_buf->data_size;
	cfg.out.pic_w = draw_buf->header.w;
	cfg.out.pic_h = draw_buf->header.h;
	cfg.out.block_offset_x = fill_area.x1;
	cfg.out.block_offset_y = fill_area.y1;
	cfg.out.fill_cm = lvgl_ppa_fill_cm(draw_buf->header.cf);
	cfg.mode = LVGL_PPA_TRANS_MODE;
	cfg.user_data = u;

	ret = ppa_do_fill(u->fill_client, &cfg);
	if (ret != 0) {
		LOG_ERR("Fill failed (%d)", ret);
	} else {
		u->submitted++;
	}
}

/* A translucent fill is a blend of a fixed colour over the destination, which
 * the fill engine cannot do because it never reads the destination back.
 */
static void lvgl_ppa_fill_opa(struct lvgl_ppa_unit *u, lv_draw_task_t *t,
			      const lv_draw_fill_dsc_t *dsc, const lv_area_t *coords)
{
	lv_draw_buf_t *draw_buf = t->target_layer->draw_buf;
	ppa_blend_color_mode_t dest_cm = lvgl_ppa_blend_cm(draw_buf->header.cf);
	uint32_t dest_px = lv_color_format_get_size(draw_buf->header.cf);
	ppa_blend_oper_config_t cfg = {0};
	lv_area_t rel_coords;
	lv_area_t rel_clip;
	lv_area_t fill_area;
	uint32_t dest_pitch_px;
	int ret;

	lv_area_copy(&rel_coords, coords);
	lv_area_move(&rel_coords, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	lv_area_copy(&rel_clip, &t->clip_area);
	lv_area_move(&rel_clip, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	if (!lv_area_intersect(&fill_area, &rel_coords, &rel_clip)) {
		return;
	}

	dest_pitch_px = draw_buf->header.stride / dest_px;

	/* The destination is the background, and an A8 foreground of a fixed
	 * colour supplies the tint, with the task opacity as its alpha.
	 */
	cfg.in_bg.buffer = draw_buf->data;
	cfg.in_bg.pic_w = dest_pitch_px;
	cfg.in_bg.pic_h = draw_buf->header.h;
	cfg.in_bg.block_w = lv_area_get_width(&fill_area);
	cfg.in_bg.block_h = lv_area_get_height(&fill_area);
	cfg.in_bg.block_offset_x = fill_area.x1;
	cfg.in_bg.block_offset_y = fill_area.y1;
	cfg.in_bg.blend_cm = dest_cm;
	cfg.bg_alpha_update_mode = PPA_ALPHA_FIX_VALUE;
	cfg.bg_alpha_fix_val = 0xFF;

	cfg.in_fg.buffer = draw_buf->data;
	cfg.in_fg.pic_w = dest_pitch_px;
	cfg.in_fg.pic_h = draw_buf->header.h;
	cfg.in_fg.block_w = lv_area_get_width(&fill_area);
	cfg.in_fg.block_h = lv_area_get_height(&fill_area);
	cfg.in_fg.block_offset_x = fill_area.x1;
	cfg.in_fg.block_offset_y = fill_area.y1;
	cfg.in_fg.blend_cm = PPA_BLEND_COLOR_MODE_A8;
	cfg.fg_fix_rgb_val.r = dsc->color.red;
	cfg.fg_fix_rgb_val.g = dsc->color.green;
	cfg.fg_fix_rgb_val.b = dsc->color.blue;
	cfg.fg_alpha_update_mode = PPA_ALPHA_FIX_VALUE;
	cfg.fg_alpha_fix_val = dsc->opa;

	cfg.out.buffer = draw_buf->data;
	cfg.out.buffer_size = draw_buf->data_size;
	cfg.out.pic_w = dest_pitch_px;
	cfg.out.pic_h = draw_buf->header.h;
	cfg.out.block_offset_x = fill_area.x1;
	cfg.out.block_offset_y = fill_area.y1;
	cfg.out.blend_cm = dest_cm;
	cfg.mode = LVGL_PPA_TRANS_MODE;
	cfg.user_data = u;

	ret = ppa_do_blend(u->blend_client, &cfg);
	if (ret != 0) {
		LOG_ERR("Translucent fill failed (%d)", ret);
	} else {
		u->submitted++;
	}
}

/* Straight copy of a source block onto the destination, compositing when the
 * source carries an alpha channel.
 */
static void lvgl_ppa_img_blit(struct lvgl_ppa_unit *u, const lv_draw_image_dsc_t *dsc,
			      const lv_draw_buf_t *decoded, lv_draw_buf_t *draw_buf,
			      const lv_area_t *src_area, const lv_area_t *dest_area,
			      const lv_area_t *block)
{
	lv_color_format_t src_cf = dsc->header.cf;
	lv_color_format_t dest_cf = draw_buf->header.cf;
	bool src_has_alpha = lvgl_ppa_cf_has_alpha(src_cf);
	ppa_blend_oper_config_t cfg = {0};
	uint32_t dest_pitch_px = draw_buf->header.stride / lv_color_format_get_size(dest_cf);
	int ret;

	cfg.in_bg.buffer = decoded->data;
	cfg.in_bg.pic_w = dsc->header.stride / lv_color_format_get_size(src_cf);
	cfg.in_bg.pic_h = dsc->header.h;
	cfg.in_bg.block_w = lv_area_get_width(block);
	cfg.in_bg.block_h = lv_area_get_height(block);
	cfg.in_bg.block_offset_x = src_area->x1;
	cfg.in_bg.block_offset_y = src_area->y1;
	cfg.in_bg.blend_cm = lvgl_ppa_blend_cm(src_cf);

	cfg.in_fg.buffer = draw_buf->data;
	cfg.in_fg.pic_w = dest_pitch_px;
	cfg.in_fg.pic_h = draw_buf->header.h;
	cfg.in_fg.block_w = lv_area_get_width(block);
	cfg.in_fg.block_h = lv_area_get_height(block);
	cfg.in_fg.block_offset_x = dest_area->x1;
	cfg.in_fg.block_offset_y = dest_area->y1;
	cfg.in_fg.blend_cm = lvgl_ppa_blend_cm(dest_cf);

	if (src_has_alpha) {
		/* Let both layers keep their own alpha so the engine really
		 * composites the source over what is already there.
		 */
		cfg.bg_alpha_update_mode = PPA_ALPHA_NO_CHANGE;
		cfg.fg_alpha_update_mode = PPA_ALPHA_NO_CHANGE;
	} else {
		/* An opaque source replaces the destination, so hold the
		 * destination fully transparent.
		 */
		cfg.bg_alpha_update_mode = PPA_ALPHA_FIX_VALUE;
		cfg.bg_alpha_fix_val = 0xFF;
		cfg.fg_alpha_update_mode = PPA_ALPHA_FIX_VALUE;
		cfg.fg_alpha_fix_val = 0;
	}

	cfg.out.buffer = draw_buf->data;
	cfg.out.buffer_size = draw_buf->data_size;
	cfg.out.pic_w = dest_pitch_px;
	cfg.out.pic_h = draw_buf->header.h;
	cfg.out.block_offset_x = dest_area->x1;
	cfg.out.block_offset_y = dest_area->y1;
	cfg.out.blend_cm = lvgl_ppa_blend_cm(dest_cf);
	cfg.mode = LVGL_PPA_TRANS_MODE;
	cfg.user_data = u;

	ret = ppa_do_blend(u->blend_client, &cfg);
	if (ret != 0) {
		LOG_ERR("Image blend failed (%d)", ret);
	} else {
		u->submitted++;
	}
}

/* Scale and rotate a source into the destination. An opaque source can go
 * straight there; one with alpha has to land in scratch first, because the
 * transform engine writes its output without consulting the destination.
 */
static void lvgl_ppa_img_transform(struct lvgl_ppa_unit *u, const lv_draw_image_dsc_t *dsc,
				   const lv_draw_buf_t *decoded, lv_draw_buf_t *draw_buf,
				   const lv_area_t *dest_area, ppa_srm_rotation_angle_t angle)
{
	lv_color_format_t src_cf = dsc->header.cf;
	lv_color_format_t dest_cf = draw_buf->header.cf;
	uint32_t dest_pitch_px = draw_buf->header.stride / lv_color_format_get_size(dest_cf);
	uint32_t out_w = lv_area_get_width(dest_area);
	uint32_t out_h = lv_area_get_height(dest_area);
	ppa_srm_oper_config_t cfg = {0};
	bool src_has_alpha = lvgl_ppa_cf_has_alpha(src_cf);
	void *scratch = NULL;
	int ret;

	cfg.in.buffer = decoded->data;
	cfg.in.pic_w = dsc->header.stride / lv_color_format_get_size(src_cf);
	cfg.in.pic_h = dsc->header.h;
	cfg.in.block_w = dsc->header.w;
	cfg.in.block_h = dsc->header.h;
	cfg.in.block_offset_x = 0;
	cfg.in.block_offset_y = 0;
	cfg.in.srm_cm = lvgl_ppa_srm_cm(src_cf);

	cfg.rotation_angle = angle;
	cfg.scale_x = (float)dsc->scale_x / (float)LVGL_PPA_SCALE_UNIT;
	cfg.scale_y = (float)dsc->scale_y / (float)LVGL_PPA_SCALE_UNIT;
	cfg.alpha_update_mode = PPA_ALPHA_NO_CHANGE;
	cfg.mode = LVGL_PPA_TRANS_MODE;
	cfg.user_data = u;

	if (!src_has_alpha) {
		cfg.out.buffer = draw_buf->data;
		cfg.out.buffer_size = draw_buf->data_size;
		cfg.out.pic_w = dest_pitch_px;
		cfg.out.pic_h = draw_buf->header.h;
		cfg.out.block_offset_x = dest_area->x1;
		cfg.out.block_offset_y = dest_area->y1;
		cfg.out.srm_cm = lvgl_ppa_srm_cm(dest_cf);

		ret = ppa_do_scale_rotate_mirror(u->srm_client, &cfg);
		if (ret != 0) {
			LOG_ERR("Image transform failed (%d)", ret);
		} else {
			u->submitted++;
		}
		return;
	}

	/* Two passes: transform into scratch keeping the alpha, then blend the
	 * scratch over the destination.
	 */
	size_t scratch_size = (size_t)out_w * out_h * sizeof(uint32_t);

	scratch = lvgl_ppa_srm_buf(u, scratch_size);
	if (scratch == NULL) {
		LOG_ERR("No scratch for a %ux%u transform", out_w, out_h);
		return;
	}

	cfg.out.buffer = scratch;
	cfg.out.buffer_size = scratch_size;
	cfg.out.pic_w = out_w;
	cfg.out.pic_h = out_h;
	cfg.out.block_offset_x = 0;
	cfg.out.block_offset_y = 0;
	cfg.out.srm_cm = PPA_SRM_COLOR_MODE_ARGB8888;

	lvgl_ppa_sync_range(scratch, scratch_size);

	/* The blend below reads the scratch, so this pass has to be finished
	 * before it is submitted, whatever mode the rest of the unit uses.
	 */
	cfg.mode = PPA_TRANS_MODE_BLOCKING;

	ret = ppa_do_scale_rotate_mirror(u->srm_client, &cfg);
	if (ret != 0) {
		LOG_ERR("Image transform failed (%d)", ret);
		return;
	}

	lvgl_ppa_sync_range(scratch, scratch_size);

	ppa_blend_oper_config_t blend = {0};

	blend.in_bg.buffer = scratch;
	blend.in_bg.pic_w = out_w;
	blend.in_bg.pic_h = out_h;
	blend.in_bg.block_w = out_w;
	blend.in_bg.block_h = out_h;
	blend.in_bg.blend_cm = PPA_BLEND_COLOR_MODE_ARGB8888;
	blend.bg_alpha_update_mode = PPA_ALPHA_NO_CHANGE;

	blend.in_fg.buffer = draw_buf->data;
	blend.in_fg.pic_w = dest_pitch_px;
	blend.in_fg.pic_h = draw_buf->header.h;
	blend.in_fg.block_w = out_w;
	blend.in_fg.block_h = out_h;
	blend.in_fg.block_offset_x = dest_area->x1;
	blend.in_fg.block_offset_y = dest_area->y1;
	blend.in_fg.blend_cm = lvgl_ppa_blend_cm(dest_cf);
	blend.fg_alpha_update_mode = PPA_ALPHA_NO_CHANGE;

	blend.out.buffer = draw_buf->data;
	blend.out.buffer_size = draw_buf->data_size;
	blend.out.pic_w = dest_pitch_px;
	blend.out.pic_h = draw_buf->header.h;
	blend.out.block_offset_x = dest_area->x1;
	blend.out.block_offset_y = dest_area->y1;
	blend.out.blend_cm = lvgl_ppa_blend_cm(dest_cf);
	blend.mode = LVGL_PPA_TRANS_MODE;
	blend.user_data = u;

	ret = ppa_do_blend(u->blend_client, &blend);
	if (ret != 0) {
		LOG_ERR("Transformed image blend failed (%d)", ret);
	} else {
		u->submitted++;
	}
}

static void lvgl_ppa_img_core(lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc,
			      const lv_image_decoder_dsc_t *decoder_dsc, lv_draw_image_sup_t *sup,
			      const lv_area_t *img_coords, const lv_area_t *clipped_img_area)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)t->draw_unit;
	lv_draw_buf_t *draw_buf = t->target_layer->draw_buf;
	const lv_draw_buf_t *decoded = decoder_dsc->decoded;
	ppa_srm_rotation_angle_t angle;
	lv_area_t rel_clip;
	lv_area_t rel_img;
	lv_area_t src_area;
	lv_area_t dest_area;

	ARG_UNUSED(sup);

	if (decoded == NULL || decoded->data == NULL) {
		return;
	}

	lv_area_copy(&dest_area, clipped_img_area);
	lv_area_move(&dest_area, -t->target_layer->buf_area.x1, -t->target_layer->buf_area.y1);

	if (dsc->rotation != 0 || dsc->scale_x != LVGL_PPA_SCALE_UNIT ||
	    dsc->scale_y != LVGL_PPA_SCALE_UNIT) {
		if (!lvgl_ppa_rotation_to_srm(dsc->rotation, &angle)) {
			return;
		}

		lvgl_ppa_img_transform(u, dsc, decoded, draw_buf, &dest_area, angle);
		return;
	}

	lv_area_copy(&rel_clip, clipped_img_area);
	lv_area_move(&rel_clip, -img_coords->x1, -img_coords->y1);

	lv_area_copy(&rel_img, img_coords);
	lv_area_move(&rel_img, -img_coords->x1, -img_coords->y1);

	if (!lv_area_intersect(&src_area, &rel_clip, &rel_img)) {
		return;
	}

	lvgl_ppa_img_blit(u, dsc, decoded, draw_buf, &src_area, &dest_area, clipped_img_area);
}

static void lvgl_ppa_img(struct lvgl_ppa_unit *u, lv_draw_task_t *t, const lv_draw_image_dsc_t *dsc,
			 const lv_area_t *coords)
{
	ARG_UNUSED(u);

	if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
		return;
	}

	lv_draw_image_normal_helper(t, dsc, coords, lvgl_ppa_img_core, NULL);
}

static int32_t lvgl_ppa_evaluate_fill(lv_draw_task_t *t)
{
	const lv_draw_fill_dsc_t *dsc = (lv_draw_fill_dsc_t *)t->draw_dsc;

	/* Rounded corners and gradients are not shapes the accelerator can
	 * express.
	 */
	if (dsc->radius != 0 || dsc->grad.dir != LV_GRAD_DIR_NONE) {
		return 0;
	}

	/* A fully transparent fill draws nothing, and the blend path below is
	 * only worth taking when something is actually composited.
	 */
	if (dsc->opa <= (lv_opa_t)LV_OPA_MIN) {
		return 0;
	}

	return 1;
}

static int32_t lvgl_ppa_evaluate_img(lv_draw_task_t *t)
{
	const lv_draw_image_dsc_t *dsc = (lv_draw_image_dsc_t *)t->draw_dsc;
	ppa_srm_rotation_angle_t angle;

	if (dsc->header.cf >= LV_COLOR_FORMAT_PROPRIETARY_START) {
		return 0;
	}

	if (!lvgl_ppa_src_cf_supported(dsc->header.cf)) {
		return 0;
	}

	/* The engine walks the source by whole pixels, so a stride that is not
	 * a pixel multiple would shear the image.
	 */
	if ((dsc->header.stride % lv_color_format_get_size(dsc->header.cf)) != 0) {
		return 0;
	}

	if (dsc->clip_radius != 0 || dsc->bitmap_mask_src != NULL || dsc->sup != NULL ||
	    dsc->tile != 0 || dsc->blend_mode != LV_BLEND_MODE_NORMAL ||
	    dsc->recolor_opa > LV_OPA_MIN || dsc->opa < (lv_opa_t)LV_OPA_MAX) {
		return 0;
	}

	if (dsc->skew_x != 0 || dsc->skew_y != 0) {
		return 0;
	}

	if (lv_image_src_get_type(dsc->src) != LV_IMAGE_SRC_VARIABLE) {
		return 0;
	}

	/* Compositing a source that carries alpha is a read-modify-write of
	 * the destination, so it only pays off once the block is large enough
	 * to cover the cost of handing the work over.
	 */
	if (lvgl_ppa_cf_has_alpha(dsc->header.cf)) {
		int32_t px = lv_area_get_width(&t->area) * lv_area_get_height(&t->area);

		if (px < LVGL_PPA_MIN_BLEND_PX) {
			return 0;
		}
	}

	if (dsc->rotation != 0 || dsc->scale_x != LVGL_PPA_SCALE_UNIT ||
	    dsc->scale_y != LVGL_PPA_SCALE_UNIT) {
		lv_area_t transformed;

		if (!lvgl_ppa_rotation_to_srm(dsc->rotation, &angle)) {
			return 0;
		}

		if (!lvgl_ppa_scale_supported(dsc->scale_x) ||
		    !lvgl_ppa_scale_supported(dsc->scale_y)) {
			return 0;
		}

		/* The transform engine takes no clip rectangle and writes its
		 * whole output extent, so a partially clipped result would
		 * paint over its surroundings.
		 */
		lv_image_buf_get_transformed_area(&transformed, dsc->header.w, dsc->header.h,
						  dsc->rotation, dsc->scale_x, dsc->scale_y,
						  &dsc->pivot);
		lv_area_move(&transformed, dsc->image_area.x1, dsc->image_area.y1);

		if (!lv_area_is_in(&transformed, &t->clip_area, 0)) {
			return 0;
		}
	}

	return 1;
}

static int32_t lvgl_ppa_evaluate(lv_draw_unit_t *unit, lv_draw_task_t *t)
{
	const lv_draw_dsc_base_t *base = (lv_draw_dsc_base_t *)t->draw_dsc;
	int32_t ok;

	ARG_UNUSED(unit);

	if (!lvgl_ppa_cf_supported(base->layer->color_format)) {
		return 0;
	}

	switch (t->type) {
	case LV_DRAW_TASK_TYPE_FILL:
		ok = lvgl_ppa_evaluate_fill(t);
		break;
	case LV_DRAW_TASK_TYPE_IMAGE:
		ok = lvgl_ppa_evaluate_img(t);
		break;
	default:
		return 0;
	}

	if (ok == 0) {
		return 0;
	}

	if (t->preference_score > LVGL_PPA_PREFERENCE_SCORE) {
		t->preference_score = LVGL_PPA_PREFERENCE_SCORE;
		t->preferred_draw_unit_id = LVGL_PPA_DRAW_UNIT_ID;
	}

	return 1;
}

#ifdef CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC

static bool lvgl_ppa_done_cb(ppa_client_handle_t client, ppa_event_data_t *event, void *user_data)
{
	struct lvgl_ppa_unit *u = user_data;

	ARG_UNUSED(client);
	ARG_UNUSED(event);

	atomic_set(&u->pend_done, 1);

	/* Ask for a dispatch, otherwise nothing comes back to finish the task:
	 * the unit reported itself idle when it handed the transfer over.
	 */
	lv_draw_dispatch_request();

	return false;
}

/* Finish a transfer that has reported it is done, from the LVGL thread. */
static bool lvgl_ppa_reap(struct lvgl_ppa_unit *u)
{
	if (u->task_act == NULL) {
		return false;
	}

	if (atomic_get(&u->pend_done) == 0) {
		return false;
	}

	if (u->pend_buf != NULL) {
		lvgl_ppa_sync_area(u->pend_buf, &u->pend_area);
		u->pend_buf = NULL;
	}

	atomic_set(&u->pend_done, 0);
	u->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
	u->task_act = NULL;
	lv_draw_dispatch_request();

	return true;
}

#endif /* CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC */

static int32_t lvgl_ppa_dispatch(lv_draw_unit_t *unit, lv_layer_t *layer)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)unit;
	lv_draw_task_t *t;
	lv_area_t area;
	lv_area_t sync_area;

#ifdef CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC
	(void)lvgl_ppa_reap(u);
#endif

	if (u->task_act != NULL) {
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
	t->draw_unit = unit;
	u->task_act = t;
	u->submitted = 0U;

	if (lv_area_intersect(&area, &t->area, &t->clip_area)) {
		/* The rows the task writes, in buffer coordinates. */
		lv_area_copy(&sync_area, &area);
		lv_area_move(&sync_area, -t->target_layer->buf_area.x1,
			     -t->target_layer->buf_area.y1);

		lvgl_ppa_sync_area(t->target_layer->draw_buf, &sync_area);

		switch (t->type) {
		case LV_DRAW_TASK_TYPE_FILL: {
			const lv_draw_fill_dsc_t *dsc = (lv_draw_fill_dsc_t *)t->draw_dsc;

			if (dsc->opa < (lv_opa_t)LV_OPA_MAX) {
				lvgl_ppa_fill_opa(u, t, dsc, &area);
			} else {
				lvgl_ppa_fill(u, t, dsc, &area);
			}
			break;
		}
		case LV_DRAW_TASK_TYPE_IMAGE:
			lvgl_ppa_img(u, t, (lv_draw_image_dsc_t *)t->draw_dsc, &t->area);
			break;
		default:
			break;
		}

#ifdef CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC
		if (u->submitted != 0U) {
			/* A transfer is running, so the rows are written back
			 * once it reports it is done.
			 */
			u->pend_buf = t->target_layer->draw_buf;
			lv_area_copy(&u->pend_area, &sync_area);

			return 1;
		}
#endif
		lvgl_ppa_sync_area(t->target_layer->draw_buf, &sync_area);
	}

	u->task_act->state = LV_DRAW_TASK_STATE_FINISHED;
	u->task_act = NULL;
	lv_draw_dispatch_request();

	return 1;
}

static int32_t lvgl_ppa_delete(lv_draw_unit_t *unit)
{
	struct lvgl_ppa_unit *u = (struct lvgl_ppa_unit *)unit;

	if (u->fill_client != NULL) {
		ppa_unregister_client(u->fill_client);
		u->fill_client = NULL;
	}

	if (u->blend_client != NULL) {
		ppa_unregister_client(u->blend_client);
		u->blend_client = NULL;
	}

	if (u->srm_client != NULL) {
		ppa_unregister_client(u->srm_client);
		u->srm_client = NULL;
	}

	if (u->srm_buf != NULL) {
		lv_free(u->srm_buf);
		u->srm_buf = NULL;
		u->srm_buf_size = 0;
	}

	return 0;
}

int lvgl_draw_unit_init(void)
{
	struct lvgl_ppa_unit *u = lv_draw_create_unit(sizeof(struct lvgl_ppa_unit));
	ppa_client_config_t fill_cfg = {
		.oper_type = PPA_OPERATION_FILL,
		.max_pending_trans_num = 1,
	};
	ppa_client_config_t blend_cfg = {
		.oper_type = PPA_OPERATION_BLEND,
		.max_pending_trans_num = 1,
	};
	ppa_client_config_t srm_cfg = {
		.oper_type = PPA_OPERATION_SRM,
		.max_pending_trans_num = 1,
	};
	int ret;

	if (u == NULL) {
		LOG_ERR("Failed to create the draw unit");
		return -ENOMEM;
	}

	ret = ppa_register_client(&fill_cfg, &u->fill_client);
	if (ret != 0) {
		LOG_ERR("Failed to register the fill client (%d)", ret);
		return -ENODEV;
	}

	ret = ppa_register_client(&blend_cfg, &u->blend_client);
	if (ret != 0) {
		LOG_ERR("Failed to register the blend client (%d)", ret);
		return -ENODEV;
	}

	ret = ppa_register_client(&srm_cfg, &u->srm_client);
	if (ret != 0) {
		LOG_ERR("Failed to register the transform client (%d)", ret);
		return -ENODEV;
	}

#ifdef CONFIG_LV_Z_DRAW_PPA_ESP32_ASYNC
	const ppa_event_callbacks_t cbs = {
		.on_trans_done = lvgl_ppa_done_cb,
	};

	if (ppa_client_register_event_callbacks(u->fill_client, &cbs) != 0 ||
	    ppa_client_register_event_callbacks(u->blend_client, &cbs) != 0 ||
	    ppa_client_register_event_callbacks(u->srm_client, &cbs) != 0) {
		LOG_ERR("Failed to register the completion callback");
		return -ENODEV;
	}
#endif

	u->base.evaluate_cb = lvgl_ppa_evaluate;
	u->base.dispatch_cb = lvgl_ppa_dispatch;
	u->base.delete_cb = lvgl_ppa_delete;

	LOG_INF("PPA draw unit registered");

	return 0;
}

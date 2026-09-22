/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_ppa.h"

#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

/*
 * The accelerator reaches the draw buffers over 2D-DMA behind the L2 cache,
 * so maintenance is done in whole L2 lines rather than the line size the
 * generic cache API reports for the core.
 */
#define LVGL_PPA_LINE_SIZE CONFIG_ESP32_CACHE_L2_LINE_SIZE

static void ppa_msync_rows(const lv_draw_buf_t *draw_buf, const lv_area_t *area, bool invalidate)
{
	size_t start = 0;
	size_t end = draw_buf->data_size;

	if (area != NULL) {
		uint32_t stride = draw_buf->header.stride;
		int32_t row_first = MAX(area->y1, 0);
		int32_t row_last = MIN(area->y2, (int32_t)draw_buf->header.h - 1);

		if (stride == 0U || row_last < row_first) {
			return;
		}

		start = (size_t)row_first * stride;
		end = (size_t)(row_last + 1) * stride;
	}

	start = ROUND_DOWN(start, LVGL_PPA_LINE_SIZE);
	end = ROUND_UP(end, LVGL_PPA_LINE_SIZE);
	if (end > draw_buf->data_size) {
		end = ROUND_DOWN(draw_buf->data_size, LVGL_PPA_LINE_SIZE);
	}
	if (start >= end) {
		return;
	}

	if (invalidate) {
		(void)sys_cache_data_invd_range(draw_buf->data + start, end - start);
	} else {
		(void)sys_cache_data_flush_range(draw_buf->data + start, end - start);
	}
}

static void ppa_invalidate_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
	ppa_msync_rows(draw_buf, area, true);
}

static void ppa_flush_cache(const lv_draw_buf_t *draw_buf, const lv_area_t *area)
{
	ppa_msync_rows(draw_buf, area, false);
}

void lvgl_ppa_buf_init_handlers(void)
{
	lv_draw_buf_handlers_t *handlers = lv_draw_buf_get_handlers();

	handlers->invalidate_cache_cb = ppa_invalidate_cache;
	handlers->flush_cache_cb = ppa_flush_cache;
}

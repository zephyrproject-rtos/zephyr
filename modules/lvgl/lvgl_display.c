/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>

#include "lvgl_display.h"

#ifdef CONFIG_LV_Z_FLUSH_THREAD

K_SEM_DEFINE(flush_complete, 0, 1);
/* Message queue will only ever need to queue one message */
K_MSGQ_DEFINE(flush_queue, sizeof(struct lvgl_display_flush), 1, 1);

void lvgl_flush_thread_entry(void *arg1, void *arg2, void *arg3)
{
	struct lvgl_display_flush flush;
	struct lvgl_disp_data *data;

	while (1) {
		k_msgq_get(&flush_queue, &flush, K_FOREVER);
		data = (struct lvgl_disp_data *)flush.disp_drv->user_data;
		const bool is_last = lv_disp_flush_is_last(flush.disp_drv);

		display_write(data->display_dev, flush.x, flush.y, &flush.desc,
			      flush.buf);

		lv_disp_flush_ready(flush.disp_drv);
		k_sem_give(&flush_complete);

		if (is_last && (data->cap.screen_info & SCREEN_INFO_REQUIRES_SHOW)) {
			display_show(data->display_dev);
		}
	}
}

K_THREAD_DEFINE(lvgl_flush_thread, CONFIG_LV_Z_FLUSH_THREAD_STACK_SIZE,
		lvgl_flush_thread_entry, NULL, NULL, NULL,
		K_PRIO_COOP(CONFIG_LV_Z_FLUSH_THREAD_PRIO), 0, 0);


void lvgl_wait_cb(lv_disp_drv_t *disp_drv)
{
	k_sem_take(&flush_complete, K_FOREVER);
}

#endif /* CONFIG_LV_Z_FLUSH_THREAD */

#ifdef CONFIG_LV_Z_USE_ROUNDER_CB
void lvgl_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
#if CONFIG_LV_Z_AREA_X_ALIGNMENT_WIDTH != 1
	__ASSERT(POPCOUNT(CONFIG_LV_Z_AREA_X_ALIGNMENT_WIDTH) == 1, "Invalid X alignment width");

	area->x1 &= ~(CONFIG_LV_Z_AREA_X_ALIGNMENT_WIDTH - 1);
	area->x2 |= (CONFIG_LV_Z_AREA_X_ALIGNMENT_WIDTH - 1);
#endif
#if CONFIG_LV_Z_AREA_Y_ALIGNMENT_WIDTH != 1
	__ASSERT(POPCOUNT(CONFIG_LV_Z_AREA_Y_ALIGNMENT_WIDTH) == 1, "Invalid Y alignment width");

	area->y1 &= ~(CONFIG_LV_Z_AREA_Y_ALIGNMENT_WIDTH - 1);
	area->y2 |= (CONFIG_LV_Z_AREA_Y_ALIGNMENT_WIDTH - 1);
#endif
}
#else
#define lvgl_rounder_cb NULL
#endif

int set_lvgl_rendering_cb(lv_disp_drv_t *disp_drv)
{
	int err = 0;
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)disp_drv->user_data;

#ifdef CONFIG_LV_Z_FLUSH_THREAD
	disp_drv->wait_cb = lvgl_wait_cb;
#endif

	switch (data->cap.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		disp_drv->flush_cb = lvgl_flush_cb_32bit;
		disp_drv->rounder_cb = lvgl_rounder_cb;
#ifdef CONFIG_LV_COLOR_DEPTH_32
		disp_drv->set_px_cb = NULL;
#else
		disp_drv->set_px_cb = lvgl_set_px_cb_32bit;
#endif
		break;
	case PIXEL_FORMAT_RGB_888:
		disp_drv->flush_cb = lvgl_flush_cb_24bit;
		disp_drv->rounder_cb = lvgl_rounder_cb;
		disp_drv->set_px_cb = lvgl_set_px_cb_24bit;
		break;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_BGR_565:
		disp_drv->flush_cb = lvgl_flush_cb_16bit;
		disp_drv->rounder_cb = lvgl_rounder_cb;
#ifdef CONFIG_LV_COLOR_DEPTH_16
		disp_drv->set_px_cb = NULL;
#else
		disp_drv->set_px_cb = lvgl_set_px_cb_16bit;
#endif
		break;
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		disp_drv->flush_cb = lvgl_flush_cb_mono;
		disp_drv->rounder_cb = lvgl_rounder_cb_mono;
		disp_drv->set_px_cb = lvgl_set_px_cb_mono;
		break;
	default:
		disp_drv->flush_cb = NULL;
		disp_drv->rounder_cb = NULL;
		disp_drv->set_px_cb = NULL;
		err = -ENOTSUP;
		break;
	}

	return err;
}

void lvgl_flush_display(struct lvgl_display_flush *request)
{
#ifdef CONFIG_LV_Z_FLUSH_THREAD
	/*
	 * LVGL will only start a flush once the previous one is complete,
	 * so we can reset the flush state semaphore here.
	 */
	k_sem_reset(&flush_complete);
	k_msgq_put(&flush_queue, request, K_FOREVER);
	/* Explicitly yield, in case the calling thread is a cooperative one */
	k_yield();
#else
	/* Write directly to the display */
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)request->disp_drv->user_data;
	const bool is_last = lv_disp_flush_is_last(request->disp_drv);

	display_write(data->display_dev, request->x, request->y,
		      &request->desc, request->buf);
	lv_disp_flush_ready(request->disp_drv);

	if (is_last && (data->cap.screen_info & SCREEN_INFO_REQUIRES_SHOW)) {
		display_show(data->display_dev);
	}
#endif
}

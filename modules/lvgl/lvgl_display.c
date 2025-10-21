/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>

#include "lvgl_display.h"
#include "lvgl_zephyr.h"
#include "draw/ambiq/lv_draw_ambiq_private.h"

static lv_display_t *lv_displays[DT_ZEPHYR_DISPLAYS_COUNT];
struct lvgl_disp_data disp_data[DT_ZEPHYR_DISPLAYS_COUNT] = {{
	.blanking_on = false,
}};

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays)
#define DISPLAY_NODE(n) DT_ZEPHYR_DISPLAY(n)
#elif DT_HAS_CHOSEN(zephyr_display)
#define DISPLAY_NODE(n) DT_CHOSEN(zephyr_display)
#else
#error Could not find "zephyr,display" chosen property, or a "zephyr,displays" compatible node in DT
#define DISPLAY_NODE(n) DT_INVALID_NODE
#endif

#define ENUMERATE_DISPLAY_DEVS(n) display_dev[n] = DEVICE_DT_GET(DISPLAY_NODE(n));

#if CONFIG_LV_Z_REFRESH_MODE_PARTIAL
static lv_display_render_mode_t refresh_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
#elif CONFIG_LV_Z_REFRESH_MODE_DIRECT
static lv_display_render_mode_t refresh_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
#elif CONFIG_LV_Z_REFRESH_MODE_FULL
static lv_display_render_mode_t refresh_mode = LV_DISPLAY_RENDER_MODE_FULL;
#else
static lv_display_render_mode_t refresh_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
#endif

#if CONFIG_LV_COLOR_DEPTH_32
static lv_color_format_t draw_buffer_format = LV_COLOR_FORMAT_ARGB8888;
#elif CONFIG_LV_COLOR_DEPTH_24
static lv_color_format_t draw_buffer_format = LV_COLOR_FORMAT_RGB888;
#elif CONFIG_LV_COLOR_DEPTH_16
static lv_color_format_t draw_buffer_format = LV_COLOR_FORMAT_RGB565;
#elif CONFIG_LV_COLOR_DEPTH_8
static lv_color_format_t draw_buffer_format = LV_COLOR_FORMAT_L8;
#else
#error "Unsupported color format"
#endif

static void display_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map);

/* Message queue will only ever need to queue one message */
K_MSGQ_DEFINE(flush_queue, sizeof(void *), 1, 1);

void lvgl_flush_thread_entry(void *arg1, void *arg2, void *arg3)
{
	struct lvgl_disp_data *private_data;
	lv_display_t *display;
	int32_t x_off;
	int32_t y_off;
	struct display_buffer_descriptor desc;
	void *buf;

	while (1) {
		k_msgq_get(&flush_queue, &display, K_FOREVER);

		private_data = (struct lvgl_disp_data *)lv_display_get_user_data(display);
		x_off = lv_display_get_offset_x(display);
		y_off = lv_display_get_offset_y(display);

		if ((x_off < 0) || (y_off < 0)) {
			continue;
		}

		desc.buf_size = private_data->display_buffer->data_size;
		desc.width = private_data->display_buffer->header.w;
		desc.height = private_data->display_buffer->header.h;
		desc.pitch = private_data->display_buffer->header.w;
		desc.frame_incomplete = false;
		buf = private_data->display_buffer->data;

		k_mutex_lock(&private_data->display_buffer_lock, K_FOREVER);

		display_write(private_data->display_dev, (uint16_t)x_off, (uint16_t)y_off, &desc,
			      buf);

		k_mutex_unlock(&private_data->display_buffer_lock);
	}
}

K_THREAD_DEFINE(lvgl_flush_thread, CONFIG_LV_Z_FLUSH_THREAD_STACK_SIZE, lvgl_flush_thread_entry,
		NULL, NULL, NULL, CONFIG_LV_Z_FLUSH_THREAD_PRIORITY, 0, 0);

static int lvgl_allocate_rendering_buffers(lv_display_t *display)
{
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)lv_display_get_user_data(display);
	uint32_t hor_res = lv_display_get_horizontal_resolution(display);
	uint32_t ver_res = lv_display_get_vertical_resolution(display);
	lv_color_format_t display_format;

	switch (data->cap.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		display_format = LV_COLOR_FORMAT_XRGB8888;
		break;
	case PIXEL_FORMAT_RGB_888:
		display_format = LV_COLOR_FORMAT_BGR888;
		break;
	case PIXEL_FORMAT_RGB_565:
		display_format = LV_COLOR_FORMAT_RGB565;
		break;
	case PIXEL_FORMAT_L_8:
		display_format = LV_COLOR_FORMAT_L8;
		break;
	case PIXEL_FORMAT_AL_88:
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}

	data->display_buffer = lv_draw_buf_create(hor_res, ver_res, display_format, 0);
	if (data->display_buffer == NULL) {
		return -ENOMEM;
	}

	uint32_t draw_buffer_width = hor_res;
	uint32_t draw_buffer_height = (refresh_mode == LV_DISPLAY_RENDER_MODE_PARTIAL)
					      ? MAX((CONFIG_LV_Z_VDB_SIZE * ver_res) / 100, 1)
					      : ver_res;

	lv_draw_buf_t *draw_buffer =
		lv_draw_buf_create(draw_buffer_width, draw_buffer_height, draw_buffer_format, 0);

	/* Set draw buffer */
	lv_display_set_draw_buffers(display, draw_buffer, NULL);

	/* Set display refresh mode */
	lv_display_set_render_mode(display, refresh_mode);

	return 0;
}

int lvgl_display_init(void)
{
	const struct device *display_dev[DT_ZEPHYR_DISPLAYS_COUNT];
	struct lvgl_disp_data *p_disp_data;
	int err;

	/* clang-format off */
	FOR_EACH(ENUMERATE_DISPLAY_DEVS, (), LV_DISPLAYS_IDX_LIST);
	/* clang-format on */
	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		if (!device_is_ready(display_dev[i])) {
			return -ENODEV;
		}
	}

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		p_disp_data = &disp_data[i];

		/* Initialize the display buffer lock */
		k_mutex_init(&p_disp_data->display_buffer_lock);

		p_disp_data->display_dev = display_dev[i];
		display_get_capabilities(display_dev[i], &p_disp_data->cap);

		uint32_t width = (CONFIG_LV_Z_REFRESH_AREA_WIDTH == 0)
					 ? p_disp_data->cap.x_resolution
					 : CONFIG_LV_Z_REFRESH_AREA_WIDTH;
		uint32_t height = (CONFIG_LV_Z_REFRESH_AREA_HEIGHT == 0)
					  ? p_disp_data->cap.y_resolution
					  : CONFIG_LV_Z_REFRESH_AREA_HEIGHT;

		lv_displays[i] = lv_display_create(width, height);
		if (!lv_displays[i]) {
			return -ENOMEM;
		}

		/* Set display physical resolution. */
		lv_display_set_physical_resolution(lv_displays[i], p_disp_data->cap.x_resolution,
						   p_disp_data->cap.y_resolution);

		/* Set display offset, the display refresh area is always in the center of the
		 * display panel.
		 */
		uint32_t offset_x = (width < p_disp_data->cap.x_resolution)
					    ? (((p_disp_data->cap.x_resolution - width) >> 2) << 1)
					    : 0;
		uint32_t offset_y = (height < p_disp_data->cap.y_resolution)
					    ? (((p_disp_data->cap.y_resolution - height) >> 2) << 1)
					    : 0;
		lv_display_set_offset(lv_displays[i], offset_x, offset_y);

		lv_display_set_user_data(lv_displays[i], p_disp_data);

		/* Set flush cb */
		lv_display_set_flush_cb(lv_displays[i], display_flush_cb);

		err = lvgl_allocate_rendering_buffers(lv_displays[i]);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

void display_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
	/* offset the draw area. */
	lv_area_t area_display = *area;
	struct lvgl_disp_data *private_data =
		(struct lvgl_disp_data *)lv_display_get_user_data(display);

	int32_t x_off = lv_display_get_offset_x(display);
	int32_t y_off = lv_display_get_offset_y(display);

	lv_area_move(&area_display, -x_off, -y_off);

	/* If this is the last part, wait for GPU to complete all the queued jobs
	 * and start transfer the display buffer to display panel.
	 */
	bool is_last = lv_display_flush_is_last(display);

	if ((refresh_mode != LV_DISPLAY_RENDER_MODE_PARTIAL) && !is_last) {
		/* Inform LVGL that the draw buffer is available to be used.*/
		lv_disp_flush_ready(display);

		return;
	}

	/* Lock display buffer, prevent display interface from reading this buffer.*/
	k_mutex_lock(&private_data->display_buffer_lock, K_FOREVER);

	/* Copy draw buffer to display buffer.*/
	lv_area_t *target_area =
		(refresh_mode == LV_DISPLAY_RENDER_MODE_PARTIAL) ? &area_display : NULL;
	lv_draw_ambiq_display_buffer_sync(private_data->display_buffer, target_area, (void *)px_map,
					  draw_buffer_format);

	/* Unlock this buffer.*/
	k_mutex_unlock(&private_data->display_buffer_lock);

	/* Inform LVGL that the draw buffer is available to be used. */
	lv_disp_flush_ready(display);

	if (is_last) {
		k_msgq_put(&flush_queue, &display, K_FOREVER);

		/* Explicitly yield to allow the refresh thread to run. */
		k_yield();
	}
}

/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr.h>
#include <lvgl.h>
#include "lvgl_display.h"
#ifdef CONFIG_LVGL_FILESYSTEM
#include "lvgl_fs.h"
#endif
#include LV_MEM_CUSTOM_INCLUDE

#define LOG_LEVEL CONFIG_LVGL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lvgl);

#ifdef CONFIG_LVGL_BUFFER_ALLOC_STATIC

static lv_disp_buf_t disp_buf;

#define BUFFER_SIZE (CONFIG_LVGL_BITS_PER_PIXEL * ((CONFIG_LVGL_VDB_SIZE * \
			CONFIG_LVGL_HOR_RES * CONFIG_LVGL_VER_RES) / 100) / 8)

#define NBR_PIXELS_IN_BUFFER (BUFFER_SIZE * 8 / CONFIG_LVGL_BITS_PER_PIXEL)

static u8_t buf0[BUFFER_SIZE];
#ifdef CONFIG_LVGL_DOUBLE_VDB
static u8_t buf1[BUFFER_SIZE];
#endif

#endif /* CONFIG_LVGL_BUFFER_ALLOC_STATIC */

#if CONFIG_LVGL_LOG_LEVEL != 0
static void lvgl_log(lv_log_level_t level, const char *file, uint32_t line,
		const char *dsc)
{
	/* Convert LVGL log level to Zephyr log level
	 *
	 * LVGL log level mapping:
	 * * LV_LOG_LEVEL_TRACE 0
	 * * LV_LOG_LEVEL_INFO  1
	 * * LV_LOG_LEVEL_WARN  2
	 * * LV_LOG_LEVEL_ERROR 3
	 * * LV_LOG_LEVEL_NUM  4
	 *
	 * Zephyr log level mapping:
	 * *  LOG_LEVEL_NONE 0
	 * * LOG_LEVEL_ERR 1
	 * * LOG_LEVEL_WRN 2
	 * * LOG_LEVEL_INF 3
	 * * LOG_LEVEL_DBG 4
	 */
	u8_t zephyr_level = LOG_LEVEL_DBG - level;

	ARG_UNUSED(file);
	ARG_UNUSED(line);

	Z_LOG(zephyr_level, "%s", dsc);
}
#endif

#ifdef CONFIG_LVGL_BUFFER_ALLOC_STATIC

static int lvgl_allocate_rendering_buffers(lv_disp_drv_t *disp_drv)
{
	disp_drv->buffer = &disp_buf;
#ifdef CONFIG_LVGL_DOUBLE_VDB
	lv_disp_buf_init(disp_drv->buffer, &buf0, &buf1, NBR_PIXELS_IN_BUFFER);
#else
	lv_disp_buf_init(disp_drv->buffer, &buf0, NULL, NBR_PIXELS_IN_BUFFER);
#endif /* CONFIG_LVGL_DOUBLE_VDB  */

	return 0;
}

#else

static int lvgl_allocate_rendering_buffers(lv_disp_drv_t *disp_drv)
{
	void *buf0 = NULL;
	void *buf1 = NULL;
	u16_t buf_nbr_pixels;
	u32_t buf_size;
	struct display_capabilities cap;
	struct device *display_dev = (struct device *)disp_drv->user_data;

	display_get_capabilities(display_dev, &cap);

	disp_drv->hor_res = cap.x_resolution;
	disp_drv->ver_res = cap.y_resolution;

	buf_nbr_pixels = (CONFIG_LVGL_VDB_SIZE * disp_drv->hor_res *
			disp_drv->ver_res) / 100;
	/* one horizontal line is the minimum buffer requirement for lvgl */
	if (buf_nbr_pixels < disp_drv->hor_res) {
		buf_nbr_pixels = disp_drv->hor_res;
	}

	switch (cap.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		buf_size = 4 * buf_nbr_pixels;
		break;
	case PIXEL_FORMAT_RGB_888:
		buf_size = 3 * buf_nbr_pixels;
		break;
	case PIXEL_FORMAT_RGB_565:
		buf_size = 2 * buf_nbr_pixels;
		break;
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		buf_size = buf_nbr_pixels / 8;
		buf_size += (buf_nbr_pixels % 8) == 0 ? 0 : 1;
		break;
	default:
		return -ENOTSUP;
	}

	buf0 = LV_MEM_CUSTOM_ALLOC(buf_size);
	if (buf0 == NULL) {
		LOG_ERR("Failed to allocate memory for rendering buffer");
		return -ENOMEM;
	}

#ifdef CONFIG_LVGL_DOUBLE_VDB
	buf1 = LV_MEM_CUSTOM_ALLOC(buf_size);
	if (buf1 == NULL) {
		LV_MEM_CUSTOM_FREE(buf0);
		LOG_ERR("Failed to allocate memory for rendering buffer");
		return -ENOMEM;
	}
#endif

	disp_drv->buffer = LV_MEM_CUSTOM_ALLOC(sizeof(lv_disp_buf_t));
	if (disp_drv->buffer == NULL) {
		LV_MEM_CUSTOM_FREE(buf0);
		LV_MEM_CUSTOM_FREE(buf1);
		LOG_ERR("Failed to allocate memory to store rendering buffers");
		return -ENOMEM;
	}

	lv_disp_buf_init(disp_drv->buffer, buf0, buf1, buf_nbr_pixels);
	return 0;
}
#endif /* CONFIG_LVGL_BUFFER_ALLOC_STATIC */

static int lvgl_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct device *display_dev =
		device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);
	lv_disp_drv_t disp_drv;

	if (display_dev == NULL) {
		LOG_ERR("Display device not found.");
		return -ENODEV;
	}

#if CONFIG_LVGL_LOG_LEVEL != 0
	lv_log_register_print_cb(lvgl_log);
#endif

	lv_init();

#ifdef CONFIG_LVGL_FILESYSTEM
	lvgl_fs_init();
#endif

	lv_disp_drv_init(&disp_drv);
	disp_drv.user_data = (void *) display_dev;

	lvgl_allocate_rendering_buffers(&disp_drv);

	if (set_lvgl_rendering_cb(&disp_drv) != 0) {
		LOG_ERR("Display not supported.");
		return -ENOTSUP;
	}

	if (lv_disp_drv_register(&disp_drv) == NULL) {
		LOG_ERR("Failed to register display device.");
		return -EPERM;
	}

	return 0;
}

SYS_INIT(lvgl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

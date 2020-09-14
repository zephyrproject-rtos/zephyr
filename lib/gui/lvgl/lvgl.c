/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr.h>
#include <lvgl.h>
#include "lvgl_display.h"
#ifdef CONFIG_LVGL_USE_FILESYSTEM
#include "lvgl_fs.h"
#endif
#ifdef CONFIG_LVGL_POINTER_KSCAN
#include <drivers/kscan.h>
#endif
#include LV_MEM_CUSTOM_INCLUDE

#define LOG_LEVEL CONFIG_LVGL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lvgl);

#ifdef CONFIG_LVGL_BUFFER_ALLOC_STATIC

static lv_disp_buf_t disp_buf;

#define BUFFER_SIZE (CONFIG_LVGL_BITS_PER_PIXEL * ((CONFIG_LVGL_VDB_SIZE * \
			CONFIG_LVGL_HOR_RES_MAX * CONFIG_LVGL_VER_RES_MAX) / 100) / 8)

#define NBR_PIXELS_IN_BUFFER (BUFFER_SIZE * 8 / CONFIG_LVGL_BITS_PER_PIXEL)

/* NOTE: depending on chosen color depth buffer may be accessed using uint8_t *,
 * uint16_t * or uint32_t *, therefore buffer needs to be aligned accordingly to
 * prevent unaligned memory accesses.
 */
static uint8_t buf0[BUFFER_SIZE] __aligned(4);
#ifdef CONFIG_LVGL_DOUBLE_VDB
static uint8_t buf1[BUFFER_SIZE] __aligned(4);
#endif

#endif /* CONFIG_LVGL_BUFFER_ALLOC_STATIC */

#if CONFIG_LVGL_LOG_LEVEL != 0
static void lvgl_log(lv_log_level_t level, const char *file, uint32_t line,
		const char *func, const char *dsc)
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
	uint8_t zephyr_level = LOG_LEVEL_DBG - level;

	ARG_UNUSED(file);
	ARG_UNUSED(line);
	ARG_UNUSED(func);

	Z_LOG(zephyr_level, "%s", log_strdup(dsc));
}
#endif

#ifdef CONFIG_LVGL_BUFFER_ALLOC_STATIC

static int lvgl_allocate_rendering_buffers(lv_disp_drv_t *disp_drv)
{
	struct display_capabilities cap;
	const struct device *display_dev = (const struct device *)disp_drv->user_data;
	int err = 0;

	display_get_capabilities(display_dev, &cap);

	if (cap.x_resolution <= CONFIG_LVGL_HOR_RES_MAX) {
		disp_drv->hor_res = cap.x_resolution;
	} else {
		LOG_ERR("Horizontal resolution is larger than maximum");
		err = -ENOTSUP;
	}

	if (cap.y_resolution <= CONFIG_LVGL_VER_RES_MAX) {
		disp_drv->ver_res = cap.y_resolution;
	} else {
		LOG_ERR("Vertical resolution is larger than maximum");
		err = -ENOTSUP;
	}

	disp_drv->buffer = &disp_buf;
#ifdef CONFIG_LVGL_DOUBLE_VDB
	lv_disp_buf_init(disp_drv->buffer, &buf0, &buf1, NBR_PIXELS_IN_BUFFER);
#else
	lv_disp_buf_init(disp_drv->buffer, &buf0, NULL, NBR_PIXELS_IN_BUFFER);
#endif /* CONFIG_LVGL_DOUBLE_VDB  */

	return err;
}

#else

static int lvgl_allocate_rendering_buffers(lv_disp_drv_t *disp_drv)
{
	void *buf0 = NULL;
	void *buf1 = NULL;
	uint16_t buf_nbr_pixels;
	uint32_t buf_size;
	struct display_capabilities cap;
	const struct device *display_dev = (const struct device *)disp_drv->user_data;

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

#ifdef CONFIG_LVGL_POINTER_KSCAN
K_MSGQ_DEFINE(kscan_msgq, sizeof(lv_indev_data_t),
	      CONFIG_LVGL_POINTER_KSCAN_MSGQ_COUNT, 4);

static void lvgl_pointer_kscan_callback(const struct device *dev,
					uint32_t row,
					uint32_t col, bool pressed)
{
	lv_indev_data_t data = {
		.point.x = col,
		.point.y = row,
		.state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL,
	};

	if (k_msgq_put(&kscan_msgq, &data, K_NO_WAIT) != 0) {
		LOG_ERR("Could put input data into queue");
	}
}

static bool lvgl_pointer_kscan_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	lv_disp_t *disp;
	const struct device *disp_dev;
	struct display_capabilities cap;
	lv_indev_data_t curr;

	static lv_indev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = LV_INDEV_STATE_REL,
	};

	if (k_msgq_get(&kscan_msgq, &curr, K_NO_WAIT) == 0) {
		prev = curr;
	}

	disp = lv_disp_get_default();
	disp_dev = disp->driver.user_data;

	display_get_capabilities(disp_dev, &cap);

	/* adjust kscan coordinates */
	if (IS_ENABLED(CONFIG_LVGL_POINTER_KSCAN_SWAP_XY)) {
		lv_coord_t x;

		x = prev.point.x;
		prev.point.x = prev.point.y;
		prev.point.y = x;
	}

	if (IS_ENABLED(CONFIG_LVGL_POINTER_KSCAN_INVERT_X)) {
		prev.point.x = cap.x_resolution - prev.point.x;
	}

	if (IS_ENABLED(CONFIG_LVGL_POINTER_KSCAN_INVERT_Y)) {
		prev.point.y = cap.y_resolution - prev.point.y;
	}

	/* rotate touch point to match display rotation */
	if (cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		lv_coord_t x;

		x = prev.point.x;
		prev.point.x = cap.y_resolution - prev.point.y;
		prev.point.y = x;
	} else if (cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		prev.point.x = cap.x_resolution - prev.point.x;
		prev.point.y = cap.y_resolution - prev.point.y;
	} else if (cap.current_orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		lv_coord_t x;

		x = prev.point.x;
		prev.point.x = prev.point.y;
		prev.point.y = cap.x_resolution - x;
	}

	*data = prev;

	return k_msgq_num_used_get(&kscan_msgq) > 0;
}

static int lvgl_pointer_kscan_init(void)
{
	const struct device *kscan_dev =
		device_get_binding(CONFIG_LVGL_POINTER_KSCAN_DEV_NAME);

	lv_indev_drv_t indev_drv;

	if (kscan_dev == NULL) {
		LOG_ERR("Keyboard scan device not found.");
		return -ENODEV;
	}

	if (kscan_config(kscan_dev, lvgl_pointer_kscan_callback) < 0) {
		LOG_ERR("Could not configure keyboard scan device.");
		return -ENODEV;
	}

	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = lvgl_pointer_kscan_read;

	if (lv_indev_drv_register(&indev_drv) == NULL) {
		LOG_ERR("Failed to register input device.");
		return -EPERM;
	}

	kscan_enable_callback(kscan_dev);

	return 0;
}
#endif /* CONFIG_LVGL_POINTER_KSCAN */

static int lvgl_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct device *display_dev =
		device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);
	int err = 0;
	lv_disp_drv_t disp_drv;

	if (display_dev == NULL) {
		LOG_ERR("Display device not found.");
		return -ENODEV;
	}

#if CONFIG_LVGL_LOG_LEVEL != 0
	lv_log_register_print_cb(lvgl_log);
#endif

	lv_init();

#ifdef CONFIG_LVGL_USE_FILESYSTEM
	lvgl_fs_init();
#endif

	lv_disp_drv_init(&disp_drv);
	disp_drv.user_data = (void *) display_dev;

	err = lvgl_allocate_rendering_buffers(&disp_drv);
	if (err != 0) {
		return err;
	}

	if (set_lvgl_rendering_cb(&disp_drv) != 0) {
		LOG_ERR("Display not supported.");
		return -ENOTSUP;
	}

	if (lv_disp_drv_register(&disp_drv) == NULL) {
		LOG_ERR("Failed to register display device.");
		return -EPERM;
	}

#ifdef CONFIG_LVGL_POINTER_KSCAN
	lvgl_pointer_kscan_init();
#endif /* CONFIG_LVGL_POINTER_KSCAN */

	return 0;
}

SYS_INIT(lvgl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

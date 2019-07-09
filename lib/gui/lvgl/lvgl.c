/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr.h>
#include <zio/zio_dev.h>
#include <zio/zio_buf.h>
#include <drivers/zio/ft5336.h>
#include <lvgl.h>
#include <lv_core/lv_refr.h>
#include "lvgl_color.h"
#include "lvgl_fs.h"

#define LOG_LEVEL CONFIG_LVGL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lvgl);

struct device *lvgl_display_dev;

#if CONFIG_LVGL_LOG_LEVEL != 0
static void lvgl_log(lv_log_level_t level, const char *file, uint32_t line,
		const char *dsc)
{
	/* Convert LVGL log level to Zephyr log lvel
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

#if CONFIG_LVGL_INPUT_POINTER
static struct device *lvgl_input_pointer_dev;
static lv_indev_data_t indev_data;
static ZIO_BUF_DEFINE(inputbuf);

static struct k_poll_event events[1] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
			K_POLL_MODE_NOTIFY_ONLY, &inputbuf.sem, 0),
};

static bool lvgl_input_pointer_read(lv_indev_data_t *data)
{
	struct ft5336_datum datum;

	if (zio_buf_get_length(&inputbuf) == 0) {
		zio_dev_trigger(lvgl_input_pointer_dev);
	}

	if ((k_poll(events, 1, K_MSEC(1)) == 0) &&
	    (zio_buf_pull(&inputbuf, &datum) == 1) &&
	    (datum.id == 0)) {
		indev_data.point.x = datum.y;
		indev_data.point.y = datum.x;
		indev_data.state =
			((datum.event == FT5336_DOWN) ||
			 (datum.event == FT5336_CONTACT)) ?
			LV_INDEV_STATE_PR :
			LV_INDEV_STATE_REL;

		LOG_DBG("point state %d at (%d,%d)",
			indev_data.state,
			indev_data.point.x,
			indev_data.point.y);
	}

	*data = indev_data;

	return zio_buf_get_length(&inputbuf) > 0;
}

static int lvgl_input_pointer_init(void)
{
	lv_indev_drv_t indev_drv;

	lvgl_input_pointer_dev =
		device_get_binding(CONFIG_LVGL_INPUT_POINTER_DEV_NAME);

	if (lvgl_input_pointer_dev == NULL) {
		LOG_ERR("Input device not found.");
		return -ENODEV;
	}

	/* lvgl doesn't suppot multitouch input */
	zio_dev_set_attr(lvgl_input_pointer_dev, FT5336_TOUCHES_IDX, (u8_t) 1);

	zio_buf_attach(&inputbuf, lvgl_input_pointer_dev);

	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read = lvgl_input_pointer_read;

	if (lv_indev_drv_register(&indev_drv) == NULL) {
		LOG_ERR("Failed to register input device.");
		return -EPERM;
	}

	return 0;
}
#endif /* CONFIG_LVGL_INPUT_POINTER */

static int lvgl_init(struct device *dev)
{
	lv_disp_drv_t disp_drv;

	ARG_UNUSED(dev);

	lvgl_display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (lvgl_display_dev == NULL) {
		LOG_ERR("Display device not found.");
		return -ENODEV;
	}

#if CONFIG_LVGL_LOG_LEVEL != 0
	lv_log_register_print(lvgl_log);
#endif

	lv_init();

#ifdef CONFIG_LVGL_FILESYSTEM
	lvgl_fs_init();
#endif

	lv_refr_set_round_cb(get_round_func());

	lv_disp_drv_init(&disp_drv);
	disp_drv.disp_flush = get_disp_flush();

#if CONFIG_LVGL_VDB_SIZE != 0
	disp_drv.vdb_wr = get_vdb_write();
#endif
	if (lv_disp_drv_register(&disp_drv) == NULL) {
		LOG_ERR("Failed to register display device.");
		return -EPERM;
	}
#if CONFIG_LVGL_INPUT_POINTER
	lvgl_input_pointer_init();
#endif

	return 0;
}

SYS_INIT(lvgl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

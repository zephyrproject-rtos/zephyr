/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <zephyr.h>
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

	_LOG(zephyr_level, "%s", dsc);
}
#endif


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

	return 0;
}

SYS_INIT(lvgl_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

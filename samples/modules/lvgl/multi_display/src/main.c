/*
 * Copyright (c) 2025 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <lvgl_mem.h>
#include <lv_demos.h>
#include <stdio.h>
#include <lvgl_zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#if !DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays) && !DT_HAS_CHOSEN(zephyr_display)
#error Could not find "zephyr,display" chosen property, or "zephyr,displays" compatible node in DT
#endif

#define ENUMERATE_DISPLAY_DEVS(n) display_dev[n] = DEVICE_DT_GET(DT_ZEPHYR_DISPLAY(n));

int main(void)
{
	const struct device *display_dev[DT_ZEPHYR_DISPLAYS_COUNT];

	FOR_EACH(ENUMERATE_DISPLAY_DEVS, (), LV_DISPLAYS_IDX_LIST)
		;

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		if (!device_is_ready(display_dev[i])) {
			LOG_ERR("Display device %d is not ready", i);
			return 0;
		}
	}

	lv_display_set_default(LVGL_DISPLAY_OBJ(0));
	lv_demo_widgets();

#if DT_ZEPHYR_DISPLAYS_COUNT > 1
	lv_display_set_default(LVGL_DISPLAY_OBJ(1));
	lv_demo_flex_layout();
#endif /* DT_ZEPHYR_DISPLAYS_COUNT > 1 */

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		display_blanking_off(display_dev[i]);
	}

#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
	lvgl_print_heap_info(false);
#else
	printf("lvgl in malloc mode\n");
#endif /* CONFIG_LV_Z_MEM_POOL_SYS_HEAP */

	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}

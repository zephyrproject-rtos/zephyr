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

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays)
#define ZEPHYR_DISPLAY_NODE DT_ZEPHYR_DISPLAY(0)
#elif DT_HAS_CHOSEN(zephyr_display)
#define ZEPHYR_DISPLAY_NODE DT_CHOSEN(zephyr_display)
#else
#error Could not find "zephyr,display" chosen property, or "zephyr,displays" compatible node in DT
#endif /* DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays) */

int main(void)
{
	const struct device *display_dev;
#if DT_ZEPHYR_DISPLAYS_COUNT > 1
	const struct device *display_dev2;
#endif /* DT_ZEPHYR_DISPLAYS_COUNT > 1 */

	display_dev = DEVICE_DT_GET(ZEPHYR_DISPLAY_NODE);
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

#if DT_ZEPHYR_DISPLAYS_COUNT > 1
	LOG_DBG("Second display is present");
	display_dev2 = DEVICE_DT_GET(DT_ZEPHYR_DISPLAY(1));
	if (!device_is_ready(display_dev2)) {
		LOG_ERR("Display device 2 not ready, aborting test");
		return 0;
	}
#endif /* DT_ZEPHYR_DISPLAYS_COUNT > 1 */

	lv_display_set_default(LVGL_DISPLAY_OBJ(0));
	lv_demo_stress();

#if DT_ZEPHYR_DISPLAYS_COUNT > 1
	lv_display_set_default(LVGL_DISPLAY_OBJ(1));
	lv_demo_benchmark();
#endif /* DT_ZEPHYR_DISPLAYS_COUNT > 1 */

	display_blanking_off(display_dev);
#if DT_ZEPHYR_DISPLAYS_COUNT > 1
	display_blanking_off(display_dev2);
#endif /* DT_ZEPHYR_DISPLAYS_COUNT > 1 */

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

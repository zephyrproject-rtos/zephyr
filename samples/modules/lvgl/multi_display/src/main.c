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

#if !DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays)
#error Could not find "zephyr,displays" compatible node in DT
#endif

#define ENUMERATE_DISPLAY_DEVS(n) display_dev[n] = DEVICE_DT_GET(DT_ZEPHYR_DISPLAY(n));

int main(void)
{
	const struct device *display_dev[DT_ZEPHYR_DISPLAYS_COUNT];
	lv_display_t *lv_displays[DT_ZEPHYR_DISPLAYS_COUNT];
	lv_display_t *d = NULL;

	/* clang-format off */
	FOR_EACH(ENUMERATE_DISPLAY_DEVS, (), LV_DISPLAYS_IDX_LIST);
	/* clang-format on */

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		if (!device_is_ready(display_dev[i])) {
			LOG_ERR("Display device %d is not ready", i);
			return 0;
		}
	}

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		d = lv_display_get_next(d);
		if (d == NULL) {
			LOG_ERR("Invalid LV display %d object", i);
			return 0;
		}
		lv_displays[i] = d;
	}

	lv_display_set_default(lv_displays[0]);
#ifdef CONFIG_LV_Z_DEMO_MUSIC_FIRST_DISP
	lv_demo_music();
#elif defined(CONFIG_LV_Z_DEMO_BENCHMARK_FIRST_DISP)
	lv_demo_benchmark();
#elif defined(CONFIG_LV_Z_DEMO_STRESS_FIRST_DISP)
	lv_demo_stress();
#elif defined(CONFIG_LV_Z_DEMO_WIDGETS_FIRST_DISP)
	lv_demo_widgets();
#endif

#if DT_ZEPHYR_DISPLAYS_COUNT > 1
	for (int i = 1; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		lv_display_set_default(lv_displays[i]);
#ifdef CONFIG_LV_Z_DEMO_MUSIC_OTHER_DISPS
		lv_demo_music();
#elif defined(CONFIG_LV_Z_DEMO_BENCHMARK_OTHER_DISPS)
		lv_demo_benchmark();
#elif defined(CONFIG_LV_Z_DEMO_STRESS_OTHER_DISPS)
		lv_demo_stress();
#elif defined(CONFIG_LV_Z_DEMO_WIDGETS_OTHER_DISPS)
		lv_demo_widgets();
#endif
	}
#endif

	lv_timer_handler();
	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		display_blanking_off(display_dev[i]);
	}

#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
	lvgl_print_heap_info(false);
#else
	printf("lvgl in malloc mode\n");
#endif
	while (1) {
		uint32_t sleep_ms = lv_timer_handler();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}

	return 0;
}

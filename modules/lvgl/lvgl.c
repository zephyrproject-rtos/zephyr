/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <lvgl.h>
#include "lvgl_display.h"
#include "lvgl_common_input.h"
#include "lvgl_zephyr.h"
#ifdef CONFIG_LV_Z_USE_FILESYSTEM
#include "lvgl_fs.h"
#endif
#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
#include "lvgl_mem.h"
#endif
#include LV_STDLIB_INCLUDE

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lvgl, CONFIG_LV_Z_LOG_LEVEL);

#define IS_MONOCHROME_DISPLAY                                                                      \
	UTIL_OR(IS_EQ(CONFIG_LV_Z_BITS_PER_PIXEL, 1), IS_EQ(CONFIG_LV_COLOR_DEPTH_1, 1))
#define ALLOC_MONOCHROME_CONV_BUFFER                                                               \
	UTIL_AND(IS_EQ(IS_MONOCHROME_DISPLAY, 1),                                                  \
		 IS_EQ(CONFIG_LV_Z_MONOCHROME_CONVERSION_BUFFER, 1))

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays)
#define LV_ZEPHYR_DISPLAY_NODE(n) DT_ZEPHYR_DISPLAY(n)
#elif DT_HAS_CHOSEN(zephyr_display)
#define LV_ZEPHYR_DISPLAY_NODE(n) DT_CHOSEN(zephyr_display)
#else
#error Could not find "zephyr,display" chosen property, or a node with "zephyr,displays" compatible
#define LV_ZEPHYR_DISPLAY_NODE(n) DT_INVALID_NODE
#endif /* DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays) */

#define DISPLAY_WIDTH(n)  DT_PROP(LV_ZEPHYR_DISPLAY_NODE(n), width)
#define DISPLAY_HEIGHT(n) DT_PROP(LV_ZEPHYR_DISPLAY_NODE(n), height)

#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC
#if IS_MONOCHROME_DISPLAY
/* monochrome buffers are expected to have 8 preceding bytes for the color palette */
#define BUFFER_SIZE(n)                                                                             \
	(((CONFIG_LV_Z_VDB_SIZE * ROUND_UP(DISPLAY_WIDTH(n), 8) *                                  \
	   ROUND_UP(DISPLAY_HEIGHT(n), 8)) /                                                       \
	  100) / 8 +                                                                               \
	 8)
#else
#define BUFFER_SIZE(n)                                                                             \
	(CONFIG_LV_Z_BITS_PER_PIXEL *                                                              \
	 ((CONFIG_LV_Z_VDB_SIZE * DISPLAY_WIDTH(n) * DISPLAY_HEIGHT(n)) / 100) / 8)
#endif /* IS_MONOCHROME_DISPLAY */
#endif /* CONFIG_LV_Z_BUFFER_ALLOC_STATIC */

/* NOTE: depending on chosen color depth buffer may be accessed using uint8_t *,*/
/* uint16_t * or uint32_t *, therefore buffer needs to be aligned accordingly to */
/* prevent unaligned memory accesses. */

#define LV_DISPLAY_MACRO(n)                                                                        \
	struct lvgl_disp_data disp_data_##n = {                                                    \
		.display_dev = NULL,                                                               \
		.blanking_on = false,                                                              \
	};                                                                                         \
                                                                                                   \
	IF_ENABLED(CONFIG_LV_Z_BUFFER_ALLOC_STATIC, (						   \
												   \
	static uint8_t buf0_##n[BUFFER_SIZE(n)]							   \
	IF_ENABLED(CONFIG_LV_Z_VDB_CUSTOM_SECTION, (Z_GENERIC_SECTION(.lvgl_buf)))		   \
			__aligned(CONFIG_LV_Z_VDB_ALIGN);					   \
												   \
	IF_ENABLED(CONFIG_LV_Z_DOUBLE_VDB, (							   \
	static uint8_t buf1_##n[BUFFER_SIZE(n)]							   \
	IF_ENABLED(CONFIG_LV_Z_VDB_CUSTOM_SECTION, (Z_GENERIC_SECTION(.lvgl_buf)))		   \
			__aligned(CONFIG_LV_Z_VDB_ALIGN);					   \
	))											   \
												   \
	IF_ENABLED(ALLOC_MONOCHROME_CONV_BUFFER, (						   \
	static uint8_t mono_vtile_buf_##n[BUFFER_SIZE(n)]					   \
	IF_ENABLED(CONFIG_LV_Z_VDB_CUSTOM_SECTION, (Z_GENERIC_SECTION(.lvgl_buf)))		   \
			__aligned(CONFIG_LV_Z_VDB_ALIGN);					   \
	))											   \
	))

FOR_EACH(LV_DISPLAY_MACRO, (), LV_DISPLAYS_IDX_LIST)
	;

#if CONFIG_LV_Z_LOG_LEVEL != 0
static void lvgl_log(lv_log_level_t level, const char *buf)
{
	switch (level) {
	case LV_LOG_LEVEL_ERROR:
		LOG_ERR("%s", buf + (sizeof("[Error] ") - 1));
		break;
	case LV_LOG_LEVEL_WARN:
		LOG_WRN("%s", buf + (sizeof("[Warn] ") - 1));
		break;
	case LV_LOG_LEVEL_INFO:
		LOG_INF("%s", buf + (sizeof("[Info] ") - 1));
		break;
	case LV_LOG_LEVEL_TRACE:
		LOG_DBG("%s", buf + (sizeof("[Trace] ") - 1));
		break;
	case LV_LOG_LEVEL_USER:
		LOG_INF("%s", buf + (sizeof("[User] ") - 1));
		break;
	}
}
#endif

#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC

#define LVGL_ALLOCATE_RENDERING_BUFFERS(display, n)                                                \
	COND_CODE_1(CONFIG_LV_Z_DOUBLE_VDB,							   \
		(lv_display_set_buffers(display, &buf0_##n, &buf1_##n, BUFFER_SIZE(n),		   \
		LV_DISPLAY_RENDER_MODE_PARTIAL);						   \
	),                                              \
	(lv_display_set_buffers(display, &buf0_##n, NULL, BUFFER_SIZE(n),		   \
		LV_DISPLAY_RENDER_MODE_PARTIAL);						   \
	))                                             \
                                                                                                   \
	IF_ENABLED(ALLOC_MONOCHROME_CONV_BUFFER,						   \
		(lvgl_set_mono_conversion_buffer(mono_vtile_buf_##n, BUFFER_SIZE(n));		   \
	))
#else

static int lvgl_allocate_rendering_buffers(lv_display_t *display)
{
	void *buf0 = NULL;
	void *buf1 = NULL;
	uint16_t buf_nbr_pixels;
	uint32_t buf_size;
	struct lvgl_disp_data *data = (struct lvgl_disp_data *)lv_display_get_user_data(display);
	uint16_t hor_res = lv_display_get_horizontal_resolution(display);
	uint16_t ver_res = lv_display_get_vertical_resolution(display);

	buf_nbr_pixels = (CONFIG_LV_Z_VDB_SIZE * hor_res * ver_res) / 100;
	/* one horizontal line is the minimum buffer requirement for lvgl */
	if (buf_nbr_pixels < hor_res) {
		buf_nbr_pixels = hor_res;
	}

	switch (data->cap.current_pixel_format) {
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
		buf_size = buf_nbr_pixels / 8 + 8;
		buf_size += (buf_nbr_pixels % 8) == 0 ? 0 : 1;
		break;
	default:
		return -ENOTSUP;
	}

	buf0 = lv_malloc(buf_size);
	if (buf0 == NULL) {
		LOG_ERR("Failed to allocate memory for rendering buffer");
		return -ENOMEM;
	}

#ifdef CONFIG_LV_Z_DOUBLE_VDB
	buf1 = lv_malloc(buf_size);
	if (buf1 == NULL) {
		lv_free(buf0);
		LOG_ERR("Failed to allocate memory for rendering buffer");
		return -ENOMEM;
	}
#endif /* CONFIG_LV_Z_DOUBLE_VDB */

#if ALLOC_MONOCHROME_CONV_BUFFER
	void *vtile_buf = lv_malloc(buf_size);

	if (vtile_buf == NULL) {
		lv_free(buf0);
		lv_free(buf1);
		LOG_ERR("Failed to allocate memory for vtile buffer");
		return -ENOMEM;
	}
	lvgl_set_mono_conversion_buffer(vtile_buf, buf_size);
#endif /* ALLOC_MONOCHROME_CONV_BUFFER */

	lv_display_set_buffers(display, buf0, buf1, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
	return 0;
}
#endif /* CONFIG_LV_Z_BUFFER_ALLOC_STATIC */

void lv_mem_init(void)
{
#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
	lvgl_heap_init();
#endif /* CONFIG_LV_Z_MEM_POOL_SYS_HEAP */
}

void lv_mem_deinit(void)
{
	/* Reinitializing the heap clears all allocations, no action needed */
}

void lv_mem_monitor_core(lv_mem_monitor_t *mon_p)
{
	memset(mon_p, 0, sizeof(lv_mem_monitor_t));

#if CONFIG_LV_Z_MEM_POOL_SYS_HEAP
	struct sys_memory_stats stats;

	lvgl_heap_stats(&stats);
	mon_p->used_pct =
		(stats.allocated_bytes * 100) / (stats.allocated_bytes + stats.free_bytes);
	mon_p->max_used = stats.max_allocated_bytes;
#else
	LOG_WRN_ONCE("Memory statistics only supported for CONFIG_LV_Z_MEM_POOL_SYS_HEAP");
#endif /* CONFIG_LV_Z_MEM_POOL_SYS_HEAP */
}

lv_result_t lv_mem_test_core(void)
{
	/* Not supported for now */
	return LV_RESULT_OK;
}

lv_display_t *lvgl_display[DT_ZEPHYR_DISPLAYS_COUNT] = {NULL};

#define LV_DISPLAY_INIT(n)                                                                         \
	const struct device *display_dev_##n = DEVICE_DT_GET(LV_ZEPHYR_DISPLAY_NODE(n));           \
	if (!device_is_ready(display_dev_##n)) {                                                   \
		printk("Display device not ready.\n");                                             \
		return -ENODEV;                                                                    \
	}                                                                                          \
	disp_data_##n.display_dev = display_dev_##n;                                               \
	display_get_capabilities(display_dev_##n, &disp_data_##n.cap);                             \
	lvgl_display[n] =                                                                          \
		lv_display_create(disp_data_##n.cap.x_resolution, disp_data_##n.cap.y_resolution); \
	if (lvgl_display[n] == NULL) {                                                             \
		printk("Failed to create LV display object.\n");                                   \
		return -ENOMEM;                                                                    \
	}                                                                                          \
	lv_display_set_user_data(lvgl_display[n], &disp_data_##n);                                 \
	if (set_lvgl_rendering_cb(lvgl_display[n]) != 0) {                                         \
		printk("Display is not supported.\n");                                             \
		return -ENOTSUP;                                                                   \
	}                                                                                          \
                                                                                                   \
	COND_CODE_1(CONFIG_LV_Z_BUFFER_ALLOC_STATIC,						   \
			(LVGL_ALLOCATE_RENDERING_BUFFERS(lvgl_display[n], n)),                     \
		(err = lvgl_allocate_rendering_buffers(lvgl_display[n]) < 0;		   \
		if (err < 0)                                                                       \
			return err;			                                   \
		))                                     \
	IF_ENABLED(CONFIG_LV_Z_FULL_REFRESH,							   \
		(lv_display_set_render_mode(lvgl_display[n], LV_DISPLAY_RENDER_MODE_FULL);))

int lvgl_init(void)
{
	int err = 0;

#if CONFIG_LV_Z_LOG_LEVEL != 0
	lv_log_register_print_cb(lvgl_log);
#endif

	LOG_DBG("LVGL initialize");
	lv_init();
	lv_tick_set_cb(k_uptime_get_32);

#ifdef CONFIG_LV_Z_USE_FILESYSTEM
	lvgl_fs_init();
#endif /* CONFIG_LV_Z_USE_FILESYSTEM */

	FOR_EACH(LV_DISPLAY_INIT, (), LV_DISPLAYS_IDX_LIST)
		;

	err = lvgl_init_input_devices();
	if (err < 0) {
		LOG_ERR("Failed to initialize input devices.");
		return err;
	}

	return 0;
}

#ifdef CONFIG_LV_Z_AUTO_INIT
SYS_INIT(lvgl_init, APPLICATION, CONFIG_LV_Z_INIT_PRIORITY);
#endif /* CONFIG_LV_Z_AUTO_INIT */

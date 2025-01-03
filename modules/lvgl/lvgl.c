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

static lv_display_t *display;
struct lvgl_disp_data disp_data = {
	.blanking_on = false,
};

#define DISPLAY_NODE          DT_CHOSEN(zephyr_display)
#define IS_MONOCHROME_DISPLAY ((CONFIG_LV_Z_BITS_PER_PIXEL == 1) || (CONFIG_LV_COLOR_DEPTH_1 == 1))
#define ALLOC_MONOCHROME_CONV_BUFFER                                                               \
	((IS_MONOCHROME_DISPLAY == 1) && (CONFIG_LV_Z_MONOCHROME_CONVERSION_BUFFER == 1))

#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC

#define DISPLAY_WIDTH  DT_PROP(DISPLAY_NODE, width)
#define DISPLAY_HEIGHT DT_PROP(DISPLAY_NODE, height)

#if IS_MONOCHROME_DISPLAY
/* monochrome buffers are expected to have 8 preceding bytes for the color palette */
#define BUFFER_SIZE                                                                                \
	(((CONFIG_LV_Z_VDB_SIZE * ROUND_UP(DISPLAY_WIDTH, 8) * ROUND_UP(DISPLAY_HEIGHT, 8)) /      \
	  100) / 8 +                                                                               \
	 8)
#else
#define BUFFER_SIZE                                                                                \
	(CONFIG_LV_Z_BITS_PER_PIXEL *                                                              \
	 ((CONFIG_LV_Z_VDB_SIZE * DISPLAY_WIDTH * DISPLAY_HEIGHT) / 100) / 8)
#endif /* IS_MONOCHROME_DISPLAY */

/* NOTE: depending on chosen color depth buffer may be accessed using uint8_t *,
 * uint16_t * or uint32_t *, therefore buffer needs to be aligned accordingly to
 * prevent unaligned memory accesses.
 */
static uint8_t buf0[BUFFER_SIZE]
#ifdef CONFIG_LV_Z_VBD_CUSTOM_SECTION
	Z_GENERIC_SECTION(.lvgl_buf)
#endif
		__aligned(CONFIG_LV_Z_VDB_ALIGN);

#ifdef CONFIG_LV_Z_DOUBLE_VDB
static uint8_t buf1[BUFFER_SIZE]
#ifdef CONFIG_LV_Z_VBD_CUSTOM_SECTION
	Z_GENERIC_SECTION(.lvgl_buf)
#endif
		__aligned(CONFIG_LV_Z_VDB_ALIGN);
#endif /* CONFIG_LV_Z_DOUBLE_VDB */

#if ALLOC_MONOCHROME_CONV_BUFFER
static uint8_t mono_vtile_buf[BUFFER_SIZE]
#ifdef CONFIG_LV_Z_VBD_CUSTOM_SECTION
	Z_GENERIC_SECTION(.lvgl_buf)
#endif
		__aligned(CONFIG_LV_Z_VDB_ALIGN);
#endif /* ALLOC_MONOCHROME_CONV_BUFFER */

#endif /* CONFIG_LV_Z_BUFFER_ALLOC_STATIC */

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

static int lvgl_allocate_rendering_buffers(lv_display_t *display)
{
	int err = 0;

#ifdef CONFIG_LV_Z_DOUBLE_VDB
	lv_display_set_buffers(display, &buf0, &buf1, BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
#else
	lv_display_set_buffers(display, &buf0, NULL, BUFFER_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif /* CONFIG_LV_Z_DOUBLE_VDB  */

#if ALLOC_MONOCHROME_CONV_BUFFER
	lvgl_set_mono_conversion_buffer(mono_vtile_buf, BUFFER_SIZE);
#endif /* ALLOC_MONOCHROME_CONV_BUFFER */

	return err;
}

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
#endif

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

int lvgl_init(void)
{
	const struct device *display_dev = DEVICE_DT_GET(DISPLAY_NODE);

	int err = 0;

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready.");
		return -ENODEV;
	}

#if CONFIG_LV_Z_LOG_LEVEL != 0
	lv_log_register_print_cb(lvgl_log);
#endif

	lv_init();
	lv_tick_set_cb(k_uptime_get_32);

#ifdef CONFIG_LV_Z_USE_FILESYSTEM
	lvgl_fs_init();
#endif

	disp_data.display_dev = display_dev;
	display_get_capabilities(display_dev, &disp_data.cap);

	display = lv_display_create(disp_data.cap.x_resolution, disp_data.cap.y_resolution);
	if (!display) {
		return -ENOMEM;
	}
	lv_display_set_user_data(display, &disp_data);

	if (set_lvgl_rendering_cb(display) != 0) {
		LOG_ERR("Display not supported.");
		return -ENOTSUP;
	}

	err = lvgl_allocate_rendering_buffers(display);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_LV_Z_FULL_REFRESH
	lv_display_set_render_mode(display, LV_DISPLAY_RENDER_MODE_FULL);
#endif

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

/*
 * Copyright (c) 2018-2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2025 Abderrahmane JARMOUNI
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

static lv_display_t *lv_displays[DT_ZEPHYR_DISPLAYS_COUNT];
struct lvgl_disp_data disp_data[DT_ZEPHYR_DISPLAYS_COUNT] = {{
	.blanking_on = false,
}};

#define DISPLAY_BUFFER_ALIGN(alignbytes) __aligned(alignbytes)

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_displays)
#define DISPLAY_NODE(n) DT_ZEPHYR_DISPLAY(n)
#elif DT_HAS_CHOSEN(zephyr_display)
#define DISPLAY_NODE(n) DT_CHOSEN(zephyr_display)
#else
#error Could not find "zephyr,display" chosen property, or a "zephyr,displays" compatible node in DT
#define DISPLAY_NODE(n) DT_INVALID_NODE
#endif

#define IS_MONOCHROME_DISPLAY                                                                      \
	UTIL_OR(IS_EQ(CONFIG_LV_Z_BITS_PER_PIXEL, 1), IS_EQ(CONFIG_LV_COLOR_DEPTH_1, 1))

#define ALLOC_MONOCHROME_CONV_BUFFER                                                               \
	UTIL_AND(IS_EQ(IS_MONOCHROME_DISPLAY, 1),                                                  \
		 IS_EQ(CONFIG_LV_Z_MONOCHROME_CONVERSION_BUFFER, 1))

#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC

#define DISPLAY_WIDTH(n)  DT_PROP(DISPLAY_NODE(n), width)
#define DISPLAY_HEIGHT(n) DT_PROP(DISPLAY_NODE(n), height)

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

static uint32_t disp_buf_size[DT_ZEPHYR_DISPLAYS_COUNT] = {0};
static uint8_t *buf0_p[DT_ZEPHYR_DISPLAYS_COUNT] = {NULL};

#ifdef CONFIG_LV_Z_DOUBLE_VDB
static uint8_t *buf1_p[DT_ZEPHYR_DISPLAYS_COUNT] = {NULL};
#endif

#if ALLOC_MONOCHROME_CONV_BUFFER
static uint8_t *mono_vtile_buf_p[DT_ZEPHYR_DISPLAYS_COUNT] = {NULL};
#endif

/* NOTE: depending on chosen color depth, buffers may be accessed using uint8_t *,*/
/* uint16_t * or uint32_t *, therefore buffer needs to be aligned accordingly to */
/* prevent unaligned memory accesses. */

/* clang-format off */
#define LV_BUFFERS_DEFINE(n)									\
	static DISPLAY_BUFFER_ALIGN(LV_DRAW_BUF_ALIGN) uint8_t buf0_##n[BUFFER_SIZE(n)]		\
	IF_ENABLED(CONFIG_LV_Z_VDB_CUSTOM_SECTION, (Z_GENERIC_SECTION(.lvgl_buf)))		\
						       __aligned(CONFIG_LV_Z_VDB_ALIGN);	\
												\
	IF_ENABLED(CONFIG_LV_Z_DOUBLE_VDB, (							\
	static DISPLAY_BUFFER_ALIGN(LV_DRAW_BUF_ALIGN) uint8_t buf1_##n[BUFFER_SIZE(n)]		\
	IF_ENABLED(CONFIG_LV_Z_VDB_CUSTOM_SECTION, (Z_GENERIC_SECTION(.lvgl_buf)))		\
			__aligned(CONFIG_LV_Z_VDB_ALIGN);					\
	))											\
												\
	IF_ENABLED(ALLOC_MONOCHROME_CONV_BUFFER, (						\
	static uint8_t mono_vtile_buf_##n[BUFFER_SIZE(n)]					\
	IF_ENABLED(CONFIG_LV_Z_VDB_CUSTOM_SECTION, (Z_GENERIC_SECTION(.lvgl_buf)))		\
			__aligned(CONFIG_LV_Z_VDB_ALIGN);					\
	))

FOR_EACH(LV_BUFFERS_DEFINE, (), LV_DISPLAYS_IDX_LIST);

#define LV_BUFFERS_REFERENCES(n)                                                                   \
	disp_buf_size[n] = (uint32_t)BUFFER_SIZE(n);                                               \
	buf0_p[n] = buf0_##n;                                                                      \
	IF_ENABLED(CONFIG_LV_Z_DOUBLE_VDB, (buf1_p[n] = buf1_##n;))                                \
	IF_ENABLED(ALLOC_MONOCHROME_CONV_BUFFER, (mono_vtile_buf_p[n] = mono_vtile_buf_##n;))
/* clang-format on */

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

static void lvgl_allocate_rendering_buffers_static(lv_display_t *display, int disp_idx)
{
#ifdef CONFIG_LV_Z_DOUBLE_VDB
	lv_display_set_buffers(display, buf0_p[disp_idx], buf1_p[disp_idx], disp_buf_size[disp_idx],
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
#else
	lv_display_set_buffers(display, buf0_p[disp_idx], NULL, disp_buf_size[disp_idx],
			       LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif /* CONFIG_LV_Z_DOUBLE_VDB */

#if ALLOC_MONOCHROME_CONV_BUFFER
	lvgl_set_mono_conversion_buffer(mono_vtile_buf_p[disp_idx], disp_buf_size[disp_idx]);
#endif
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
	case PIXEL_FORMAT_L_8:
		buf_size = buf_nbr_pixels;
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

#ifdef CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE

static K_THREAD_STACK_DEFINE(lvgl_workqueue_stack, CONFIG_LV_Z_LVGL_WORKQUEUE_STACK_SIZE);
static struct k_work_q lvgl_workqueue;

static void lvgl_timer_handler_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	uint32_t wait_time;

	lvgl_lock();
	wait_time = lv_timer_handler();
	lvgl_unlock();

	/* schedule next timer verification */
	if (wait_time == LV_NO_TIMER_READY) {
		wait_time = CONFIG_LV_DEF_REFR_PERIOD;
	}

	k_work_schedule_for_queue(&lvgl_workqueue, dwork, K_MSEC(wait_time));
}
static K_WORK_DELAYABLE_DEFINE(lvgl_work, lvgl_timer_handler_work);

struct k_work_q *lvgl_get_workqueue(void)
{
	return &lvgl_workqueue;
}
#endif /* CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE */

#if defined(CONFIG_LV_Z_LVGL_MUTEX) && !defined(CONFIG_LV_Z_USE_OSAL)

static K_MUTEX_DEFINE(lvgl_mutex);

void lvgl_lock(void)
{
	(void)k_mutex_lock(&lvgl_mutex, K_FOREVER);
}

bool lvgl_trylock(void)
{
	return k_mutex_lock(&lvgl_mutex, K_NO_WAIT) == 0;
}

void lvgl_unlock(void)
{
	(void)k_mutex_unlock(&lvgl_mutex);
}

#endif /* CONFIG_LV_Z_LVGL_MUTEX */

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

#define ENUMERATE_DISPLAY_DEVS(n) display_dev[n] = DEVICE_DT_GET(DISPLAY_NODE(n));

int lvgl_init(void)
{
	const struct device *display_dev[DT_ZEPHYR_DISPLAYS_COUNT];
	struct lvgl_disp_data *p_disp_data;
	int err;

	/* clang-format off */
	FOR_EACH(ENUMERATE_DISPLAY_DEVS, (), LV_DISPLAYS_IDX_LIST);
	/* clang-format on */
	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		if (!device_is_ready(display_dev[i])) {
			LOG_ERR("Display device %d is not ready", i);
			return -ENODEV;
		}
	}

	lv_init();
	lv_tick_set_cb(k_uptime_get_32);

#if CONFIG_LV_Z_LOG_LEVEL != 0
	lv_log_register_print_cb(lvgl_log);
#endif

#ifdef CONFIG_LV_Z_USE_FILESYSTEM
	lvgl_fs_init();
#endif

#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC
	/* clang-format off */
	FOR_EACH(LV_BUFFERS_REFERENCES, (), LV_DISPLAYS_IDX_LIST);
	/* clang-format on */
#endif

	for (int i = 0; i < DT_ZEPHYR_DISPLAYS_COUNT; i++) {
		p_disp_data = &disp_data[i];
		p_disp_data->display_dev = display_dev[i];
		display_get_capabilities(display_dev[i], &p_disp_data->cap);

		lv_displays[i] = lv_display_create(p_disp_data->cap.x_resolution,
						   p_disp_data->cap.y_resolution);
		if (!lv_displays[i]) {
			LOG_ERR("Failed to create display %d LV object.", i);
			return -ENOMEM;
		}

		lv_display_set_user_data(lv_displays[i], p_disp_data);
		if (set_lvgl_rendering_cb(lv_displays[i]) != 0) {
			LOG_ERR("Display %d not supported.", i);
			return -ENOTSUP;
		}

#ifdef CONFIG_LV_Z_BUFFER_ALLOC_STATIC
		lvgl_allocate_rendering_buffers_static(lv_displays[i], i);
#else
		err = lvgl_allocate_rendering_buffers(lv_displays[i]);
		if (err < 0) {
			return err;
		}
#endif

#ifdef CONFIG_LV_Z_FULL_REFRESH
		lv_display_set_render_mode(lv_displays[i], LV_DISPLAY_RENDER_MODE_FULL);
#endif
	}

	err = lvgl_init_input_devices();
	if (err < 0) {
		LOG_ERR("Failed to initialize input devices.");
		return err;
	}

#ifdef CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE
	const struct k_work_queue_config lvgl_workqueue_cfg = {
		.name = "lvgl",
	};

	k_work_queue_init(&lvgl_workqueue);
	k_work_queue_start(&lvgl_workqueue, lvgl_workqueue_stack,
			   K_THREAD_STACK_SIZEOF(lvgl_workqueue_stack),
			   CONFIG_LV_Z_LVGL_WORKQUEUE_PRIORITY, &lvgl_workqueue_cfg);

	k_work_submit_to_queue(&lvgl_workqueue, &lvgl_work.work);
#endif

	return 0;
}

#ifdef CONFIG_LV_Z_AUTO_INIT
SYS_INIT(lvgl_init, APPLICATION, CONFIG_LV_Z_INIT_PRIORITY);
#endif /* CONFIG_LV_Z_AUTO_INIT */

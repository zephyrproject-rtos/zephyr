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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lvgl, CONFIG_LV_Z_LOG_LEVEL);


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

int lvgl_init(void)
{
	int err;

	lv_init();
	lv_tick_set_cb(k_uptime_get_32);

#if CONFIG_LV_Z_LOG_LEVEL != 0
	lv_log_register_print_cb(lvgl_log);
#endif

#ifdef CONFIG_LV_Z_USE_FILESYSTEM
	lvgl_fs_init();
#endif

	err = lvgl_display_init();
	if (err < 0) {
		LOG_ERR("Failed to initialize display devices.");
		return err;
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

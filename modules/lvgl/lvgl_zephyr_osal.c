/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_zephyr_osal.h"
#include <lvgl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lvgl, CONFIG_LV_Z_LOG_LEVEL);

typedef void (*lv_thread_entry)(void *);
static void thread_entry(void *thread, void *cb, void *user_data);

lv_result_t lv_thread_init(lv_thread_t *thread, const char *const name, lv_thread_prio_t prio,
			   void (*callback)(void *), size_t stack_size, void *user_data)
{
	int thread_priority;

	thread->stack = k_thread_stack_alloc(stack_size, 0);
	if (thread->stack == NULL) {
		return LV_RESULT_INVALID;
	}

	thread_priority = (CONFIG_NUM_PREEMPT_PRIORITIES - 1) -
			  ((prio * (CONFIG_NUM_PREEMPT_PRIORITIES - 1)) / LV_THREAD_PRIO_HIGHEST);

	thread->tid = k_thread_create(&thread->thread, thread->stack, stack_size, thread_entry,
				      thread, callback, user_data, thread_priority, 0, K_NO_WAIT);

	k_thread_name_set(thread->tid, name);

	return LV_RESULT_OK;
}

lv_result_t lv_thread_delete(lv_thread_t *thread)
{
	int ret;

	k_thread_abort(thread->tid);
	ret = k_thread_stack_free(thread->stack);
	if (ret < 0) {
		LOG_ERR("Failled to delete thread: %d", ret);
		return LV_RESULT_INVALID;
	}

	return LV_RESULT_OK;
}

lv_result_t lv_mutex_init(lv_mutex_t *mutex)
{
	k_mutex_init(mutex);
	return LV_RESULT_OK;
}

lv_result_t lv_mutex_lock(lv_mutex_t *mutex)
{
	int ret;

	ret = k_mutex_lock(mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock mutex: %d", ret);
		return LV_RESULT_INVALID;
	}

	return LV_RESULT_OK;
}

lv_result_t lv_mutex_lock_isr(lv_mutex_t *mutex)
{
	int ret;

	ret = k_mutex_lock(mutex, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Failed to lock mutex: %d", ret);
		return LV_RESULT_INVALID;
	}

	return LV_RESULT_OK;
}

lv_result_t lv_mutex_unlock(lv_mutex_t *mutex)
{
	int ret;

	ret = k_mutex_unlock(mutex);
	if (ret != 0) {
		LOG_ERR("Failed to unlock mutex: %d", ret);
		return LV_RESULT_INVALID;
	}

	return LV_RESULT_OK;
}

lv_result_t lv_mutex_delete(lv_mutex_t *mutex)
{
	ARG_UNUSED(mutex);
	return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_init(lv_thread_sync_t *sync)
{
	int ret;

	ret = k_sem_init(sync, 0, 1);
	if (ret != 0) {
		LOG_ERR("Failed to init thread sync: %d", ret);
		return LV_RESULT_INVALID;
	}

	return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_wait(lv_thread_sync_t *sync)
{
	int ret;

	ret = k_sem_take(sync, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Error waiting on thread sync: %d", ret);
		return LV_RESULT_INVALID;
	}

	return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_signal(lv_thread_sync_t *sync)
{
	k_sem_give(sync);
	return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_signal_isr(lv_thread_sync_t *sync)
{
	k_sem_give(sync);
	return LV_RESULT_OK;
}

lv_result_t lv_thread_sync_delete(lv_thread_sync_t *sync)
{
	ARG_UNUSED(sync);
	return LV_RESULT_OK;
}

void thread_entry(void *thread, void *cb, void *user_data)
{
	__ASSERT_NO_MSG(cb != NULL);
	lv_thread_entry entry_cb = (lv_thread_entry)cb;

	entry_cb(user_data);
	lv_thread_delete((lv_thread_t *)thread);
}

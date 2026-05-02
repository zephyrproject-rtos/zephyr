/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_LVGL_ZEPHYR_OSAL_H_
#define ZEPHYR_MODULES_LVGL_ZEPHYR_OSAL_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	k_tid_t tid;
	k_thread_stack_t *stack;
	struct k_thread thread;
} lv_thread_t;

typedef struct k_mutex lv_mutex_t;

typedef struct k_sem lv_thread_sync_t;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_LVGL_ZEPHYR_OSAL_H_ */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WRAPPER_H__
#define __WRAPPER_H__

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_types.h>
#include <zephyr/portability/cmsis_os2.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

extern osThreadId_t get_cmsis_thread_id(k_tid_t tid);
extern void *is_cmsis_rtos_v2_thread(void *thread_id);

#endif

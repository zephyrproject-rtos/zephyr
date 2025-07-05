/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_H__
#define __OS_WRAPPER_H__

/**
 * @brief  Necessary headers
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "platform_autoconf.h"
#include "ameba.h"
#include <zephyr/kernel.h>

/**
 * @brief  Common header file
 */
#ifdef __cplusplus
extern "C" {
#endif
#include "os_wrapper_semaphore.h"
#include "os_wrapper_critical.h"
#include "os_wrapper_memory.h"
#include "os_wrapper_mutex.h"
#include "os_wrapper_queue.h"
#include "os_wrapper_semaphore.h"
#include "os_wrapper_task.h"
#include "os_wrapper_time.h"
#include "os_wrapper_timer.h"
#ifdef __cplusplus
}
#endif

/**
 * @brief  General macro definition
 */

#define RTOS_MAX_DELAY   0xFFFFFFFFUL
#define RTOS_MAX_TIMEOUT 0xFFFFFFFFUL

#endif

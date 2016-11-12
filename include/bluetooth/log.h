/** @file
 *  @brief Bluetooth subsystem logging helpers.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BT_LOG_H
#define __BT_LOG_H

#include <sections.h>
#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BLUETOOTH_DEBUG_MONITOR)
#include <stdio.h>

/* These defines follow the values used by syslog(2) */
#define BT_LOG_ERR      3
#define BT_LOG_WARN     4
#define BT_LOG_INFO     6
#define BT_LOG_DBG      7

void bt_log(int prio, const char *fmt, ...);

#define BT_DBG(fmt, ...) bt_log(BT_LOG_DBG, "%s (%p): " fmt, \
				__func__, k_current_get(), ##__VA_ARGS__)
#define BT_ERR(fmt, ...) bt_log(BT_LOG_ERR, "%s: " fmt, \
				__func__, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) bt_log(BT_LOG_WARN, "%s: " fmt, \
				 __func__, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) bt_log(BT_LOG_INFO, fmt, ##__VA_ARGS__)

/* Enabling debug increases stack size requirement considerably */
#define BT_STACK_DEBUG_EXTRA	512

#elif defined(CONFIG_BLUETOOTH_DEBUG_LOG)

#define SYS_LOG_DOMAIN "bt"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <misc/sys_log.h>

#define BT_DBG(fmt, ...) SYS_LOG_DBG("(%p) " fmt, k_current_get(), \
				##__VA_ARGS__)
#define BT_ERR(fmt, ...) SYS_LOG_ERR(fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) SYS_LOG_WRN(fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) SYS_LOG_INF(fmt, ##__VA_ARGS__)

/* Enabling debug increases stack size requirement considerably */
#define BT_STACK_DEBUG_EXTRA	512

#else

#define BT_DBG(fmt, ...)
#define BT_ERR(fmt, ...)
#define BT_WARN(fmt, ...)
#define BT_INFO(fmt, ...)

#define BT_STACK_DEBUG_EXTRA	0

#endif

#define BT_ASSERT(cond) if (!(cond)) { \
				BT_ERR("assert: '" #cond "' failed"); \
				_SysFatalErrorHandler(0, NULL); \
			}

#define BT_STACK(name, size) \
		char __stack name[(size) + BT_STACK_DEBUG_EXTRA]
#define BT_STACK_NOINIT(name, size) \
		char __noinit __stack name[(size) + BT_STACK_DEBUG_EXTRA]

#if defined(CONFIG_BLUETOOTH_DEBUG)
const char *bt_hex(const void *buf, size_t len);
#else
#define bt_hex(buf, len)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __BT_LOG_H */

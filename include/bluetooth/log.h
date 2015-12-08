/** @file
 *  @brief Bluetooth subsystem logging helpers.
 */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <stdio.h>

#if defined(CONFIG_BLUETOOTH_DEBUG_COLOR)
#define BT_COLOR_OFF     "\x1B[0m"
#define BT_COLOR_RED     "\x1B[0;31m"
#define BT_COLOR_YELLOW  "\x1B[0;33m"
#else
#define BT_COLOR_OFF     ""
#define BT_COLOR_RED     ""
#define BT_COLOR_YELLOW  ""
#endif

#if defined(CONFIG_BLUETOOTH_DEBUG)
#define BT_DBG(fmt, ...) printf("bt: %s (%p): " fmt "\n", __func__, \
				sys_thread_self_get(), ##__VA_ARGS__)
#define BT_ERR(fmt, ...) printf("bt: %s: %s" fmt "%s\n", __func__, \
				BT_COLOR_RED, ##__VA_ARGS__, BT_COLOR_OFF)
#define BT_WARN(fmt, ...) printf("bt: %s: %s" fmt "%s\n", __func__, \
				 BT_COLOR_YELLOW, ##__VA_ARGS__, BT_COLOR_OFF)
#define BT_INFO(fmt, ...) printf("bt: " fmt "\n", ##__VA_ARGS__)
#define BT_ASSERT(cond) if (!(cond)) { \
				BT_ERR("bt: assert: '" #cond "' failed"); \
			}
#else
#define BT_DBG(fmt, ...)
#define BT_ERR(fmt, ...)
#define BT_WARN(fmt, ...)
#define BT_INFO(fmt, ...)
#define BT_ASSERT(cond)
#endif /* CONFIG_BLUETOOTH_DEBUG */

#endif /* __BT_LOG_H */

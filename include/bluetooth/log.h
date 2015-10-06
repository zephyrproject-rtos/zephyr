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

#if defined(CONFIG_BLUETOOTH_DEBUG)
#define BT_DBG(fmt, ...) printf("bt: %s (%p): " fmt, __func__, \
				sys_thread_self_get(), ##__VA_ARGS__)
#define BT_ERR(fmt, ...) printf("bt: %s: " fmt, __func__, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) printf("bt: %s: " fmt, __func__, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) printf("bt: " fmt,  ##__VA_ARGS__)
#define BT_ASSERT(cond) if (!(cond)) { \
				BT_ERR("bt: assert: '" #cond "' failed\n"); \
			}
#else
#define BT_DBG(fmt, ...)
#define BT_ERR(fmt, ...)
#define BT_WARN(fmt, ...)
#define BT_INFO(fmt, ...)
#define BT_ASSERT(cond)
#endif /* CONFIG_BLUETOOTH_DEBUG */

#endif /* __BT_LOG_H */

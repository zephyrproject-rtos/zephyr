/*
 * Copyright (c) 2015 Intel Corporation.
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

#ifndef DRIVERS_ETHERNET_ETHERNET_H_
#define DRIVERS_ETHERNET_ETHERNET_H_

#include <misc/printk.h>

#ifdef CONFIG_ETHERNET_DEBUG

#define ETH_DBG(fmt, ...) printk("ethernet: %s: " fmt, __func__, ##__VA_ARGS__)
#define ETH_ERR(fmt, ...) printk("ethernet: %s: " fmt, __func__, ##__VA_ARGS__)
#define ETH_INFO(fmt, ...) printk("ethernet: " fmt,  ##__VA_ARGS__)
#define ETH_PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)

#else

#define ETH_DBG(fmt, ...)
#define ETH_ERR(fmt, ...)
#define ETH_INFO(fmt, ...)
#define ETH_PRINT(fmt, ...)

#endif

#endif /* DRIVERS_ETHERNET_ETHERNET_H_ */

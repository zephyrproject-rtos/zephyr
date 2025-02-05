/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ZLOG_H_
#define ZEPHYR_INCLUDE_ZVM_ZLOG_H_

#include <zephyr/kernel.h>

#ifdef CONFIG_LOG

#define ZVM_LOG_ERR(...)	LOG_ERR(__VA_ARGS__)
#define ZVM_LOG_WARN(...)   LOG_WRN(__VA_ARGS__)
#ifdef CONFIG_ZVM_DEBUG_LOG_INFO
#define ZVM_LOG_INFO(...)   LOG_PRINTK(__VA_ARGS__)
#else
#define ZVM_LOG_INFO(...)
#endif

#else
#define ZVM_LOG_ERR(format, ...) \
DEBUG("\033[31m[ERR:]File:%s Line:%d. " format "\n\033[0m", __FILE__, \
__LINE__, ##__VA_ARGS__)

#define ZVM_LOG_WARN(format, ...) \
DEBUG("\033[33m[WRN:]File:%s Line:%d. " format "\n\033[0m", __FILE__, \
__LINE__, ##__VA_ARGS__)

#ifdef CONFIG_ZVM_DEBUG_LOG_INFO
#define ZVM_LOG_INFO(format, ...)	\
DEBUG("\033[34m[INFO:]File:%s Line:%d. " format "\n\033[0m", __FILE__, \
__LINE__, ##__VA_ARGS__)

#else
#define ZVM_LOG_INFO(...)
#endif

#endif

#endif /* ZEPHYR_INCLUDE_ZVM_ZLOG_H_ */

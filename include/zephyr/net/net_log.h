/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Network subsystem logging helpers.
 *
 * These macros are private to the network subsystem and should not be used
 * by the application or other subsystems.
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_LOG_H_
#define ZEPHYR_INCLUDE_NET_NET_LOG_H_

#include <zephyr/logging/log.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_THREAD_NAME
#define NET_DBG(fmt, ...) LOG_DBG("(%s): " fmt,				\
			k_thread_name_get(k_current_get()), \
			##__VA_ARGS__)
#else
#define NET_DBG(fmt, ...) LOG_DBG("(%p): " fmt, k_current_get(),	\
				  ##__VA_ARGS__)
#endif /* CONFIG_THREAD_NAME */
#define NET_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_INFO(fmt, ...) LOG_INF(fmt,  ##__VA_ARGS__)

/* Rate-limited network logging macros */
#define NET_ERR_RATELIMIT(fmt, ...)  LOG_ERR_RATELIMIT(fmt, ##__VA_ARGS__)
#define NET_WARN_RATELIMIT(fmt, ...) LOG_WRN_RATELIMIT(fmt, ##__VA_ARGS__)
#define NET_INFO_RATELIMIT(fmt, ...) LOG_INF_RATELIMIT(fmt, ##__VA_ARGS__)
#define NET_DBG_RATELIMIT(fmt, ...)  LOG_DBG_RATELIMIT(fmt, ##__VA_ARGS__)

#define NET_HEXDUMP_DBG(_data, _length, _str)  LOG_HEXDUMP_DBG(_data, _length, _str)
#define NET_HEXDUMP_ERR(_data, _length, _str)  LOG_HEXDUMP_ERR(_data, _length, _str)
#define NET_HEXDUMP_WARN(_data, _length, _str) LOG_HEXDUMP_WRN(_data, _length, _str)
#define NET_HEXDUMP_INFO(_data, _length, _str) LOG_HEXDUMP_INF(_data, _length, _str)

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_LOG_H_ */

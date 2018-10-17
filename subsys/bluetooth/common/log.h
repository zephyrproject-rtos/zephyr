/** @file
 *  @brief Bluetooth subsystem logging helpers.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_LOG_H
#define __BT_LOG_H

#include <linker/sections.h>
#include <offsets.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(BT_DBG_ENABLED)
#define BT_DBG_ENABLED 1
#endif

#if defined(CONFIG_BT_DEBUG_MONITOR)
#include <stdio.h>

/* These defines follow the values used by syslog(2) */
#define BT_LOG_ERR      3
#define BT_LOG_WARN     4
#define BT_LOG_INFO     6
#define BT_LOG_DBG      7

__printf_like(2, 3) void bt_log(int prio, const char *fmt, ...);

#define BT_DBG(fmt, ...) \
	if (BT_DBG_ENABLED) { \
		bt_log(BT_LOG_DBG, "%s (%p): " fmt, \
		       __func__, k_current_get(), ##__VA_ARGS__); \
	}

#define BT_ERR(fmt, ...) bt_log(BT_LOG_ERR, "%s: " fmt, \
				__func__, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) bt_log(BT_LOG_WARN, "%s: " fmt, \
				 __func__, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) bt_log(BT_LOG_INFO, fmt, ##__VA_ARGS__)

#elif defined(CONFIG_BT_DEBUG_LOG)

#if BT_DBG_ENABLED
#define LOG_LEVEL LOG_LEVEL_DBG
#else
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#endif

#include <logging/log.h>

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if IS_ENABLED(CONFIG_LOG_FUNCTION_NAME)
#define BT_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#else
#define BT_DBG(fmt, ...) LOG_DBG("%s: " fmt, __func__, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) LOG_ERR("%s: " fmt, __func__, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) LOG_WRN("%s: " fmt, __func__, ##__VA_ARGS__)
#endif

#define BT_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)

#else

static inline __printf_like(1, 2) void _bt_log_dummy(const char *fmt, ...) {};

#define BT_DBG(fmt, ...) \
		if (0) { \
			_bt_log_dummy(fmt, ##__VA_ARGS__); \
		}
#define BT_ERR BT_DBG
#define BT_WARN BT_DBG
#define BT_INFO BT_DBG

#endif

#define BT_ASSERT(cond) if (!(cond)) { \
				BT_ERR("assert: '" #cond "' failed"); \
				k_oops(); \
			}

const char *bt_hex_real(const void *buf, size_t len);
const char *bt_addr_str_real(const bt_addr_t *addr);
const char *bt_addr_le_str_real(const bt_addr_le_t *addr);

#define bt_hex(buf, len) log_strdup(bt_hex_real(buf, len))
#define bt_addr_str(addr) log_strdup(bt_addr_str_real(addr))
#define bt_addr_le_str(addr) log_strdup(bt_addr_le_str_real(addr))

#ifdef __cplusplus
}
#endif

#endif /* __BT_LOG_H */


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

#include <zephyr/linker/sections.h>
#include <offsets.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(BT_DBG_ENABLED)
#define BT_DBG_ENABLED 1
#endif

#if BT_DBG_ENABLED
#define LOG_LEVEL LOG_LEVEL_DBG
#else
#define LOG_LEVEL CONFIG_BT_LOG_LEVEL
#endif

LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define BT_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)

#if defined(CONFIG_BT_ASSERT_VERBOSE)
#define BT_ASSERT_PRINT(test) __ASSERT_LOC(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...) __ASSERT_MSG_INFO(fmt, ##__VA_ARGS__)
#else
#define BT_ASSERT_PRINT(test)
#define BT_ASSERT_PRINT_MSG(fmt, ...)
#endif /* CONFIG_BT_ASSERT_VERBOSE */

#if defined(CONFIG_BT_ASSERT_PANIC)
#define BT_ASSERT_DIE k_panic
#else
#define BT_ASSERT_DIE k_oops
#endif /* CONFIG_BT_ASSERT_PANIC */

#if defined(CONFIG_BT_ASSERT)
#define BT_ASSERT(cond)                          \
	do {                                     \
		if (!(cond)) {                   \
			BT_ASSERT_PRINT(cond);   \
			BT_ASSERT_DIE();         \
		}                                \
	} while (0)

#define BT_ASSERT_MSG(cond, fmt, ...)                              \
	do {                                                       \
		if (!(cond)) {                                     \
			BT_ASSERT_PRINT(cond);                     \
			BT_ASSERT_PRINT_MSG(fmt, ##__VA_ARGS__);   \
			BT_ASSERT_DIE();                           \
		}                                                  \
	} while (0)
#else
#define BT_ASSERT(cond) __ASSERT_NO_MSG(cond)
#define BT_ASSERT_MSG(cond, msg, ...) __ASSERT(cond, msg, ##__VA_ARGS__)
#endif/* CONFIG_BT_ASSERT*/

#define BT_HEXDUMP_DBG(_data, _length, _str) \
		LOG_HEXDUMP_DBG((const uint8_t *)_data, _length, _str)

/* NOTE: These helper functions always encodes into the same buffer storage.
 * It is the responsibility of the user of this function to copy the information
 * in this string if needed.
 *
 * NOTE: These functions are not thread-safe!
 */
const char *bt_hex_real(const void *buf, size_t len);
const char *bt_addr_str_real(const bt_addr_t *addr);
const char *bt_addr_le_str_real(const bt_addr_le_t *addr);
const char *bt_uuid_str_real(const struct bt_uuid *uuid);

#define bt_hex(buf, len) bt_hex_real(buf, len)
#define bt_addr_str(addr) bt_addr_str_real(addr)
#define bt_addr_le_str(addr) bt_addr_le_str_real(addr)
#define bt_uuid_str(uuid) bt_uuid_str_real(uuid)

#ifdef __cplusplus
}
#endif

#endif /* __BT_LOG_H */

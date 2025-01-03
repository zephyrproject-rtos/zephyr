/** @file
 *  @brief Bluetooth shell functions
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_H
#define __BT_H

#include <zephyr/bluetooth/bluetooth.h>
#include <sys/types.h>

extern struct bt_conn *default_conn;

bool passes_scan_filter(const struct bt_le_scan_recv_info *info, const struct net_buf_simple *buf);

#if defined(CONFIG_BT_ISO)
extern struct bt_iso_chan iso_chan;
#endif /* CONFIG_BT_ISO */

#if defined(CONFIG_BT_EXT_ADV)
extern uint8_t selected_adv;
extern struct bt_le_ext_adv *adv_sets[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
#if defined(CONFIG_BT_PER_ADV_SYNC)
extern size_t selected_per_adv_sync;
extern struct bt_le_per_adv_sync *per_adv_syncs[CONFIG_BT_PER_ADV_SYNC_MAX];
#endif /* CONFIG_BT_PER_ADV_SYNC */
#endif /* CONFIG_BT_EXT_ADV */

void conn_addr_str(struct bt_conn *conn, char *addr, size_t len);

/**
 * @brief Compares two strings without case sensitivity
 *
 * @param substr The substring
 * @param str The string to find the substring in
 *
 * @return true if @substr is a substring of @p, else false
 */
bool is_substring(const char *substr, const char *str);

#endif /* __BT_H */

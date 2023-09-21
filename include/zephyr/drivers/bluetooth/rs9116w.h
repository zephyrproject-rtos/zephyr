/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>

/**
 * @brief Get the RSSI of a connection
 *
 * @param conn Connection to read RSSI for
 * @return int Absolute value of RSSI on success, negative errno on fail.
 */
int bt_conn_le_get_rssi(const struct bt_conn *conn);

/**
 * @brief Get the hadware MAC address from the bluetooth hardware
 *
 * @param mac Output buffer for the MAC address
 * @return int 0 on success, negative errno on failure.
 */
int bt_get_mac(char *mac);

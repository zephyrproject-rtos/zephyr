/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>

/* Globals shared between the central/peripheral mains and gatt_write_common.c */
extern struct bt_conn *conn_connected;
extern uint32_t last_write_rate;
extern uint32_t *write_countdown;

/* Restart helpers installed by each sample's main() */
extern void (*start_scan_func)(void);
extern int (*restart_adv_func)(void);

/**
 * @brief Send one GATT Write Without Response to the connected peer.
 *
 * @param conn Connection to transmit on.
 *
 * @return 0 on success, or a negative errno on failure.
 */
int write_cmd(struct bt_conn *conn);

/**
 * @brief Get the current connection interval, for backing off a retry.
 *
 * @param conn Connection to query.
 *
 * @return The connection interval, or 10 ms if it cannot be read.
 */
k_timeout_t conn_interval_get(struct bt_conn *conn);

/**
 * @brief Advance the notification flow and check if this side may transmit.
 *
 * Retries a failed local discovery or subscription. Call on every iteration
 * of the sample's thread while holding a reference to @p conn.
 *
 * Only defined when @kconfig{CONFIG_USE_NOTIFY} is set.
 *
 * @param conn Current connection.
 *
 * @return true once the peer has subscribed on this connection and, with
 *         @kconfig{CONFIG_USE_FULL_DUPLEX}, the local subscription is in
 *         place; false otherwise, always for the simplex central.
 */
bool notify_poll(struct bt_conn *conn);

/**
 * @brief Send one GATT notification to the subscribed peer.
 *
 * Call only after notify_poll() returned true. Only defined when
 * @kconfig{CONFIG_USE_NOTIFY} is set.
 *
 * @param conn Connection to notify on.
 *
 * @return 0 when a notification was queued, -ENOMEM if no buffer was free,
 *         or another negative errno on failure.
 */
int notify_data(struct bt_conn *conn);

/**
 * @brief Start a simplex notification RX measurement window.
 *
 * @param count Notifications to receive from now on to complete the window,
 *              or 0 to receive forever.
 */
void notify_rx_window_start(uint32_t count);

/**
 * @brief Check whether the current simplex RX measurement window is complete.
 *
 * @return true once the notifications requested by the last
 *         notify_rx_window_start() have been received, false otherwise.
 */
bool notify_rx_window_done(void);

#endif /* ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_ */

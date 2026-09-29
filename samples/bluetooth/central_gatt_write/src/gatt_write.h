/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_

#include <stdbool.h>
#include <stdint.h>

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
 * @brief Send one GATT notification to the subscribed peer.
 *
 * Only defined when @kconfig{CONFIG_USE_NOTIFY} is set.
 *
 * @param conn Connection to notify on.
 *
 * @return 0 when a notification was queued, -EAGAIN if the peer has not yet
 *         subscribed or, with @kconfig{CONFIG_USE_FULL_DUPLEX}, the local
 *         subscription is not yet in place, or another negative errno on
 *         failure.
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

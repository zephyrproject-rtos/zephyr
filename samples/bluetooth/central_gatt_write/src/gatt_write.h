/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_
#define ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>

/* Globals shared between the central/peripheral mains and gatt_write_common.c */
extern struct bt_conn *conn_connected;
extern uint32_t last_write_rate;
extern uint32_t *write_countdown;

/* Restart helpers installed by each sample's main() */
extern void (*start_scan_func)(void);
extern void (*restart_adv_func)(void);

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
 *         subscribed, or another negative errno on failure.
 */
int notify_data(struct bt_conn *conn);

/**
 * @brief Notifications received since the current measurement window started.
 *
 * Only defined when @kconfig{CONFIG_USE_NOTIFY} is set and
 * @kconfig{CONFIG_USE_FULL_DUPLEX} is not.
 *
 * @return Number of notifications received in the current window.
 */
uint32_t notify_rx_delta(void);

#endif /* ZEPHYR_SAMPLES_BLUETOOTH_CENTRAL_GATT_WRITE_GATT_WRITE_H_ */

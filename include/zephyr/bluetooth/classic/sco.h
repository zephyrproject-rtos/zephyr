/** @file
 *  @brief Bluetooth SCO (Synchronous Connection Oriented) API
 */

/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_

/**
 * @brief Bluetooth SCO
 * @defgroup bt_sco Bluetooth SCO
 * @ingroup bluetooth
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set the SCO data receive callback for a SCO connection.
 *
 *  Used with SCO over HCI (@kconfig{CONFIG_BT_SCO_OVER_HCI}). The callback is
 *  run from the Bluetooth RX workqueue context (@kconfig{CONFIG_BT_RECV_WORKQ_BT})
 *  or the system workqueue (@kconfig{CONFIG_BT_RECV_WORKQ_SYS}), depending on
 *  configuration.
 *
 *  The callback is per SCO connection and is automatically cleared when the
 *  connection is disconnected.
 *
 *  @param sco_conn SCO connection object.
 *  @param recv Receive callback. Pass NULL to clear. The callback receives the
 *             SCO connection object and a buffer containing the SCO payload data
 *             (excluding HCI header). The callback must call @ref net_buf_unref
 *             on the buffer when processing is complete. To retain the buffer
 *             beyond the callback, call @ref net_buf_ref before returning.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL If @p sco_conn is NULL or not a SCO connection.
 *  @retval -ENOTCONN If @p sco_conn has no associated channel.
 */
int bt_sco_recv_cb_set(struct bt_conn *sco_conn,
		       void (*recv)(struct bt_conn *sco_conn, struct net_buf *buf));

/** @brief Allocate a buffer for outgoing SCO data.
 *
 *  The buffer is allocated with sufficient headroom for the HCI SCO header
 *  and H:4 packet type byte. Data should be added with net_buf_add() before
 *  calling bt_sco_send().
 *
 *  @param timeout Non-negative waiting period or K_NO_WAIT / K_FOREVER.
 *
 *  @return Allocated buffer or NULL on failure.
 */
struct net_buf *bt_sco_buf_alloc(k_timeout_t timeout);

/** @brief Send SCO data to the controller over HCI.
 *
 *  Sends a SCO data packet to the remote device via the HCI transport.
 *  The buffer must have been allocated with bt_sco_buf_alloc() and contain
 *  the SCO payload data (excluding HCI headers). The payload must not exceed
 *  the effective SCO TX MTU (@kconfig{CONFIG_BT_BUF_SCO_TX_SIZE}, capped by
 *  the Controller's SCO packet length from HCI Read Buffer Size when known).
 *
 *  @param conn SCO connection object.
 *  @param buf Buffer containing SCO payload data.
 *
 *  @retval 0 Success.
 *  @retval -EINVAL Invalid parameters.
 *  @retval -ENOTCONN SCO connection is not connected.
 *  @retval -EMSGSIZE Buffer too large.
 */
int bt_sco_send(struct bt_conn *conn, struct net_buf *buf);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_SCO_H_ */

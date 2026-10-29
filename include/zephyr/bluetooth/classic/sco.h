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
#include <zephyr/sys/clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set the SCO data receive callback for a SCO connection.
 *
 *  Used with SCO over HCI (@kconfig{CONFIG_BT_SCO_OVER_HCI}). The callback runs
 *  in the Bluetooth RX workqueue thread and must not block. Codec processing
 *  and other potentially long-running work must be handed to another execution
 *  context, such as an application thread, another core, or another process.
 *
 *  The callback is per SCO connection and is automatically cleared when the
 *  connection is disconnected.
 *
 *  @param sco_conn SCO connection object.
 *  @param recv Receive callback. Pass NULL to clear. The callback receives the
 *             SCO connection object and ownership of a buffer containing the
 *             SCO payload data (excluding HCI header). It may queue the buffer
 *             to another thread, or copy the data across an application-owned
 *             IPC boundary, but must eventually call @ref net_buf_unref. Use
 *             @ref net_buf_ref if multiple contexts retain the buffer. Size
 *             @kconfig{CONFIG_BT_BUF_SCO_RX_COUNT} for the consumer's maximum
 *             jitter when buffers are retained asynchronously.
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
 *  This function must be called from a cooperative thread. The application is
 *  responsible for pacing calls from its audio clock.
 *
 *  @note Buffer ownership is transferred to the stack on success. On error,
 *  the caller retains ownership of the buffer.
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

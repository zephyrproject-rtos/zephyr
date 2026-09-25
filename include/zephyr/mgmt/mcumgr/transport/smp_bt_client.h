/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth GATT client transport for the MCUmgr SMP client.
 * @ingroup mcumgr_transport_bt_client
 */

#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_BT_CLIENT_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_BT_CLIENT_H_

#include <stdbool.h>

#include <zephyr/kernel.h>

struct bt_conn;

/**
 * @brief Bluetooth client transport for the MCUmgr SMP client.
 *
 * Lets an SMP client manage a Bluetooth peer that runs the SMP service, with the local
 * device as the GATT client. The transport registers itself at boot under
 * #SMP_BLUETOOTH_CLIENT_TRANSPORT and carries traffic for one target at a time.
 *
 * Notifications are handed to the SMP core like the input of any other transport, so
 * requests the peer sends are served by the management groups the image enables.
 *
 * @defgroup mcumgr_transport_bt_client Bluetooth client transport
 * @ingroup mcumgr_transport
 * @since 4.5
 * @version 0.1.0
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Attach the transport to a connected peer.
 *
 * Discovers the SMP service on the peer and subscribes to its notifications. Blocks until
 * done, so it must not be called from the Bluetooth RX thread, the system work queue or
 * the MCUmgr work queue.
 *
 * On success the transport holds a reference on @p conn until smp_bt_client_detach() is
 * called or the peer disconnects.
 *
 * @param conn Connection to the peer.
 * @param timeout How long to wait for the peer to answer discovery and the subscription.
 *
 * @retval 0 The transport is attached.
 * @retval -EINVAL @p conn is NULL.
 * @retval -EBUSY A target is attached, an attach or detach is in progress, or the host
 *         still holds the subscription from a previous target.
 * @retval -ECANCELED smp_bt_client_detach() was called while attaching.
 * @retval -ENOTSUP The peer has no usable SMP service.
 * @retval -ENOTCONN The peer disconnected while attaching.
 * @retval -ETIMEDOUT The peer did not answer within @p timeout.
 * @retval -EIO The peer rejected the subscription, for example because it requires an
 *         encrypted link.
 * @retval -errno Other negative errno code reported by the Bluetooth host.
 */
int smp_bt_client_attach(struct bt_conn *conn, k_timeout_t timeout);

/**
 * @brief Detach the transport from its target.
 *
 * Cancels an attach in progress and waits for it to return, unsubscribes if the link is
 * still up, drops partially received responses and releases the connection reference.
 * Safe to call when nothing is attached. Must not be called from the MCUmgr work queue.
 */
void smp_bt_client_detach(void);

/**
 * @brief Report whether a target is attached.
 *
 * @retval true A target is attached.
 * @retval false No target is attached.
 */
bool smp_bt_client_is_attached(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_BT_CLIENT_H_ */

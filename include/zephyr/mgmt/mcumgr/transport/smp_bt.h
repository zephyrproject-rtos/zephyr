/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 * @ingroup mcumgr_transport_bt
 */

#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_BT_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_BT_H_

/**
 * @brief This allows to use the MCUmgr SMP protocol over Bluetooth.
 * @defgroup mcumgr_transport_bt Bluetooth transport
 * @ingroup mcumgr_transport
 * @{
 */

#include <stdbool.h>

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/clock.h>
#include <zephyr/types.h>
struct bt_conn;

#ifdef __cplusplus
extern "C" {
#endif

/** SMP service UUID value. */
#define SMP_BT_SVC_UUID_VAL \
	BT_UUID_128_ENCODE(0x8d53dc1d, 0x1db7, 0x4cd3, 0x868b, 0x8a527460aa84)

/** SMP service UUID. */
#define SMP_BT_SVC_UUID \
	BT_UUID_DECLARE_128(SMP_BT_SVC_UUID_VAL)

/** SMP characteristic UUID value. */
#define SMP_BT_CHR_UUID_VAL \
	BT_UUID_128_ENCODE(0xda2e7828, 0xfbce, 0x4e01, 0xae9e, 0x261174997c48)

/** SMP characteristic UUID
 *  Used for both requests and responses.
 */
#define SMP_BT_CHR_UUID \
	BT_UUID_DECLARE_128(SMP_BT_CHR_UUID_VAL)

/**
 * @brief	Registers the SMP Bluetooth service. Should only be called if the Bluetooth
 *		transport has been unregistered by calling smp_bt_unregister().
 *
 * @return	0 on success; negative error code on failure.
 */
int smp_bt_register(void);

/**
 * @brief	Unregisters the SMP Bluetooth service.
 *
 * @return	0 on success; negative error code on failure.
 */
int smp_bt_unregister(void);

/**
 * @brief	Transmits an SMP command/response over the specified Bluetooth connection as a
 *		notification.
 *
 * @param conn	Connection object.
 * @param data	Pointer to SMP message.
 * @param len	data length.
 *
 * @return	0 in case of success or negative value in case of error.
 */
int smp_bt_notify(struct bt_conn *conn, const void *data, uint16_t len);

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) || defined(__DOXYGEN__)
/**
 * @brief Bluetooth client transport for the MCUmgr SMP client.
 *
 * Lets an SMP client manage a Bluetooth peer that runs the SMP service, with the local
 * device as the GATT client. The transport registers itself at boot as
 * #SMP_BLUETOOTH_TRANSPORT and carries traffic for one target at a time. SMP client
 * commands still pending when the target is released are not cancelled; their retries
 * are never sent to a later target, so they time out.
 *
 * When built with MCUMGR_GRP_TRANSPORT instead of SMP_CLIENT, the same transport is the
 * outgoing side of transport management bridges, and these functions are not available.
 *
 * @defgroup mcumgr_transport_bt_client Bluetooth client transport
 * @ingroup mcumgr_transport_bt
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Attach the transport to a connected peer.
 *
 * Discovers the SMP service on the peer and subscribes to its notifications. Blocks until
 * done or until @p timeout expires, so it must not be called from the Bluetooth RX thread,
 * the system work queue or the MCUmgr work queue.
 *
 * On success the transport holds a reference on @p conn until smp_client_bt_detach() is
 * called or the peer disconnects.
 *
 * The application must not subscribe to the SMP characteristic of the peer itself: the
 * host makes no CCC write for a second subscription to a characteristic, so the attach
 * would wait for a response that never comes and fail with -ETIMEDOUT.
 *
 * @param conn Connection to the peer.
 * @param timeout How long to wait for the peer to answer discovery and the subscription.
 *
 * @retval 0 The transport is attached.
 * @retval -EINVAL @p conn is NULL.
 * @retval -EBUSY A target is attached, another attach or a detach is in progress, the
 *         host still holds the subscription of a previous target, or the GATT discovery of
 *         an earlier attach is still running.
 * @retval -ECANCELED smp_client_bt_detach() was called during the attach, or was in
 *         progress when it started.
 * @retval -ENOTSUP The peer has no usable SMP service.
 * @retval -ENOTCONN The peer disconnected while attaching.
 * @retval -ETIMEDOUT The peer did not answer within @p timeout.
 * @retval -EIO The peer rejected the subscription, for example because it requires an
 *         encrypted link.
 * @retval -ENOSYS The transport is built for transport management bridges.
 * @retval -errno Other negative errno code reported by the Bluetooth host.
 */
int smp_client_bt_attach(struct bt_conn *conn, k_timeout_t timeout);

/**
 * @brief Detach the transport from its target.
 *
 * Cancels an attach in progress and waits for it to return. Then unsubscribes if the link
 * is still up, drops partly received responses and releases the connection reference; the
 * link stays up. Until the host reports the response to the CCC write of the unsubscribe,
 * or the link ends, smp_client_bt_attach() fails with -EBUSY. Safe to call when nothing is
 * attached. Must not be called from the MCUmgr work queue.
 */
void smp_client_bt_detach(void);

/**
 * @brief Report whether a target is attached for the SMP client.
 *
 * @retval true A target is attached.
 * @retval false No target is attached.
 */
bool smp_client_bt_is_attached(void);

/**
 * @}
 */
#endif /* CONFIG_MCUMGR_TRANSPORT_BT_CLIENT || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SMP_BT_H_ */

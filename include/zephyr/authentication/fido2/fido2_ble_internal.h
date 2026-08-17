/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_AUTHENTICATION_FIDO2_FIDO2_BLE_INTERNAL_H_
#define ZEPHYR_SUBSYS_AUTHENTICATION_FIDO2_FIDO2_BLE_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/** Maximum complete FIDO BLE message size accepted by the transport. */
#define FIDO2_BLE_MAX_MESSAGE_SIZE (CONFIG_FIDO2_CBOR_MAX_SIZE + 1U)

/**
 * @brief FIDO BLE framing command identifiers.
 */
enum fido2_ble_command {
	/** Echo request or response. */
	FIDO2_BLE_CMD_PING = 0x81U,
	/** Authenticator keepalive notification. */
	FIDO2_BLE_CMD_KEEPALIVE = 0x82U,
	/** CTAP message request or response. */
	FIDO2_BLE_CMD_MSG = 0x83U,
	/** Request to cancel the currently active operation. */
	FIDO2_BLE_CMD_CANCEL = 0xBEU,
	/** Transport-level error response. */
	FIDO2_BLE_CMD_ERROR = 0xBFU,
};

/**
 * @brief Callbacks from the FIDO BLE GATT layer.
 */
struct fido2_ble_gatt_callbacks {
	/**
	 * @brief Callback for a fragment written to the FIDO BLE Control Point.
	 * @param conn      Connection that received the fragment
	 * @param data      Fragment data
	 * @param len       Length of @p data in bytes
	 * @retval 0        Fragment accepted for processing
	 * @retval non-zero Fragment could not be accepted by the framing layer
	 */
	int (*fragment_received)(struct bt_conn *conn, const void *data, uint16_t len);

	/**
	 * @brief Callback for a disconnected FIDO BLE connection.
	 * @param conn Connection that was disconnected
	 * @param cid  Transport connection identifier assigned to @p conn
	 */
	void (*disconnected)(struct bt_conn *conn, uint32_t cid);

	/**
	 * @brief Callback for changes to FIDO BLE Status notifications.
	 * @param cid     Transport connection identifier
	 * @param enabled true when notifications are enabled, false otherwise
	 */
	void (*notifications_changed)(uint32_t cid, bool enabled);
};

/**
 * @brief Callbacks from the FIDO BLE framing layer.
 */
struct fido2_ble_framing_callbacks {
	/**
	 * @brief Callback for a completely reassembled FIDO BLE MSG command.
	 * @param conn Connection that received the message
	 * @param data Complete message payload
	 * @param len  Length of @p data in bytes
	 * @retval 0        Message accepted by the transport
	 * @retval non-zero Message could not be accepted; value is translated to a BLE error
	 */
	int (*message_received)(struct bt_conn *conn, const uint8_t *data, size_t len);

	/**
	 * @brief Callback for a received FIDO BLE CANCEL command.
	 */
	void (*cancel_received)(void);

	/**
	 * @brief Callback for checking whether a CTAP transaction is active.
	 * @retval true  A transaction is currently active
	 * @retval false No transaction is currently active
	 */
	bool (*transaction_active)(void);
};

/**
 * @brief Initialize the FIDO BLE GATT layer.
 * @param callbacks Callbacks used to forward GATT events to the framing layer
 * @retval 0         GATT layer initialized successfully
 * @retval -EINVAL   @p callbacks or one of its required callbacks is NULL
 * @retval -EALREADY The GATT layer is already initialized
 */
int fido2_ble_gatt_init(const struct fido2_ble_gatt_callbacks *callbacks);

/**
 * @brief Shut down the FIDO BLE GATT layer.
 */
void fido2_ble_gatt_shutdown(void);

/**
 * @brief Acquire a reference to the ready connection identified by a CID.
 * @param cid Transport connection identifier
 * @return Referenced connection on success, or NULL if @p cid is not the current ready connection
 *
 * The caller must release a non-NULL return value with bt_conn_unref().
 */
struct bt_conn *fido2_ble_gatt_conn_ref(uint32_t cid);

/**
 * @brief Get the CID assigned to a ready FIDO BLE connection.
 * @param conn Connection to query
 * @param cid  Destination for the transport connection identifier
 * @retval true  @p conn is the current ready FIDO BLE connection and @p cid was written
 * @retval false @p conn is not ready/current or @p cid is NULL
 */
bool fido2_ble_gatt_conn_get_cid(struct bt_conn *conn, uint32_t *cid);

/**
 * @brief Check whether a CID identifies the current ready FIDO BLE connection.
 * @param cid Transport connection identifier to check
 * @retval true  @p cid identifies the current ready connection
 * @retval false @p cid does not identify the current ready connection
 */
bool fido2_ble_gatt_cid_is_current(uint32_t cid);

/**
 * @brief Check whether a connection is the current security-ready FIDO BLE connection.
 * @param conn Connection to check
 * @retval true  @p conn is ready for FIDO BLE protocol traffic
 * @retval false @p conn is not current or has not reached the required security level
 */
bool fido2_ble_gatt_conn_is_ready(struct bt_conn *conn);

/**
 * @brief Send a notification on the FIDO BLE Status characteristic.
 * @param conn      Connection on which to send the notification
 * @param data      Notification payload
 * @param len       Length of @p data in bytes
 * @param func      Optional completion callback passed to the Bluetooth GATT layer
 * @param user_data Opaque context passed to @p func
 * @retval 0         Notification queued successfully
 * @retval -ENOTCONN @p conn is not the current ready FIDO BLE connection
 * @retval -EACCES   Status notifications are not enabled by the client
 * @retval non-zero  Error returned by bt_gatt_notify_cb()
 */
int fido2_ble_gatt_notify(struct bt_conn *conn, const void *data, uint16_t len,
			  bt_gatt_complete_func_t func, void *user_data);

/**
 * @brief Initialize the FIDO BLE framing layer.
 * @param callbacks Callbacks used to forward complete messages and control events
 * @retval 0         Framing layer initialized successfully
 * @retval -EINVAL   @p callbacks or one of its required callbacks is NULL
 * @retval -EALREADY The framing work queue has already been started
 */
int fido2_ble_framing_init(const struct fido2_ble_framing_callbacks *callbacks);

/**
 * @brief Shut down the FIDO BLE framing layer and release queued state.
 */
void fido2_ble_framing_shutdown(void);

/**
 * @brief Discard framing state associated with a closed connection.
 * @param conn Connection that was closed
 */
void fido2_ble_framing_connection_closed(struct bt_conn *conn);

/**
 * @brief Submit a received FIDO BLE fragment for asynchronous reassembly.
 * @param conn Connection that received the fragment
 * @param data Fragment data
 * @param len  Length of @p data in bytes
 * @retval 0         Fragment queued successfully
 * @retval -EINVAL   @p conn or @p data is NULL
 * @retval -EMSGSIZE @p len is zero or exceeds the configured Control Point length
 * @retval -ESHUTDOWN The framing layer is not initialized or is shutting down
 * @retval -ENOTCONN @p conn is not the current ready FIDO BLE connection
 * @retval -ENOMEM   The receive queue is full
 * @retval non-zero  Error returned while scheduling the receive worker
 */
int fido2_ble_framing_submit_fragment(struct bt_conn *conn, const void *data, uint16_t len);

/**
 * @brief Queue a complete FIDO BLE command for transmission.
 * @param conn    Connection on which to send the command
 * @param command FIDO BLE command identifier
 * @param data    Command payload, or NULL when @p len is zero
 * @param len     Length of @p data in bytes
 * @retval 0         Command queued successfully
 * @retval -EINVAL   @p data is NULL while @p len is non-zero
 * @retval -EMSGSIZE @p len exceeds the maximum FIDO BLE message size
 * @retval -ESHUTDOWN The framing layer is not initialized or is shutting down
 * @retval -ENOTCONN @p conn is NULL or is not the current ready FIDO BLE connection
 * @retval -ENOMEM   No transmit frame is available
 */
int fido2_ble_framing_send(struct bt_conn *conn, enum fido2_ble_command command,
			   const uint8_t *data, size_t len);

#endif /* ZEPHYR_SUBSYS_AUTHENTICATION_FIDO2_FIDO2_BLE_INTERNAL_H_ */

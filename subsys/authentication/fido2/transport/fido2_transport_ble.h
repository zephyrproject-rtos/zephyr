/*
 * Copyright (c) 2026 Jan Philipp Schmale <jan-philipp.schmale@teratron.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FIDO2 Bluetooth Low Energy transport definitions.
 * @ingroup fido2
 */

#ifndef FIDO2_TRANSPORT_FIDO2_TRANSPORT_BLE_H_
#define FIDO2_TRANSPORT_FIDO2_TRANSPORT_BLE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/authentication/fido2/fido2_transport.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

/** FIDO BLE service UUID. */
#define BT_UUID_FIDO2_SERVICE BT_UUID_DECLARE_16(FIDO2_BLE_SERVICE_UUID_VAL)

/** FIDO BLE Control Point characteristic UUID value. */
#define FIDO2_BLE_CONTROL_POINT_UUID_VAL                                                           \
	BT_UUID_128_ENCODE(0xF1D0FFF1, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Status characteristic UUID value. */
#define FIDO2_BLE_STATUS_UUID_VAL                                                                  \
	BT_UUID_128_ENCODE(0xF1D0FFF2, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Control Point Length characteristic UUID value. */
#define FIDO2_BLE_CONTROL_POINT_LENGTH_UUID_VAL                                                    \
	BT_UUID_128_ENCODE(0xF1D0FFF3, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Service Revision Bitfield characteristic UUID value. */
#define FIDO2_BLE_REVISION_BITFIELD_UUID_VAL                                                       \
	BT_UUID_128_ENCODE(0xF1D0FFF4, 0xDEAA, 0xECEE, 0xB42F, 0xC9BA7ED623BB)

/** FIDO BLE Control Point characteristic UUID. */
#define BT_UUID_FIDO2_BLE_CONTROL_POINT BT_UUID_DECLARE_128(FIDO2_BLE_CONTROL_POINT_UUID_VAL)

/** FIDO BLE Status characteristic UUID. */
#define BT_UUID_FIDO2_BLE_STATUS BT_UUID_DECLARE_128(FIDO2_BLE_STATUS_UUID_VAL)

/** FIDO BLE Control Point Length characteristic UUID. */
#define BT_UUID_FIDO2_BLE_CONTROL_POINT_LENGTH                                                     \
	BT_UUID_DECLARE_128(FIDO2_BLE_CONTROL_POINT_LENGTH_UUID_VAL)

/** FIDO BLE Service Revision Bitfield characteristic UUID. */
#define BT_UUID_FIDO2_BLE_REVISION_BITFIELD                                                        \
	BT_UUID_DECLARE_128(FIDO2_BLE_REVISION_BITFIELD_UUID_VAL)

/** Supported FIDO BLE service revision bitfield. */
#define FIDO2_BLE_REVISION 0x20

/**
 * Maximum CTAP message size, see CTAP 2.3 § 11.2.9.1.2.
 *
 * CTAP2 requests consist of a one-byte command code followed by up to
 * CONFIG_FIDO2_CBOR_MAX_SIZE bytes of CBOR-encoded command parameters.
 */
#define FIDO2_BLE_MAX_MESSAGE_SIZE (CONFIG_FIDO2_CBOR_MAX_SIZE + 1)

/**
 * @brief Attribute indexes within the statically defined FIDO BLE GATT service.
 */
enum fido2_ble_gatt_attr_index {
	/** Primary service declaration. */
	FIDO2_BLE_ATTR_SERVICE,
	/** Control Point characteristic declaration. */
	FIDO2_BLE_ATTR_CONTROL_POINT_CHRC,
	/** Control Point characteristic value. */
	FIDO2_BLE_ATTR_CONTROL_POINT_VALUE,
	/** Status characteristic declaration. */
	FIDO2_BLE_ATTR_STATUS_CHRC,
	/** Status characteristic value. */
	FIDO2_BLE_ATTR_STATUS_VALUE,
	/** Status Client Characteristic Configuration descriptor. */
	FIDO2_BLE_ATTR_STATUS_CCC,
	/** Control Point Length characteristic declaration. */
	FIDO2_BLE_ATTR_CONTROL_POINT_LENGTH_CHRC,
	/** Control Point Length characteristic value. */
	FIDO2_BLE_ATTR_CONTROL_POINT_LENGTH_VALUE,
	/** Service Revision Bitfield characteristic declaration. */
	FIDO2_BLE_ATTR_REVISION_CHRC,
	/** Service Revision Bitfield characteristic value. */
	FIDO2_BLE_ATTR_REVISION_VALUE,
};

/**
 * @brief FIDO BLE framing command identifiers.
 */
enum fido2_ble_command {
	/** Echo request or response. */
	FIDO2_BLE_CMD_PING = 0x81,
	/** Authenticator keepalive notification. */
	FIDO2_BLE_CMD_KEEPALIVE = 0x82,
	/** CTAP message request or response. */
	FIDO2_BLE_CMD_MSG = 0x83,
	/** Request to cancel the currently active operation. */
	FIDO2_BLE_CMD_CANCEL = 0xBE,
	/** Transport-level error response. */
	FIDO2_BLE_CMD_ERROR = 0xBF,
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

#endif /* ZEPHYR_INCLUDE_AUTHENTICATION_FIDO2_TRANSPORT_BLE_H_ */

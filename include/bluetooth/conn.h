/** @file
 *  @brief Bluetooth connection handling
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BT_CONN_H
#define __BT_CONN_H

#if defined(CONFIG_BLUETOOTH_CENTRAL) || defined(CONFIG_BLUETOOTH_PERIPHERAL)
#include <stdbool.h>

#include <bluetooth/hci.h>

/** Opaque type representing a connection to a remote device */
struct bt_conn;

/** @brief Increment a connection's reference count.
 *
 *  Increment the reference count of a connection object.
 *
 *  @param conn Connection object.
 *
 *  @return Connection object with incremented reference count.
 */
struct bt_conn *bt_conn_ref(struct bt_conn *conn);

/** @brief Decrement a connection's reference count.
 *
 *  Decrement the reference count of a connection object.
 *
 *  @param conn Connection object.
 */
void bt_conn_unref(struct bt_conn *conn);

/** @brief Look up an existing connection by address.
 *
 *  Look up an existing connection based on the remote address.
 *
 *  @param peer Remote address.
 *
 *  @return Connection object or NULL if not found. The caller gets a
 *  new reference to the connection object which must be released with
 *  bt_conn_unref() once done using the object.
 */
struct bt_conn *bt_conn_lookup_addr_le(const bt_addr_le_t *peer);

/** @brief Get destination (peer) address of a connection.
 *
 *  @param conn Connection object.
 *
 *  @return Destination address.
 */
const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn);

/** @brief Disconnect from a remote device or cancel pending connection.
 *
 *  Disconnect an active connection with the specified reason code or cancel
 *  pending outgoing connection.
 *
 *  @param conn Connection to disconnect.
 *  @param reason Reason code for the disconnection.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason);

#if defined(CONFIG_BLUETOOTH_CENTRAL)
/** @brief Initiate an LE connection to a remote device.
 *
 *  Allows initiate new LE link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer Remote address.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer);
#endif

/** Security level. */
typedef enum {
	BT_SECURITY_LOW,    /** No encryption and no authentication. */
	BT_SECURITY_MEDIUM, /** encryption and no authentication (no MITM). */
	BT_SECURITY_HIGH,   /** encryption and authentication (MITM). */
	BT_SECURITY_FIPS,   /** Authenticated LE Secure Connections and
			     *  encryption.
			     */
} bt_security_t;

#if defined(CONFIG_BLUETOOTH_SMP)
/** @brief Set security level for a connection.
 *
 *  This function enable security (encryption) for a connection. If device is
 *  already paired with sufficiently strong key encryption will be enabled. If
 *  link is already encrypted with sufficiently strong key this function does
 *  nothing.
 *
 *  If device is not paired pairing will be initiated. If device is paired and
 *  keys are too weak but input output capabilities allow for strong enough keys
 *  pairing will be initiated.
 *
 *  This function may return error if required level of security is not possible
 *  to achieve due to local or remote device limitation (eg input output
 *  capabilities).
 *
 *  @param conn Connection object.
 *  @param sec Requested security level.
 *
 *  @return 0 on success or negative error
 */
int bt_conn_security(struct bt_conn *conn, bt_security_t sec);
#endif

/** Connection callback structure */
struct bt_conn_cb {
	void (*connected)(struct bt_conn *conn);
	void (*disconnected)(struct bt_conn *conn);
#if defined(CONFIG_BLUETOOTH_SMP)
	void (*identity_resolved)(struct bt_conn *conn,
				  const bt_addr_le_t *rpa,
				  const bt_addr_le_t *identity);
	void (*security_changed)(struct bt_conn *conn, bt_security_t level);
#endif
	struct bt_conn_cb *_next;
};

/** @brief Register connection callbacks.
 *
 *  Register callbacks to monitor the state of connections.
 *
 *  @param cb Callback struct.
 */
void bt_conn_cb_register(struct bt_conn_cb *cb);

/** @brief Get encryption key size.
 *
 *  This function gets encryption key size.
 *  If there is no security (encryption) enabled 0 will be returned.
 *
 *  @param conn Existing connection object.
 *
 *  @return Encryption key size.
 */
uint8_t bt_conn_enc_key_size(struct bt_conn *conn);

#if defined(CONFIG_BLUETOOTH_CENTRAL)
/** @brief Automatically connect to remote device if it's in range.
 *
 *  This function enables/disables automatic connection initiation.
 *  Everytime the device looses the connection with peer, this connection
 *  will be re-established if connectable advertisement from peer is received.
 *
 *  @param conn Existing connection object.
 *  @param auto_conn boolean value. If true, auto connect is enabled,
 *  if false, auto connect is disabled.
 *
 *  @return none
 */
void bt_conn_set_auto_conn(struct bt_conn *conn, bool auto_conn);

#endif /* CONFIG_BLUETOOTH_CENTRAL */
#endif /* CONFIG_BLUETOOTH_CENTRAL || CONFIG_BLUETOOTH_PERIPHERAL */
#endif /* __BT_CONN_H */

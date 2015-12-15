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

/** Connection parameters for LE connections */
struct bt_le_conn_param {
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
};

/** Helper to declare connection parameters inline
  *
  * @param int_min  Minimum Connection Interval (N * 1.25 ms)
  * @param int_max  Maximum Connection Interval (N * 1.25 ms)
  * @param lat      Connection Latency
  * @param timeout  Supervision Timeout (N * 10 ms)
  */
#define BT_LE_CONN_PARAM(int_min, int_max, lat, to) \
	(&(struct bt_le_conn_param) { \
		.interval_min = (int_min), \
		.interval_max = (int_max), \
		.latency = (lat), \
		.timeout = (to), \
	 })

/** Default LE connection parameters:
  *   Connection Interval: 30-50 ms
  *   Latency: 0
  *   Timeout: 4 s
  */
#define BT_LE_CONN_PARAM_DEFAULT BT_LE_CONN_PARAM(BT_GAP_INIT_CONN_INT_MIN, \
						  BT_GAP_INIT_CONN_INT_MAX, \
						  0, 400)

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

/** Connection Type */
enum {
	BT_CONN_TYPE_LE, /** LE Connection Type */
#if defined(CONFIG_BLUETOOTH_BREDR)
	BT_CONN_TYPE_BR, /** BR/EDR Connection Type */
#endif
};

/** LE Connection Info Structure */
struct bt_conn_le_info {
	const bt_addr_le_t *src; /** Source Address */
	const bt_addr_le_t *dst; /** Destination Address */
};

#if defined(CONFIG_BLUETOOTH_BREDR)
/** BR/EDR Connection Info Structure */
struct bt_conn_br_info {};
#endif

/** Connection Info Structure */
struct bt_conn_info {
	/** Connection Type */
	uint8_t type;
	union {
		/** LE Connection specific Info */
		struct bt_conn_le_info le;
#if defined(CONFIG_BLUETOOTH_BREDR)
		struct bt_conn_br_info br;
#endif
	};
};

/** @brief Get connection info
 *
 *  @param conn Connection object.
 *  @param info Connection info object.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info);

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
 *  @param peer  Remote address.
 *  @param param Initial connection parameters.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param);
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
#endif /* CONFIG_BLUETOOTH_SMP */

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

#endif /* CONFIG_BLUETOOTH_CENTRAL || CONFIG_BLUETOOTH_PERIPHERAL */
#endif /* __BT_CONN_H */

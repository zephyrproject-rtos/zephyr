/*! @file
 *  @brief Bluetooth connection handling
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BT_CONN_H
#define __BT_CONN_H

#include <bluetooth/hci.h>

/*! Opaque type representing a connection to a remote device */
struct bt_conn;

/*! @brief Increment a connection's reference count.
 *
 *  Increment the reference count of a connection object.
 *
 *  @param conn Connection object.
 *
 *  @return Connection object with incremented reference count.
 */
struct bt_conn *bt_conn_get(struct bt_conn *conn);

/*! @brief Decrement a connection's reference count.
 *
 *  Decrement the reference count of a connection object.
 *
 *  @param conn Connection object.
*/
void bt_conn_put(struct bt_conn *conn);

/*! @brief Look up an existing connection by address.
 *
 *  Look up an existing connection based on the remote address.
 *
 *  @param peer Remote address.
 *
 *  @return Connection object or NULL if not found. The caller gets a
 *  new reference to the connection object which must be released with
 *  bt_conn_put() once done using the object.
 */
struct bt_conn *bt_conn_lookup_addr_le(const bt_addr_le_t *peer);

/*! @brief Get destination (peer) address of a connection.
 *
 *  @param conn Connection object.
 *
 *  @return Destination address.
 */
const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn);

/*! Connection callback structure */
struct bt_conn_cb {
	void (*connected)(struct bt_conn *conn);
	void (*disconnected)(struct bt_conn *conn);

	struct bt_conn_cb *_next;
};

/*! @brief Register connection callbacks.
 *
 *  Register callbacks to monitor the state of connections.
 *
 *  @param cb Callback struct.
 */
void bt_conn_cb_register(struct bt_conn_cb *cb);


typedef enum {
	BT_CONN_SEC_NONE,
	BT_CONN_SEC_LOW,
	BT_CONN_SEC_MEDIUM,
	BT_CONN_SEC_HIGH,
	BT_CONN_SEC_FIPS,
} bt_conn_security_t;

/*! @brief Set security level for a connection.
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
int bt_conn_security(struct bt_conn *conn, bt_conn_security_t sec);

#endif /* __BT_CONN_H */

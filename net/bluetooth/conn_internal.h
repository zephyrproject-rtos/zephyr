/** @file
 *  @brief Internal APIs for Bluetooth connection handling.
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

typedef enum {
	BT_CONN_DISCONNECTED,
	BT_CONN_CONNECT_SCAN,
	BT_CONN_CONNECT,
	BT_CONN_CONNECTED,
	BT_CONN_DISCONNECT,
} bt_conn_state_t;

/* L2CAP signaling channel specific context */
struct bt_conn_l2cap {
	uint8_t			ident;
	void			*channels;
};

/* bt_conn flags: the flags defined here represent connection parameters */
enum {
	BT_CONN_AUTO_CONNECT,
};

struct bt_conn {
	uint16_t		handle;
	uint8_t			role;
	atomic_t		flags[1];

	bt_addr_le_t		dst;

	bt_addr_le_t		init_addr;
	bt_addr_le_t		resp_addr;

	uint8_t			pending_pkts;

#if defined(CONFIG_BLUETOOTH_SMP)
	uint8_t			encrypt;
	bt_security_t		sec_level;
	bt_security_t		required_sec_level;
#endif /* CONFIG_BLUETOOTH_SMP */

	uint16_t		rx_len;
	struct bt_buf		*rx;

	/* Queue for outgoing ACL data */
	struct nano_fifo	tx_queue;

	struct bt_keys		*keys;

	/* Fixed channel contexts */
	struct bt_conn_l2cap	l2cap;
	void			*att;
	void			*smp;

	uint8_t			le_conn_interval;

	atomic_t		ref;

	bt_conn_state_t		state;

	/* Handle allowing to cancel timeout fiber */
	void			*timeout;

	uint8_t			le_features[8];

	/* Stack for TX fiber and timeout fiber.
	 * Since these fibers don't overlap, one stack can be used by
	 * both of them.
	 */
	BT_STACK(stack, 256);
};

/* Process incoming data for a connection */
void bt_conn_recv(struct bt_conn *conn, struct bt_buf *buf, uint8_t flags);

/* Send data over a connection */
void bt_conn_send(struct bt_conn *conn, struct bt_buf *buf);

/* Add a new connection */
struct bt_conn *bt_conn_add(const bt_addr_le_t *peer);

/* Look up an existing connection */
struct bt_conn *bt_conn_lookup_handle(uint16_t handle);

/* Look up a connection state. For BT_ADDR_LE_ANY, returns the first connection
 * with the specific state
 */
struct bt_conn *bt_conn_lookup_state(const bt_addr_le_t *peer,
				     const bt_conn_state_t state);

/* Set connection object in certain state and perform action related to state */
void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state);

int bt_conn_le_conn_update(struct bt_conn *conn, uint16_t min, uint16_t max,
			   uint16_t latency, uint16_t timeout);

#if defined(CONFIG_BLUETOOTH_SMP)
/* rand and ediv should be in BT order */
int bt_conn_le_start_encryption(struct bt_conn *conn, uint64_t rand,
				uint16_t ediv, const uint8_t *ltk, size_t len);

/* Notify higher layers that RPA was resolved */
void bt_conn_identity_resolved(struct bt_conn *conn);

/* Notify higher layers that connection security changed */
void bt_conn_security_changed(struct bt_conn *conn);
#endif /* CONFIG_BLUETOOTH_SMP */

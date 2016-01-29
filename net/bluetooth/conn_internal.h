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

typedef enum __packed {
	BT_CONN_DISCONNECTED,
	BT_CONN_CONNECT_SCAN,
	BT_CONN_CONNECT,
	BT_CONN_CONNECTED,
	BT_CONN_DISCONNECT,
} bt_conn_state_t;

/* bt_conn flags: the flags defined here represent connection parameters */
enum {
	BT_CONN_AUTO_CONNECT,
	BT_CONN_BR_LEGACY_SECURE,	/* 16 digits legacy PIN tracker */
	BT_CONN_USER,			/* user I/O when pairing */
};

struct bt_conn_le {
	bt_addr_le_t		dst;

	bt_addr_le_t		init_addr;
	bt_addr_le_t		resp_addr;

	uint16_t		interval;
	uint16_t		interval_min;
	uint16_t		interval_max;

	uint16_t		latency;
	uint16_t		timeout;

	uint8_t			features[8];
};

#if defined(CONFIG_BLUETOOTH_BREDR)
struct bt_conn_br {
	bt_addr_t		dst;
	uint8_t			remote_io_capa;
	uint8_t			remote_auth;
};
#endif

struct bt_conn {
	uint16_t		handle;
	uint8_t			type;
	uint8_t			role;
	atomic_t		flags[1];

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
	bt_security_t		sec_level;
	bt_security_t		required_sec_level;
	uint8_t			encrypt;
#endif /* CONFIG_BLUETOOTH_SMP || CONFIG_BLUETOOTH_BREDR */

	uint8_t			pending_pkts;

	uint16_t		rx_len;
	struct net_buf		*rx;

	/* Queue for outgoing ACL data */
	struct nano_fifo	tx_queue;

	struct bt_keys		*keys;

	/* L2CAP channels */
	void			*channels;

	atomic_t		ref;

	/* Connection error or reason for disconnect */
	uint8_t			err;

	bt_conn_state_t		state;

	/* Handle allowing to cancel timeout fiber */
	nano_thread_id_t timeout;

	union {
		struct bt_conn_le	le;
#if defined(CONFIG_BLUETOOTH_BREDR)
		struct bt_conn_br	br;
#endif
	};

	/* Stack for TX fiber and timeout fiber.
	 * Since these fibers don't overlap, one stack can be used by
	 * both of them.
	 */
	BT_STACK(stack, 256);
};

/* Process incoming data for a connection */
void bt_conn_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags);

/* Send data over a connection */
void bt_conn_send(struct bt_conn *conn, struct net_buf *buf);

/* Add a new LE connection */
struct bt_conn *bt_conn_add_le(const bt_addr_le_t *peer);

#if defined(CONFIG_BLUETOOTH_BREDR)
/* Add a new BR/EDR connection */
struct bt_conn *bt_conn_add_br(const bt_addr_t *peer);

/* Look up an existing connection by BT address */
struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer);

void bt_conn_pin_code_req(struct bt_conn *conn);
#endif

/* Look up an existing connection */
struct bt_conn *bt_conn_lookup_handle(uint16_t handle);

/* Look up a connection state. For BT_ADDR_LE_ANY, returns the first connection
 * with the specific state
 */
struct bt_conn *bt_conn_lookup_state_le(const bt_addr_le_t *peer,
					const bt_conn_state_t state);

/* Set connection object in certain state and perform action related to state */
void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state);

void bt_conn_set_param_le(struct bt_conn *conn,
			  const struct bt_le_conn_param *param);

int bt_conn_update_param_le(struct bt_conn *conn,
			    const struct bt_le_conn_param *param);

int bt_conn_le_conn_update(struct bt_conn *conn,
			   const struct bt_le_conn_param *param);

void notify_le_param_updated(struct bt_conn *conn);

#if defined(CONFIG_BLUETOOTH_SMP)
/* rand and ediv should be in BT order */
int bt_conn_le_start_encryption(struct bt_conn *conn, uint64_t rand,
				uint16_t ediv, const uint8_t *ltk, size_t len);

/* Notify higher layers that RPA was resolved */
void bt_conn_identity_resolved(struct bt_conn *conn);

/* Notify higher layers that connection security changed */
void bt_conn_security_changed(struct bt_conn *conn);
#endif /* CONFIG_BLUETOOTH_SMP */

/* Prepare a PDU to be sent over a connection */
struct net_buf *bt_conn_create_pdu(struct nano_fifo *fifo, size_t reserve);

/* Initialize connection management */
int bt_conn_init(void);

/* Selects based on connecton type right semaphore for ACL packets */
static inline struct nano_sem *bt_conn_get_pkts(struct bt_conn *conn)
{
#if defined(CONFIG_BLUETOOTH_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		return &bt_dev.br.pkts;
	}
#endif /* CONFIG_BLUETOOTH_BREDR */

	return &bt_dev.le.pkts;
}

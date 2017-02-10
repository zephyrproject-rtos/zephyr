/** @file
 *  @brief Internal APIs for Bluetooth connection handling.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
	BT_CONN_BR_PAIRING,		/* BR connection in pairing context */
	BT_CONN_BR_NOBOND,		/* SSP no bond pairing tracker */
	BT_CONN_BR_PAIRING_INITIATOR,	/* local host starts authentication */
	BT_CONN_CLEANUP,                /* Disconnected, pending cleanup */

	/* Total number of flags - must be at the end of the enum */
	BT_CONN_NUM_FLAGS,
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

	uint8_t			features[1][8];

	struct bt_keys		*keys;

	/* Delayed work for connection update and timeout handling */
	struct k_delayed_work	update_work;
};

#if defined(CONFIG_BLUETOOTH_BREDR)
/* For now reserve space for 2 pages of LMP remote features */
#define LMP_MAX_PAGES 2

struct bt_conn_br {
	bt_addr_t		dst;
	uint8_t			remote_io_capa;
	uint8_t			remote_auth;
	uint8_t			pairing_method;
	/* remote LMP features pages per 8 bytes each */
	uint8_t			features[LMP_MAX_PAGES][8];

	struct bt_keys_link_key	*link_key;
};
#endif

struct bt_conn {
	uint16_t		handle;
	uint8_t			type;
	uint8_t			role;

	ATOMIC_DEFINE(flags, BT_CONN_NUM_FLAGS);

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
	bt_security_t		sec_level;
	bt_security_t		required_sec_level;
	uint8_t			encrypt;
#endif /* CONFIG_BLUETOOTH_SMP || CONFIG_BLUETOOTH_BREDR */

	uint8_t			pending_pkts;

	uint16_t		rx_len;
	struct net_buf		*rx;

	/* Queue for outgoing ACL data */
	struct k_fifo		tx_queue;

	/* L2CAP channels */
	void			*channels;

	atomic_t		ref;

	/* Connection error or reason for disconnect */
	uint8_t			err;

	bt_conn_state_t		state;

	union {
		struct bt_conn_le	le;
#if defined(CONFIG_BLUETOOTH_BREDR)
		struct bt_conn_br	br;
#endif
	};
};

/* Process incoming data for a connection */
void bt_conn_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags);

/* Send data over a connection */
int bt_conn_send(struct bt_conn *conn, struct net_buf *buf);

/* Add a new LE connection */
struct bt_conn *bt_conn_add_le(const bt_addr_le_t *peer);

/* Add a new BR/EDR connection */
struct bt_conn *bt_conn_add_br(const bt_addr_t *peer);

/* Look up an existing connection by BT address */
struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer);

void bt_conn_pin_code_req(struct bt_conn *conn);
uint8_t bt_conn_get_io_capa(void);
uint8_t bt_conn_ssp_get_auth(const struct bt_conn *conn);
void bt_conn_ssp_auth(struct bt_conn *conn, uint32_t passkey);

void bt_conn_disconnect_all(void);

/* Look up an existing connection */
struct bt_conn *bt_conn_lookup_handle(uint16_t handle);

/* Compare an address with bt_conn destination address */
int bt_conn_addr_le_cmp(const struct bt_conn *conn, const bt_addr_le_t *peer);

/* Look up a connection state. For BT_ADDR_LE_ANY, returns the first connection
 * with the specific state
 */
struct bt_conn *bt_conn_lookup_state_le(const bt_addr_le_t *peer,
					const bt_conn_state_t state);

/* Set connection object in certain state and perform action related to state */
void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state);

int bt_conn_le_conn_update(struct bt_conn *conn,
			   const struct bt_le_conn_param *param);

void notify_le_param_updated(struct bt_conn *conn);

bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param);

#if defined(CONFIG_BLUETOOTH_SMP)
/* rand and ediv should be in BT order */
int bt_conn_le_start_encryption(struct bt_conn *conn, uint64_t rand,
				uint16_t ediv, const uint8_t *ltk, size_t len);

/* Notify higher layers that RPA was resolved */
void bt_conn_identity_resolved(struct bt_conn *conn);
#endif /* CONFIG_BLUETOOTH_SMP */

#if defined(CONFIG_BLUETOOTH_SMP) || defined(CONFIG_BLUETOOTH_BREDR)
/* Notify higher layers that connection security changed */
void bt_conn_security_changed(struct bt_conn *conn);
#endif /* CONFIG_BLUETOOTH_SMP || CONFIG_BLUETOOTH_BREDR */

/* Prepare a PDU to be sent over a connection */
struct net_buf *bt_conn_create_pdu(struct net_buf_pool *pool, size_t reserve);

/* Initialize connection management */
int bt_conn_init(void);

/* Selects based on connecton type right semaphore for ACL packets */
static inline struct k_sem *bt_conn_get_pkts(struct bt_conn *conn)
{
#if defined(CONFIG_BLUETOOTH_BREDR)
	if (conn->type == BT_CONN_TYPE_BR || !bt_dev.le.mtu) {
		return &bt_dev.br.pkts;
	}
#endif /* CONFIG_BLUETOOTH_BREDR */

	return &bt_dev.le.pkts;
}

/* k_poll related helpers for the TX thread */
int bt_conn_prepare_events(struct k_poll_event events[]);
void bt_conn_process_tx(struct bt_conn *conn);

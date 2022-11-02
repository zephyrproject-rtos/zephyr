/** @file
 *  @brief Internal APIs for Bluetooth connection handling.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/iso.h>

typedef enum __packed {
	BT_CONN_DISCONNECTED,
	BT_CONN_DISCONNECT_COMPLETE,
	BT_CONN_CONNECTING_SCAN,
	BT_CONN_CONNECTING_AUTO,
	BT_CONN_CONNECTING_ADV,
	BT_CONN_CONNECTING_DIR_ADV,
	BT_CONN_CONNECTING,
	BT_CONN_CONNECTED,
	BT_CONN_DISCONNECTING,
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
	BT_CONN_PERIPHERAL_PARAM_UPDATE,/* If periph param update timer fired */
	BT_CONN_PERIPHERAL_PARAM_SET,	/* If periph param were set from app */
	BT_CONN_PERIPHERAL_PARAM_L2CAP,	/* If should force L2CAP for CPUP */
	BT_CONN_FORCE_PAIR,             /* Pairing even with existing keys. */
#if defined(CONFIG_BT_GATT_CLIENT)
	BT_CONN_ATT_MTU_EXCHANGED,	/* If ATT MTU has been exchanged. */
#endif /* CONFIG_BT_GATT_CLIENT */

	BT_CONN_AUTO_FEATURE_EXCH,	/* Auto-initiated LE Feat done */
	BT_CONN_AUTO_VERSION_INFO,      /* Auto-initiated LE version done */

	BT_CONN_CTE_RX_ENABLED,          /* CTE receive and sampling is enabled */
	BT_CONN_CTE_RX_PARAMS_SET,       /* CTE parameters are set */
	BT_CONN_CTE_TX_PARAMS_SET,       /* CTE transmission parameters are set */
	BT_CONN_CTE_REQ_ENABLED,         /* CTE request procedure is enabled */
	BT_CONN_CTE_RSP_ENABLED,         /* CTE response procedure is enabled */

	/* Total number of flags - must be at the end of the enum */
	BT_CONN_NUM_FLAGS,
};

struct bt_conn_le {
	bt_addr_le_t		dst;

	bt_addr_le_t		init_addr;
	bt_addr_le_t		resp_addr;

	uint16_t			interval;
	uint16_t			interval_min;
	uint16_t			interval_max;

	uint16_t			latency;
	uint16_t			timeout;
	uint16_t			pending_latency;
	uint16_t			pending_timeout;

	uint8_t			features[8];

	struct bt_keys		*keys;

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	struct bt_conn_le_phy_info      phy;
#endif

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	struct bt_conn_le_data_len_info data_len;
#endif
};

#if defined(CONFIG_BT_BREDR)
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

struct bt_conn_sco {
	/* Reference to ACL Connection */
	struct bt_conn          *acl;
	uint16_t                pkt_type;
};
#endif

struct bt_conn_iso {
	/* Reference to ACL Connection */
	struct bt_conn          *acl;

	/* Reference to the struct bt_iso_chan */
	struct bt_iso_chan      *chan;

	union {
		/* CIG ID */
		uint8_t			cig_id;
		/* BIG handle */
		uint8_t			big_handle;
	};

	union {
		/* CIS ID within the CIG */
		uint8_t			cis_id;

		/* BIS ID within the BIG*/
		uint8_t			bis_id;
	};

	/** Stored information about the ISO stream */
	struct bt_iso_info info;
};

typedef void (*bt_conn_tx_cb_t)(struct bt_conn *conn, void *user_data, int err);

struct bt_conn_tx {
	sys_snode_t node;

	bt_conn_tx_cb_t cb;
	void *user_data;

	/* Number of pending packets without a callback after this one */
	uint32_t pending_no_cb;
};

struct acl_data {
	/* Extend the bt_buf user data */
	struct bt_buf_data buf_data;

	/* Index into the bt_conn storage array */
	uint8_t  index;

	/** ACL connection handle */
	uint16_t handle;
};

struct bt_conn {
	uint16_t			handle;
	uint8_t			type;
	uint8_t			role;

	ATOMIC_DEFINE(flags, BT_CONN_NUM_FLAGS);

	/* Which local identity address this connection uses */
	uint8_t                    id;

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	bt_security_t		sec_level;
	bt_security_t		required_sec_level;
	uint8_t			encrypt;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

#if defined(CONFIG_BT_DF_CONNECTION_CTE_RX)
	/**
	 * @brief Bitfield with allowed CTE types.
	 *
	 *  Allowed values are defined by @ref bt_df_cte_type, except BT_DF_CTE_TYPE_NONE.
	 */
	uint8_t cte_types;
#endif /* CONFIG_BT_DF_CONNECTION_CTE_RX */

	/* Connection error or reason for disconnect */
	uint8_t			err;

	bt_conn_state_t		state;
	uint16_t rx_len;
	struct net_buf		*rx;

	/* Sent but not acknowledged TX packets with a callback */
	sys_slist_t		tx_pending;
	/* Sent but not acknowledged TX packets without a callback before
	 * the next packet (if any) in tx_pending.
	 */
	uint32_t                   pending_no_cb;

	/* Completed TX for which we need to call the callback */
	sys_slist_t		tx_complete;
#if defined(CONFIG_BT_CONN_TX)
	struct k_work           tx_complete_work;
#endif /* CONFIG_BT_CONN_TX */

	/* Queue for outgoing ACL data */
	struct k_fifo		tx_queue;

	/* Active L2CAP channels */
	sys_slist_t		channels;

	/* Delayed work deferred tasks:
	 * - Peripheral delayed connection update.
	 * - Initiator connect create cancel.
	 * - Connection cleanup.
	 */
	struct k_work_delayable	deferred_work;

	union {
		struct bt_conn_le	le;
#if defined(CONFIG_BT_BREDR)
		struct bt_conn_br	br;
		struct bt_conn_sco	sco;
#endif
#if defined(CONFIG_BT_ISO)
		struct bt_conn_iso	iso;
#endif
	};

#if defined(CONFIG_BT_REMOTE_VERSION)
	struct bt_conn_rv {
		uint8_t  version;
		uint16_t manufacturer;
		uint16_t subversion;
	} rv;
#endif
	/* Must be at the end so that everything else in the structure can be
	 * memset to zero without affecting the ref.
	 */
	atomic_t		ref;
};

void bt_conn_reset_rx_state(struct bt_conn *conn);

/* Process incoming data for a connection */
void bt_conn_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags);

/* Send data over a connection
 *
 * Buffer ownership is transferred to stack in case of success.
 *
 * Calling this from RX thread is assumed to never fail so the return can be
 * ignored.
 */
int bt_conn_send_cb(struct bt_conn *conn, struct net_buf *buf,
		    bt_conn_tx_cb_t cb, void *user_data);

static inline int bt_conn_send(struct bt_conn *conn, struct net_buf *buf)
{
	return bt_conn_send_cb(conn, buf, NULL, NULL);
}

/* Check if a connection object with the peer already exists */
bool bt_conn_exists_le(uint8_t id, const bt_addr_le_t *peer);

/* Add a new LE connection */
struct bt_conn *bt_conn_add_le(uint8_t id, const bt_addr_le_t *peer);

/** Connection parameters for ISO connections */
struct bt_iso_create_param {
	uint8_t			id;
	uint8_t			num_conns;
	struct bt_conn		**conns;
	struct bt_iso_chan	**chans;
};

int bt_conn_iso_init(void);

/* Cleanup ISO references */
void bt_iso_cleanup_acl(struct bt_conn *iso_conn);

/* Add a new BR/EDR connection */
struct bt_conn *bt_conn_add_br(const bt_addr_t *peer);

/* Add a new SCO connection */
struct bt_conn *bt_conn_add_sco(const bt_addr_t *peer, int link_type);

/* Cleanup SCO references */
void bt_sco_cleanup(struct bt_conn *sco_conn);

/* Look up an existing sco connection by BT address */
struct bt_conn *bt_conn_lookup_addr_sco(const bt_addr_t *peer);

/* Look up an existing connection by BT address */
struct bt_conn *bt_conn_lookup_addr_br(const bt_addr_t *peer);

void bt_conn_disconnect_all(uint8_t id);

/* Allocate new connection object */
struct bt_conn *bt_conn_new(struct bt_conn *conns, size_t size);

/* Look up an existing connection */
struct bt_conn *bt_conn_lookup_handle(uint16_t handle);

static inline bool bt_conn_is_handle_valid(struct bt_conn *conn)
{
	switch (conn->state) {
	case BT_CONN_CONNECTED:
	case BT_CONN_DISCONNECTING:
	case BT_CONN_DISCONNECT_COMPLETE:
		return true;
	case BT_CONN_CONNECTING:
		/* ISO connection handle assigned at connect state */
		if (IS_ENABLED(CONFIG_BT_ISO) &&
		    conn->type == BT_CONN_TYPE_ISO) {
			return true;
		}
	__fallthrough;
	default:
		return false;
	}
}

/* Check if the connection is with the given peer. */
bool bt_conn_is_peer_addr_le(const struct bt_conn *conn, uint8_t id,
			     const bt_addr_le_t *peer);

/* Helpers for identifying & looking up connections based on the the index to
 * the connection list. This is useful for O(1) lookups, but can't be used
 * e.g. as the handle since that's assigned to us by the controller.
 */
#define BT_CONN_INDEX_INVALID 0xff
struct bt_conn *bt_conn_lookup_index(uint8_t index);

/* Look up a connection state. For BT_ADDR_LE_ANY, returns the first connection
 * with the specific state
 */
struct bt_conn *bt_conn_lookup_state_le(uint8_t id, const bt_addr_le_t *peer,
					const bt_conn_state_t state);

/* Set connection object in certain state and perform action related to state */
void bt_conn_set_state(struct bt_conn *conn, bt_conn_state_t state);

void bt_conn_connected(struct bt_conn *conn);

int bt_conn_le_conn_update(struct bt_conn *conn,
			   const struct bt_le_conn_param *param);

void notify_remote_info(struct bt_conn *conn);

void notify_le_param_updated(struct bt_conn *conn);

void notify_le_data_len_updated(struct bt_conn *conn);

void notify_le_phy_updated(struct bt_conn *conn);

bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param);

#if defined(CONFIG_BT_SMP)
/* rand and ediv should be in BT order */
int bt_conn_le_start_encryption(struct bt_conn *conn, uint8_t rand[8],
				uint8_t ediv[2], const uint8_t *ltk, size_t len);

/* Notify higher layers that RPA was resolved */
void bt_conn_identity_resolved(struct bt_conn *conn);
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
/* Notify higher layers that connection security changed */
void bt_conn_security_changed(struct bt_conn *conn, uint8_t hci_err,
			      enum bt_security_err err);
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

/* Prepare a PDU to be sent over a connection */
#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_conn_create_pdu_timeout_debug(struct net_buf_pool *pool,
						 size_t reserve,
						 k_timeout_t timeout,
						 const char *func, int line);
#define bt_conn_create_pdu_timeout(_pool, _reserve, _timeout) \
	bt_conn_create_pdu_timeout_debug(_pool, _reserve, _timeout, \
					 __func__, __LINE__)

#define bt_conn_create_pdu(_pool, _reserve) \
	bt_conn_create_pdu_timeout_debug(_pool, _reserve, K_FOREVER, \
					 __func__, __line__)
#else
struct net_buf *bt_conn_create_pdu_timeout(struct net_buf_pool *pool,
					   size_t reserve, k_timeout_t timeout);

#define bt_conn_create_pdu(_pool, _reserve) \
	bt_conn_create_pdu_timeout(_pool, _reserve, K_FOREVER)
#endif

/* Prepare a PDU to be sent over a connection */
#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_conn_create_frag_timeout_debug(size_t reserve,
						  k_timeout_t timeout,
						  const char *func, int line);

#define bt_conn_create_frag_timeout(_reserve, _timeout) \
	bt_conn_create_frag_timeout_debug(_reserve, _timeout, \
					  __func__, __LINE__)

#define bt_conn_create_frag(_reserve) \
	bt_conn_create_frag_timeout_debug(_reserve, K_FOREVER, \
					  __func__, __LINE__)
#else
struct net_buf *bt_conn_create_frag_timeout(size_t reserve,
					    k_timeout_t timeout);

#define bt_conn_create_frag(_reserve) \
	bt_conn_create_frag_timeout(_reserve, K_FOREVER)
#endif

/* Initialize connection management */
int bt_conn_init(void);

/* Reset states of connections and set state to BT_CONN_DISCONNECTED. */
void bt_conn_cleanup_all(void);

/* Selects based on connection type right semaphore for ACL packets */
struct k_sem *bt_conn_get_pkts(struct bt_conn *conn);

/* k_poll related helpers for the TX thread */
int bt_conn_prepare_events(struct k_poll_event events[]);
void bt_conn_process_tx(struct bt_conn *conn);

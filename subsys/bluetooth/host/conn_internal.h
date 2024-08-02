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
	BT_CONN_DISCONNECTED,         /* Disconnected, conn is completely down */
	BT_CONN_DISCONNECT_COMPLETE,  /* Received disconn comp event, transition to DISCONNECTED */

	BT_CONN_INITIATING,           /* Central connection establishment */
	/** Central scans for a device preceding establishing a connection to it.
	 *
	 * This can happen when:
	 * - The application has explicitly configured the stack to connect to the device,
	 *   but the controller resolving list is too small. The stack therefore first
	 *   scans to be able to retrieve the currently used (private) address, resolving
	 *   the address in the host if needed.
	 * - The stack uses this connection context for automatic connection establishment
	 *   without the use of filter accept list. Instead of immediately starting
	 *   the initiator, it first starts scanning. This allows the application to start
	 *   scanning while automatic connection establishment in ongoing.
	 *   It also allows the stack to use host based privacy for cases where this is needed.
	 */
	BT_CONN_SCAN_BEFORE_INITIATING,

	/** Central initiates a connection to a device in the filter accept list.
	 *
	 * For this type of connection establishment, the controller's initiator is started
	 * immediately. That is, it is assumed that the controller resolving list
	 * holds all entries that are part of the filter accept list if private addresses are used.
	 */
	BT_CONN_INITIATING_FILTER_LIST,

	BT_CONN_ADV_CONNECTABLE,       /* Peripheral connectable advertising */
	BT_CONN_ADV_DIR_CONNECTABLE,   /* Peripheral directed advertising */
	BT_CONN_CONNECTED,            /* Peripheral or Central connected */
	BT_CONN_DISCONNECTING,        /* Peripheral or Central issued disconnection command */
} bt_conn_state_t;

/* bt_conn flags: the flags defined here represent connection parameters */
enum {
	/** The connection context is used for automatic connection establishment
	 *
	 * That is, with @ref bt_conn_le_create_auto() or bt_le_set_auto_conn().
	 * This flag is set even after the connection has been established so
	 * that the connection can be reestablished once disconnected.
	 * The connection establishment may be performed with or without the filter
	 * accept list.
	 */
	BT_CONN_AUTO_CONNECT,
	BT_CONN_BR_LEGACY_SECURE,             /* 16 digits legacy PIN tracker */
	BT_CONN_USER,                         /* user I/O when pairing */
	BT_CONN_BR_PAIRING,                   /* BR connection in pairing context */
	BT_CONN_BR_NOBOND,                    /* SSP no bond pairing tracker */
	BT_CONN_BR_PAIRING_INITIATOR,         /* local host starts authentication */
	BT_CONN_CLEANUP,                      /* Disconnected, pending cleanup */
	BT_CONN_PERIPHERAL_PARAM_UPDATE,      /* If periph param update timer fired */
	BT_CONN_PERIPHERAL_PARAM_AUTO_UPDATE, /* If periph param auto update on timer fired */
	BT_CONN_PERIPHERAL_PARAM_SET,         /* If periph param were set from app */
	BT_CONN_PERIPHERAL_PARAM_L2CAP,       /* If should force L2CAP for CPUP */
	BT_CONN_FORCE_PAIR,                   /* Pairing even with existing keys. */
#if defined(CONFIG_BT_GATT_CLIENT)
	BT_CONN_ATT_MTU_EXCHANGED,            /* If ATT MTU has been exchanged. */
#endif /* CONFIG_BT_GATT_CLIENT */

	BT_CONN_AUTO_FEATURE_EXCH,            /* Auto-initiated LE Feat done */
	BT_CONN_AUTO_VERSION_INFO,            /* Auto-initiated LE version done */

	BT_CONN_CTE_RX_ENABLED,               /* CTE receive and sampling is enabled */
	BT_CONN_CTE_RX_PARAMS_SET,            /* CTE parameters are set */
	BT_CONN_CTE_TX_PARAMS_SET,            /* CTE transmission parameters are set */
	BT_CONN_CTE_REQ_ENABLED,              /* CTE request procedure is enabled */
	BT_CONN_CTE_RSP_ENABLED,              /* CTE response procedure is enabled */

	/* Total number of flags - must be at the end of the enum */
	BT_CONN_NUM_FLAGS,
};

struct bt_conn_le {
	bt_addr_le_t dst;

	bt_addr_le_t init_addr;
	bt_addr_le_t resp_addr;

	uint16_t interval;
	uint16_t interval_min;
	uint16_t interval_max;

	uint16_t latency;
	uint16_t timeout;
	uint16_t pending_latency;
	uint16_t pending_timeout;

#if defined(CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS)
	uint8_t  conn_param_retry_countdown;
#endif

	uint8_t features[8];

	struct bt_keys *keys;

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	struct bt_conn_le_phy_info phy;
#endif

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	struct bt_conn_le_data_len_info data_len;
#endif
};

#if defined(CONFIG_BT_CLASSIC)
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

	/* Reference to the struct bt_sco_chan */
	struct bt_sco_chan      *chan;

	uint16_t                pkt_type;
	uint8_t                 dev_class[3];
	uint8_t                 link_type;
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

	/** Queue from which conn will pull data */
	struct k_fifo                   txq;
};

typedef void (*bt_conn_tx_cb_t)(struct bt_conn *conn, void *user_data, int err);

struct bt_conn_tx {
	sys_snode_t node;

	bt_conn_tx_cb_t cb;
	void *user_data;
};

struct acl_data {
	/* Extend the bt_buf user data */
	struct bt_buf_data buf_data;

	/* Index into the bt_conn storage array */
	uint8_t index;

	/** Host has already sent a Host Number of Completed Packets
	 *  for this buffer.
	 */
	bool host_ncp_sent;

	/** ACL connection handle */
	uint16_t handle;
};

struct bt_conn {
	uint16_t			handle;
	enum bt_conn_type	type;
	uint8_t			role;

	ATOMIC_DEFINE(flags, BT_CONN_NUM_FLAGS);

	/* Which local identity address this connection uses */
	uint8_t                    id;

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
	bt_security_t		sec_level;
	bt_security_t		required_sec_level;
	uint8_t			encrypt;
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */

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

	/* Pending TX that are awaiting the NCP event. len(tx_pending) == in_ll */
	sys_slist_t		tx_pending;

	/* Completed TX for which we need to call the callback */
	sys_slist_t		tx_complete;
#if defined(CONFIG_BT_CONN_TX)
	struct k_work           tx_complete_work;
#endif /* CONFIG_BT_CONN_TX */

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
#if defined(CONFIG_BT_CLASSIC)
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

	/* Callback into the higher-layers (L2CAP / ISO) to return a buffer for
	 * sending `amount` of bytes to HCI.
	 *
	 * Scheduling from which channel to pull (e.g. for L2CAP) is done at the
	 * upper layer's discretion.
	 */
	struct net_buf * (*tx_data_pull)(struct bt_conn *conn,
					 size_t amount,
					 size_t *length);

	/* Get (and clears for ACL conns) callback and user-data for `buf`. */
	void (*get_and_clear_cb)(struct bt_conn *conn, struct net_buf *buf,
				 bt_conn_tx_cb_t *cb, void **ud);

	/* Return true if upper layer has data to send over HCI */
	bool (*has_data)(struct bt_conn *conn);

	/* For ACL: List of data-ready L2 channels. Used by TX processor for
	 * pulling HCI fragments. Channels are only ever removed from this list
	 * when a whole PDU (ie all its frags) have been sent.
	 */
	sys_slist_t		l2cap_data_ready;

	/* Node for putting this connection in a data-ready mode for the bt_dev.
	 * This will be used by the TX processor to then fetch HCI frags from it.
	 */
	sys_snode_t		_conn_ready;
	atomic_t		_conn_ready_lock;

	/* Holds the number of packets that have been sent to the controller but
	 * not yet ACKd (by receiving an Number of Completed Packets). This
	 * variable can be used for deriving a QoS or waterlevel scheme in order
	 * to maximize throughput/latency.
	 * It's an optimization so we don't chase `tx_pending` all the time.
	 */
	atomic_t		in_ll;

	/* Next buffer should be an ACL/ISO HCI fragment */
	bool			next_is_frag;

	/* Must be at the end so that everything else in the structure can be
	 * memset to zero without affecting the ref.
	 */
	atomic_t		ref;

	/* Another one */
	void (*tx_done)(struct bt_conn *conn, bt_conn_tx_cb_t cb, void *user_data, int err);
};

/* Holds the callback and a user-data field for the upper layer. This callback
 * shall be called when the buffer is ACK'd by the controller (by a Num Complete
 * Packets event) or if the connection dies.
 *
 * Flow control in the spec be crazy, look it up. LL is allowed to choose
 * between sending NCP events always or not at all on disconnect.
 *
 * We pack the struct to make sure it fits in the net_buf user_data field.
 */
struct closure {
	void *cb;
	void *data;
} __packed;

#if defined(CONFIG_BT_CONN_TX_USER_DATA_SIZE)
BUILD_ASSERT(sizeof(struct closure) < CONFIG_BT_CONN_TX_USER_DATA_SIZE);
#endif

static inline void make_closure(void *storage, void *cb, void *data)
{
	((struct closure *)storage)->cb = cb;
	((struct closure *)storage)->data = data;
}

static inline void *closure_cb(void *storage)
{
	return ((struct closure *)storage)->cb;
}

static inline void *closure_data(void *storage)
{
	return ((struct closure *)storage)->data;
}

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

/* Thin wrapper over `bt_conn_send_cb`
 *
 * Used to set the TS_Flag bit in `buf`'s metadata.
 *
 * Return values & buf ownership same as parent.
 */
int bt_conn_send_iso_cb(struct bt_conn *conn, struct net_buf *buf,
			bt_conn_tx_cb_t cb, bool has_ts);

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

void bt_iso_reset(void);

/* Add a new BR/EDR connection */
struct bt_conn *bt_conn_add_br(const bt_addr_t *peer);

/* Add a new SCO connection */
struct bt_conn *bt_conn_add_sco(const bt_addr_t *peer, int link_type);

/* Cleanup SCO ACL reference */
void bt_sco_cleanup_acl(struct bt_conn *sco_conn);

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
struct bt_conn *bt_conn_lookup_handle(uint16_t handle, enum bt_conn_type type);

static inline bool bt_conn_is_handle_valid(struct bt_conn *conn)
{
	switch (conn->state) {
	case BT_CONN_CONNECTED:
	case BT_CONN_DISCONNECTING:
	case BT_CONN_DISCONNECT_COMPLETE:
		return true;
	case BT_CONN_INITIATING:
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

/* Helpers for identifying & looking up connections based on the index to
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

void notify_tx_power_report(struct bt_conn *conn,
			    struct bt_conn_le_tx_power_report report);

void notify_path_loss_threshold_report(struct bt_conn *conn,
				       struct bt_conn_le_path_loss_threshold_report report);

#if defined(CONFIG_BT_SMP)
/* If role specific LTK is present */
bool bt_conn_ltk_present(const struct bt_conn *conn);

/* rand and ediv should be in BT order */
int bt_conn_le_start_encryption(struct bt_conn *conn, uint8_t rand[8],
				uint8_t ediv[2], const uint8_t *ltk, size_t len);

/* Notify higher layers that RPA was resolved */
void bt_conn_identity_resolved(struct bt_conn *conn);
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
/* Notify higher layers that connection security changed */
void bt_conn_security_changed(struct bt_conn *conn, uint8_t hci_err,
			      enum bt_security_err err);
#endif /* CONFIG_BT_SMP || CONFIG_BT_CLASSIC */

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
					 __func__, __LINE__)
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

void bt_conn_tx_processor(void);

/* To be called by upper layers when they want to send something.
 * Functions just like an IRQ.
 *
 * Note: This fn will take and hold a reference to `conn` until the IRQ for that
 * conn is serviced.
 * For the current implementation, that means:
 * - ref the conn when putting on an "conn-ready" slist
 * - unref the conn when popping the conn from the slist
 */
void bt_conn_data_ready(struct bt_conn *conn);

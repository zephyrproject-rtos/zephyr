/** @file
 *  @brief Internal APIs for Bluetooth ISO handling.
 */

/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/iso.h>

#define BT_ISO_DATA_PATH_DISABLED			0xFF

struct iso_data {
	/** BT_BUF_ISO_IN */
	uint8_t  type;

	/* Index into the bt_conn storage array */
	uint8_t  index;

	/** ISO connection handle */
	uint16_t handle;
};

enum {
	BT_BIG_INITIALIZED,

	/* Creating a BIG as a broadcaster */
	BT_BIG_PENDING,
	/* Creating a BIG as a receiver */
	BT_BIG_SYNCING,

	BT_BIG_NUM_FLAGS,
};

struct bt_iso_big {
	/** Array of ISO channels to setup as BIS (the BIG). */
	struct bt_iso_chan **bis;

	/** Total number of BISes in the BIG. */
	uint8_t  num_bis;

	/** The BIG handle */
	uint8_t handle;

	ATOMIC_DEFINE(flags, BT_BIG_NUM_FLAGS);
};

#define iso(buf) ((struct iso_data *)net_buf_user_data(buf))

#if defined(CONFIG_BT_ISO_MAX_CHAN)
extern struct bt_conn iso_conns[CONFIG_BT_ISO_MAX_CHAN];
#endif

/* Process ISO buffer */
void hci_iso(struct net_buf *buf);

/* Allocates RX buffer */
struct net_buf *bt_iso_get_rx(k_timeout_t timeout);

/* Create new ISO connecting */
struct bt_conn *iso_new(void);

/* Process CIS Estabilished event */
void hci_le_cis_estabilished(struct net_buf *buf);

/* Process CIS Request event */
void hci_le_cis_req(struct net_buf *buf);

/** Process BIG complete event */
void hci_le_big_complete(struct net_buf *buf);

/** Process BIG terminate event */
void hci_le_big_terminate(struct net_buf *buf);

/** Process BIG sync established event */
void hci_le_big_sync_established(struct net_buf *buf);

/** Process BIG sync lost event */
void hci_le_big_sync_lost(struct net_buf *buf);

/* Notify ISO channels of a new connection */
int bt_iso_accept(struct bt_conn *conn);

/* Notify ISO channels of a new connection */
void bt_iso_connected(struct bt_conn *conn);

/* Notify ISO channels of a disconnect event */
void bt_iso_disconnected(struct bt_conn *conn);

/* Allocate ISO PDU */
#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_iso_create_pdu_timeout_debug(struct net_buf_pool *pool,
						size_t reserve,
						k_timeout_t timeout,
						const char *func, int line);
#define bt_iso_create_pdu_timeout(_pool, _reserve, _timeout) \
	bt_iso_create_pdu_timeout_debug(_pool, _reserve, _timeout, \
					__func__, __LINE__)

#define bt_iso_create_pdu(_pool, _reserve) \
	bt_iso_create_pdu_timeout_debug(_pool, _reserve, K_FOREVER, \
					__func__, __line__)
#else
struct net_buf *bt_iso_create_pdu_timeout(struct net_buf_pool *pool,
					  size_t reserve, k_timeout_t timeout);

#define bt_iso_create_pdu(_pool, _reserve) \
	bt_iso_create_pdu_timeout(_pool, _reserve, K_FOREVER)
#endif

/* Allocate ISO Fragment */
#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *bt_iso_create_frag_timeout_debug(size_t reserve,
						 k_timeout_t timeout,
						 const char *func, int line);

#define bt_iso_create_frag_timeout(_reserve, _timeout) \
	bt_iso_create_frag_timeout_debug(_reserve, _timeout, \
					 __func__, __LINE__)

#define bt_iso_create_frag(_reserve) \
	bt_iso_create_frag_timeout_debug(_reserve, K_FOREVER, \
					 __func__, __LINE__)
#else
struct net_buf *bt_iso_create_frag_timeout(size_t reserve, k_timeout_t timeout);

#define bt_iso_create_frag(_reserve) \
	bt_iso_create_frag_timeout(_reserve, K_FOREVER)
#endif

#if defined(CONFIG_BT_DEBUG_ISO)
void bt_iso_chan_set_state_debug(struct bt_iso_chan *chan, uint8_t state,
				 const char *func, int line);
#define bt_iso_chan_set_state(_chan, _state) \
	bt_iso_chan_set_state_debug(_chan, _state, __func__, __LINE__)
#else
void bt_iso_chan_set_state(struct bt_iso_chan *chan, uint8_t state);
#endif /* CONFIG_BT_DEBUG_ISO */

/* Process incoming data for a connection */
void bt_iso_recv(struct bt_conn *conn, struct net_buf *buf, uint8_t flags);

struct bt_conn_iso *bt_conn_iso(struct bt_conn *conn);

void bt_iso_remove_data_path(struct bt_conn *conn);

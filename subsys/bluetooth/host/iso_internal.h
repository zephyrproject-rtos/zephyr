/** @file
 *  @brief Internal APIs for Bluetooth ISO handling.
 */

/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/iso.h>

struct iso_data {
	/* Extend the bt_buf user data */
	struct bt_buf_data buf_data;

	/* Index into the bt_conn storage array */
	uint8_t  index;

	/** ISO connection handle */
	uint16_t handle;
};

enum bt_iso_cig_state {
	BT_ISO_CIG_STATE_IDLE,
	BT_ISO_CIG_STATE_CONFIGURED,
	BT_ISO_CIG_STATE_ACTIVE,
	BT_ISO_CIG_STATE_INACTIVE
};

struct bt_iso_cig {
	/** List of ISO channels to setup as CIS (the CIG). */
	sys_slist_t cis_channels;

	/** Total number of CISes in the CIG. */
	uint8_t  num_cis;

	/** The CIG ID */
	uint8_t id;

	/** The CIG state
	 *
	 * Refer to BT Core Spec 5.3, Vol 6, Part 6, Figure 4.63
	 */
	enum bt_iso_cig_state state;
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
	/** List of ISO channels to setup as BIS (the BIG). */
	sys_slist_t bis_channels;

	/** Total number of BISes in the BIG. */
	uint8_t  num_bis;

	/** The BIG handle */
	uint8_t handle;

	ATOMIC_DEFINE(flags, BT_BIG_NUM_FLAGS);
};

#define iso(buf) ((struct iso_data *)net_buf_user_data(buf))

/* Process ISO buffer */
void hci_iso(struct net_buf *buf);

/* Allocates RX buffer */
struct net_buf *bt_iso_get_rx(k_timeout_t timeout);

/* Process CIS Established event */
void hci_le_cis_established(struct net_buf *buf);

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
void bt_iso_connected(struct bt_conn *iso);

/* Notify ISO channels of a disconnect event */
void bt_iso_disconnected(struct bt_conn *iso);

/* Notify ISO connected channels of security changed */
void bt_iso_security_changed(struct bt_conn *acl, uint8_t hci_status);

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
					__func__, __LINE__)
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

#if defined(CONFIG_BT_ISO_LOG_LEVEL_DBG)
void bt_iso_chan_set_state_debug(struct bt_iso_chan *chan,
				 enum bt_iso_state state,
				 const char *func, int line);
#define bt_iso_chan_set_state(_chan, _state) \
	bt_iso_chan_set_state_debug(_chan, _state, __func__, __LINE__)
#else
void bt_iso_chan_set_state(struct bt_iso_chan *chan, enum bt_iso_state state);
#endif /* CONFIG_BT_ISO_LOG_LEVEL_DBG */

/* Process incoming data for a connection */
void bt_iso_recv(struct bt_conn *iso, struct net_buf *buf, uint8_t flags);

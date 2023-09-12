/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RS9116W_BLE_CONN_H_
#define RS9116W_BLE_CONN_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <rsi_ble_common_config.h>
#ifndef LOG_MODULE_NAME
#define LOG_MODULE_NAME rsi_bt_conn
#endif
#include "rs9116w_ble_core.h"


/* For Identity Resolved Call in SMP.c */
void identity_resolved(struct bt_conn *conn,
				  const bt_addr_le_t *rpa,
				  const bt_addr_le_t *identity);

/* For Security Changed Call in SMP.c */
void security_changed(struct bt_conn *conn, uint8_t status);

/* bt_conn flags: the flags defined here represent connection parameters */
enum {
	BT_CONN_AUTO_CONNECT,
	BT_CONN_BR_LEGACY_SECURE,       /* 16 digits legacy PIN tracker */
	BT_CONN_USER,                   /* user I/O when pairing */
	BT_CONN_BR_PAIRING,             /* BR connection in pairing context */
	BT_CONN_BR_NOBOND,              /* SSP no bond pairing tracker */
	BT_CONN_BR_PAIRING_INITIATOR,   /* local host starts authentication */
	BT_CONN_CLEANUP,                /* Disconnected, pending cleanup */
	BT_CONN_AUTO_PHY_UPDATE,        /* Auto-update PHY */
	BT_CONN_SLAVE_PARAM_UPDATE,     /* If slave param update timer fired */
	BT_CONN_SLAVE_PARAM_SET,        /* If slave param were set from app */
	BT_CONN_SLAVE_PARAM_L2CAP,      /* If should force L2CAP for CPUP */
	BT_CONN_FORCE_PAIR,             /* Pairing even with existing keys. */

	BT_CONN_AUTO_PHY_COMPLETE,      /* Auto-initiated PHY procedure done */
	BT_CONN_AUTO_FEATURE_EXCH,      /* Auto-initiated LE Feat done */
	BT_CONN_AUTO_VERSION_INFO,      /* Auto-initiated LE version done */

	/* Auto-initiated Data Length done. Auto-initiated Data Length Update
	 * is only needed for controllers with BT_QUIRK_NO_AUTO_DLE.
	 */
	BT_CONN_AUTO_DATA_LEN_COMPLETE,

	/* Total number of flags - must be at the end of the enum */
	BT_CONN_NUM_FLAGS,
};

enum __packed bt_conn_state_t {
	BT_CONN_DISCONNECTED,
	BT_CONN_DISCONNECT_COMPLETE,
	BT_CONN_CONNECT_SCAN,
	BT_CONN_CONNECT_AUTO,
	BT_CONN_CONNECT_ADV,
	BT_CONN_CONNECT_DIR_ADV,
	BT_CONN_CONNECT,
	BT_CONN_CONNECTED,
	BT_CONN_DISCONNECT,
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

	uint8_t features[8];

	struct bt_keys *keys;

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	struct bt_conn_le_data_len_info data_len;
#endif
};

struct bt_conn {
	uint16_t handle;
	uint8_t type;
	uint8_t role;

	ATOMIC_DEFINE(flags, BT_CONN_NUM_FLAGS);

	/* Which local identity address this connection uses */
	uint8_t id;

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
	bt_security_t sec_level;
	bt_security_t required_sec_level;
	uint8_t encrypt;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

	/* Connection error or reason for disconnect */
	uint8_t err;

	enum bt_conn_state_t state;
	uint16_t rx_len;
	struct net_buf *rx;

	/* Sent but not acknowledged TX packets with a callback */
	sys_slist_t tx_pending;
	/* Sent but not acknowledged TX packets without a callback before
	 * the next packet (if any) in tx_pending.
	 */
	uint32_t pending_no_cb;

	/* Completed TX for which we need to call the callback */
	sys_slist_t tx_complete;
	struct k_work tx_complete_work;


	/* Queue for outgoing ACL data */
	struct k_fifo tx_queue;

	/* Active L2CAP/ISO channels */
	sys_slist_t channels;

	/* Delayed work deferred tasks:
	 * - Peripheral delayed connection update.
	 * - Initiator connect create cancel.
	 * - Connection cleanup.
	 */
	struct k_work_delayable deferred_work;

	union {
		struct bt_conn_le le;
	};

	/* Must be at the end so that everything else in the structure can be
	 * memset to zero without affecting the ref.
	 */
	atomic_t ref;
};

// struct bt_hci_evt_le_enh_conn_complete {
// 	uint8_t status;
// 	uint16_t handle;
// 	uint8_t role;
// 	bt_addr_le_t peer_addr;
// 	bt_addr_t local_rpa;
// 	bt_addr_t peer_rpa;
// 	uint16_t interval;
// 	uint16_t latency;
// 	uint16_t supv_timeout;
// 	uint8_t clock_accuracy;
// } __packed;

#define BT_HCI_ROLE_MASTER                      0x00
#define BT_HCI_ROLE_SLAVE                       0x01


/* Add a new LE connection */
struct bt_conn *bt_conn_add_le(uint8_t id, const bt_addr_le_t *peer);

/* Allocate new connection object */
struct bt_conn *bt_conn_new(struct bt_conn *conns, size_t size);

/* Set connection object in certain state and perform action related to state */
void bt_conn_set_state(struct bt_conn *conn, enum bt_conn_state_t state);

/* Check if a connection object with the peer already exists */
bool bt_conn_exists_le(uint8_t id, const bt_addr_le_t *peer);

/**
 * @brief Determines whether a particular connection has a given address
 *
 * @param conn Connection object
 * @param id Connection id (unused)
 * @param peer Peer address
 * @return true if address matches
 */
bool bt_conn_is_peer_addr_le(const struct bt_conn *conn, uint8_t id,
			     const bt_addr_le_t *peer);

//extern uint16_t conn_mtu[CONFIG_BT_MAX_CONN];

void bt_gatt_init(void);

void bt_gap_init(void);

void notify_connected(struct bt_conn *conn);

void notify_disconnected(struct bt_conn *conn);

void rsi_connection_cleanup_task(void);

int get_active_le_conns(void);

void bt_le_adv_resume(void);

struct bt_conn *get_acl_conn(int i);

#if defined(CONFIG_BT_SMP)
/* Notify higher layers that connection security changed */
void bt_conn_security_changed(struct bt_conn *conn, uint8_t hci_err,
			      enum bt_security_err err);
#endif /* CONFIG_BT_SMP */

#endif

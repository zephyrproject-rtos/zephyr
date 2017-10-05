/* hci_core.h - Bluetooth HCI core access */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* LL connection parameters */
#define LE_CONN_LATENCY		0x0000
#define LE_CONN_TIMEOUT		0x002a

#if defined(CONFIG_BT_BREDR)
#define LMP_FEAT_PAGES_COUNT	3
#else
#define LMP_FEAT_PAGES_COUNT	1
#endif

/* SCO  settings */
#define BT_VOICE_CVSD_16BIT     0x0060

/* k_poll event tags */
enum {
	BT_EVENT_CMD_TX,
	BT_EVENT_CONN_TX_NOTIFY,
	BT_EVENT_CONN_TX_QUEUE,
};

/* bt_dev flags: the flags defined here represent BT controller state */
enum {
	BT_DEV_ENABLE,
	BT_DEV_READY,
	BT_DEV_ID_STATIC_RANDOM,
	BT_DEV_HAS_PUB_KEY,
	BT_DEV_PUB_KEY_BUSY,

	BT_DEV_ADVERTISING,
	BT_DEV_KEEP_ADVERTISING,
	BT_DEV_SCANNING,
	BT_DEV_EXPLICIT_SCAN,
	BT_DEV_ACTIVE_SCAN,
	BT_DEV_SCAN_FILTER_DUP,

	BT_DEV_RPA_VALID,

	BT_DEV_ID_PENDING,

#if defined(CONFIG_BT_BREDR)
	BT_DEV_ISCAN,
	BT_DEV_PSCAN,
	BT_DEV_INQUIRY,
#endif /* CONFIG_BT_BREDR */

	/* Total number of flags - must be at the end of the enum */
	BT_DEV_NUM_FLAGS,
};

struct bt_dev_le {
	/* LE features */
	u8_t			features[8];
	/* LE states */
	u64_t			states;

#if defined(CONFIG_BT_CONN)
	/* Controller buffer information */
	u16_t			mtu;
	struct k_sem		pkts;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_SMP)
	/* Size of the the controller resolving list */
	u8_t                    rl_size;
	/* Number of entries in the resolving list. rl_entries > rl_size
	 * means that host-side resolving is used.
	 */
	u8_t                    rl_entries;
#endif /* CONFIG_BT_SMP */
};

#if defined(CONFIG_BT_BREDR)
struct bt_dev_br {
	/* Max controller's acceptable ACL packet length */
	u16_t         mtu;
	struct k_sem  pkts;
	u16_t         esco_pkt_type;
};
#endif

/* The theoretical max for these is 8 and 64, but there's no point
 * in allocating the full memory if we only support a small subset.
 * These values must be updated whenever the host implementation is
 * extended beyond the current values.
 */
#define BT_DEV_VS_FEAT_MAX  1
#define BT_DEV_VS_CMDS_MAX  2

/* State tracking for the local Bluetooth controller */
struct bt_dev {
	/* Local Identity Address */
	bt_addr_le_t		id_addr;

	/* Current local Random Address */
	bt_addr_le_t		random_addr;

	/* Controller version & manufacturer information */
	u8_t			hci_version;
	u8_t			lmp_version;
	u16_t			hci_revision;
	u16_t			lmp_subversion;
	u16_t			manufacturer;

	/* LMP features (pages 0, 1, 2) */
	u8_t			features[LMP_FEAT_PAGES_COUNT][8];

	/* Supported commands */
	u8_t			supported_commands[64];

#if defined(CONFIG_BT_HCI_VS_EXT)
	/* Vendor HCI support */
	u8_t                    vs_features[BT_DEV_VS_FEAT_MAX];
	u8_t                    vs_commands[BT_DEV_VS_CMDS_MAX];
#endif

	struct k_work           init;

	ATOMIC_DEFINE(flags, BT_DEV_NUM_FLAGS);

	/* LE controller specific features */
	struct bt_dev_le	le;

#if defined(CONFIG_BT_BREDR)
	/* BR/EDR controller specific features */
	struct bt_dev_br	br;
#endif

	/* Number of commands controller can accept */
	struct k_sem		ncmd_sem;

	/* Last sent HCI command */
	struct net_buf		*sent_cmd;

#if !defined(CONFIG_BT_RECV_IS_RX_THREAD)
	/* Queue for incoming HCI events & ACL data */
	struct k_fifo		rx_queue;
#endif

	/* Queue for high priority HCI events which may unlock waiters
	 * in other threads. Such events include Number of Completed
	 * Packets, as well as the Command Complete/Status events.
	 */
	struct k_fifo		rx_prio_queue;

	/* Queue for outgoing HCI commands */
	struct k_fifo		cmd_tx_queue;

	/* Registered HCI driver */
	const struct bt_hci_driver *drv;

#if defined(CONFIG_BT_PRIVACY)
	/* Local Identity Resolving Key */
	u8_t			irk[16];

	/* Work used for RPA rotation */
	struct k_delayed_work rpa_update;
#endif
};

extern struct bt_dev bt_dev;
extern const struct bt_storage *bt_storage;
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_BREDR)
extern const struct bt_conn_auth_cb *bt_auth;
#endif /* CONFIG_BT_SMP || CONFIG_BT_BREDR */

bool bt_le_conn_params_valid(const struct bt_le_conn_param *param);

struct net_buf *bt_hci_cmd_create(u16_t opcode, u8_t param_len);
int bt_hci_cmd_send(u16_t opcode, struct net_buf *buf);
int bt_hci_cmd_send_sync(u16_t opcode, struct net_buf *buf,
			 struct net_buf **rsp);

int bt_le_scan_update(bool fast_scan);

bool bt_addr_le_is_bonded(const bt_addr_le_t *addr);

int bt_send(struct net_buf *buf);

u16_t bt_hci_get_cmd_opcode(struct net_buf *buf);

/* Don't require everyone to include keys.h */
struct bt_keys;
int bt_id_add(struct bt_keys *keys);
int bt_id_del(struct bt_keys *keys);

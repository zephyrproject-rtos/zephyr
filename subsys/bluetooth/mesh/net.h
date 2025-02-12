/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adv.h"
#include "subnet.h"
#include <zephyr/bluetooth/mesh/sar_cfg.h>

#define BT_MESH_IV_UPDATE(flags)   ((flags >> 1) & 0x01)
#define BT_MESH_KEY_REFRESH(flags) (flags & 0x01)

/* How many hours in between updating IVU duration */
#define BT_MESH_IVU_MIN_HOURS      96
#define BT_MESH_IVU_HOURS          (BT_MESH_IVU_MIN_HOURS /     \
				    CONFIG_BT_MESH_IVU_DIVIDER)
#define BT_MESH_IVU_TIMEOUT        K_HOURS(BT_MESH_IVU_HOURS)

/* Minimum valid Mesh Network PDU length. The Network headers
 * themselves take up 9 bytes. After that there is a minimum of 1 byte
 * payload for both CTL=1 and CTL=0 PDUs (smallest OpCode is 1 byte). CTL=1
 * PDUs must use a 64-bit (8 byte) NetMIC, whereas CTL=0 PDUs have at least
 * a 32-bit (4 byte) NetMIC and AppMIC giving again a total of 8 bytes.
 */
#define BT_MESH_NET_MIN_PDU_LEN (BT_MESH_NET_HDR_LEN + 1 + 8)
/* Maximum valid Mesh Network PDU length. The longest packet can either be a
 * transport control message (CTL=1) of 12 bytes + 8 bytes of NetMIC, or an
 * access message (CTL=0) of 16 bytes + 4 bytes of NetMIC.
 */
#define BT_MESH_NET_MAX_PDU_LEN (BT_MESH_NET_HDR_LEN + 16 + 4)

struct bt_mesh_net_cred;
enum bt_mesh_nonce_type;

struct bt_mesh_node {
	uint16_t addr;
	uint16_t net_idx;
	struct bt_mesh_key dev_key;
	uint8_t  num_elem;
};

#if defined(CONFIG_BT_MESH_FRIEND)
#define FRIEND_SEG_RX CONFIG_BT_MESH_FRIEND_SEG_RX
#define FRIEND_SUB_LIST_SIZE CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE
#else
#define FRIEND_SEG_RX 0
#define FRIEND_SUB_LIST_SIZE 0
#endif

struct bt_mesh_friend {
	uint16_t lpn;
	uint8_t  recv_delay;
	uint8_t  fsn:1,
	      send_last:1,
	      pending_req:1,
	      pending_buf:1,
	      established:1;
	int32_t poll_to;
	uint8_t  num_elem;
	uint16_t lpn_counter;
	uint16_t counter;

	struct bt_mesh_subnet *subnet;

	struct bt_mesh_net_cred cred[2];

	uint16_t sub_list[FRIEND_SUB_LIST_SIZE];

	struct k_work_delayable timer;

	struct bt_mesh_friend_seg {
		sys_slist_t queue;

		/* The target number of segments, i.e. not necessarily
		 * the current number of segments, in the queue. This is
		 * used for Friend Queue free space calculations.
		 */
		uint8_t        seg_count;
	} seg[FRIEND_SEG_RX];

	struct net_buf *last;

	sys_slist_t queue;
	uint32_t queue_size;

	/* Friend Clear Procedure */
	struct {
		uint32_t start;                  /* Clear Procedure start */
		uint16_t frnd;                   /* Previous Friend's address */
		uint16_t repeat_sec;             /* Repeat timeout in seconds */
		struct k_work_delayable timer;   /* Repeat timer */
	} clear;
};

#if defined(CONFIG_BT_MESH_LOW_POWER)
#define LPN_GROUPS CONFIG_BT_MESH_LPN_GROUPS
#else
#define LPN_GROUPS 0
#endif

/* Low Power Node state */
struct bt_mesh_lpn {
	enum __packed {
		BT_MESH_LPN_DISABLED,     /* LPN feature is disabled */
		BT_MESH_LPN_CLEAR,        /* Clear in progress */
		BT_MESH_LPN_TIMER,        /* Waiting for auto timer expiry */
		BT_MESH_LPN_ENABLED,      /* LPN enabled, but no Friend */
		BT_MESH_LPN_REQ_WAIT,     /* Wait before scanning for offers */
		BT_MESH_LPN_WAIT_OFFER,   /* Friend Req sent */
		BT_MESH_LPN_ESTABLISHED,  /* Friendship established */
		BT_MESH_LPN_RECV_DELAY,   /* Poll sent, waiting ReceiveDelay */
		BT_MESH_LPN_WAIT_UPDATE,  /* Waiting for Update or message */
	} state;

	/* Transaction Number (used for subscription list) */
	uint8_t xact_next;
	uint8_t xact_pending;
	uint8_t sent_req;

	/* Address of our Friend when we're a LPN. Unassigned if we don't
	 * have a friend yet.
	 */
	uint16_t frnd;

	/* Value from the friend offer */
	uint8_t  recv_win;

	uint8_t  req_attempts;     /* Number of Request attempts */

	int32_t poll_timeout;

	uint8_t  groups_changed:1, /* Friend Subscription List needs updating */
	      pending_poll:1,   /* Poll to be sent after subscription */
	      disable:1,        /* Disable LPN after clearing */
	      fsn:1,            /* Friend Sequence Number */
	      established:1,    /* Friendship established */
	      clear_success:1;  /* Friend Clear Confirm received */

	/* Friend Queue Size */
	uint8_t  queue_size;

	/* FriendCounter */
	uint16_t frnd_counter;

	/* LPNCounter */
	uint16_t lpn_counter;

	/* Previous Friend of this LPN */
	uint16_t old_friend;

	/* Duration reported for last advertising packet */
	uint16_t adv_duration;

	/* Advertising start time. */
	uint32_t adv_start_time;

	/* Next LPN related action timer */
	struct k_work_delayable timer;

	/* Subscribed groups */
	uint16_t groups[LPN_GROUPS];

	struct bt_mesh_subnet *sub;

	struct bt_mesh_net_cred cred[2];

	/* Bit fields for tracking which groups the Friend knows about */
	ATOMIC_DEFINE(added, LPN_GROUPS);
	ATOMIC_DEFINE(pending, LPN_GROUPS);
	ATOMIC_DEFINE(to_remove, LPN_GROUPS);
};

/* bt_mesh_net.flags */
enum {
	BT_MESH_INIT,            /* We have been initialized */
	BT_MESH_VALID,           /* We have been provisioned */
	BT_MESH_SUSPENDED,       /* Network is temporarily suspended */
	BT_MESH_IVU_IN_PROGRESS, /* IV Update in Progress */
	BT_MESH_IVU_INITIATOR,   /* IV Update initiated by us */
	BT_MESH_IVU_TEST,        /* IV Update test mode */
	BT_MESH_IVU_PENDING,     /* Update blocked by SDU in progress */
	BT_MESH_COMP_DIRTY,      /* Composition data is dirty */
	BT_MESH_DEVKEY_CAND,     /* Has device key candidate */
	BT_MESH_METADATA_DIRTY,  /* Models metadata is dirty */

	/* Feature flags */
	BT_MESH_RELAY,
	BT_MESH_BEACON,
	BT_MESH_GATT_PROXY,
	BT_MESH_FRIEND,
	BT_MESH_PRIV_BEACON,
	BT_MESH_PRIV_GATT_PROXY,
	BT_MESH_OD_PRIV_PROXY,

	/* Don't touch - intentionally last */
	BT_MESH_FLAG_COUNT,
};

struct bt_mesh_net {
	uint32_t iv_index; /* Current IV Index */
	uint32_t seq;      /* Next outgoing sequence number (24 bits) */

	ATOMIC_DEFINE(flags, BT_MESH_FLAG_COUNT);

	/* Local network interface */
	struct k_work local_work;
	sys_slist_t local_queue;

#if defined(CONFIG_BT_MESH_FRIEND)
	/* Friend state, unique for each LPN that we're Friends for */
	struct bt_mesh_friend frnd[CONFIG_BT_MESH_FRIEND_LPN_COUNT];
#endif

#if defined(CONFIG_BT_MESH_LOW_POWER)
	struct bt_mesh_lpn lpn;  /* Low Power Node state */
#endif

	/* Number of hours in current IV Update state */
	uint8_t  ivu_duration;

	uint8_t net_xmit;
	uint8_t relay_xmit;
	uint8_t default_ttl;

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	uint8_t priv_beacon_int;
#endif

	/* Timer to track duration in current IV Update state */
	struct k_work_delayable ivu_timer;

	struct bt_mesh_key dev_key;

#if defined(CONFIG_BT_MESH_RPR_SRV)
	struct bt_mesh_key dev_key_cand;
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	uint8_t on_demand_state;
#endif
	struct bt_mesh_sar_tx sar_tx; /* Transport SAR Transmitter configuration */
	struct bt_mesh_sar_rx sar_rx; /* Transport SAR Receiver configuration */
};

/* Network interface */
enum bt_mesh_net_if {
	BT_MESH_NET_IF_ADV,
	BT_MESH_NET_IF_LOCAL,
	BT_MESH_NET_IF_PROXY,
	BT_MESH_NET_IF_PROXY_CFG,
};

/* Decoding context for Network/Transport data */
struct bt_mesh_net_rx {
	struct bt_mesh_subnet *sub;
	struct bt_mesh_msg_ctx ctx;
	uint32_t  seq;            /* Sequence Number */
	uint8_t   old_iv:1,       /* iv_index - 1 was used */
	       new_key:1,      /* Data was encrypted with updated key */
	       friend_cred:1,  /* Data was encrypted with friend cred */
	       ctl:1,          /* Network Control */
	       net_if:2,       /* Network interface */
	       local_match:1,  /* Matched a local element */
	       friend_match:1; /* Matched an LPN we're friends for */
};

/* Encoding context for Network/Transport data */
struct bt_mesh_net_tx {
	struct bt_mesh_subnet *sub;
	struct bt_mesh_msg_ctx *ctx;
	uint16_t src;
	uint8_t  xmit;
	uint8_t  friend_cred:1,
	      aszmic:1,
	      aid:6;
};

extern struct bt_mesh_net bt_mesh;

#define BT_MESH_NET_IVI_TX (bt_mesh.iv_index - \
			    atomic_test_bit(bt_mesh.flags, \
					    BT_MESH_IVU_IN_PROGRESS))
#define BT_MESH_NET_IVI_RX(rx) (bt_mesh.iv_index - (rx)->old_iv)

#define BT_MESH_NET_HDR_LEN 9

int bt_mesh_net_create(uint16_t idx, uint8_t flags, const struct bt_mesh_key *key,
		       uint32_t iv_index);

bool bt_mesh_net_iv_update(uint32_t iv_index, bool iv_update);

int bt_mesh_net_encode(struct bt_mesh_net_tx *tx, struct net_buf_simple *buf,
		       enum bt_mesh_nonce_type type);

int bt_mesh_net_send(struct bt_mesh_net_tx *tx, struct bt_mesh_adv *adv,
		     const struct bt_mesh_send_cb *cb, void *cb_data);

int bt_mesh_net_decode(struct net_buf_simple *in, enum bt_mesh_net_if net_if,
		       struct bt_mesh_net_rx *rx, struct net_buf_simple *out);

void bt_mesh_net_recv(struct net_buf_simple *data, int8_t rssi,
		      enum bt_mesh_net_if net_if);

void bt_mesh_net_loopback_clear(uint16_t net_idx);

uint32_t bt_mesh_next_seq(void);
void bt_mesh_net_seq_store(bool force);

void bt_mesh_net_init(void);
void bt_mesh_net_header_parse(struct net_buf_simple *buf,
			      struct bt_mesh_net_rx *rx);
void bt_mesh_net_pending_net_store(void);
void bt_mesh_net_pending_iv_store(void);
void bt_mesh_net_pending_seq_store(void);

void bt_mesh_net_pending_dev_key_cand_store(void);
void bt_mesh_net_dev_key_cand_store(void);

void bt_mesh_net_store(void);
void bt_mesh_net_clear(void);
void bt_mesh_net_settings_commit(void);

static inline void send_cb_finalize(const struct bt_mesh_send_cb *cb,
				    void *cb_data)
{
	if (!cb) {
		return;
	}

	if (cb->start) {
		cb->start(0, 0, cb_data);
	}

	if (cb->end) {
		cb->end(0, cb_data);
	}
}

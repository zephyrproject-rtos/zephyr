/*
 * Copyright (c) 2023 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_DFW_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_DFW_H_

#if defined(CONFIG_BT_MESH_DFW)
#define BT_MESH_DFW_DISCOVERY_COUNT		CONFIG_BT_MESH_DFW_DISCOVERY_COUNT
#define BT_MESH_DFW_FORWARDING_COUNT		CONFIG_BT_MESH_DFW_FORWARDING_COUNT
#define BT_MESH_DFW_DEPENDENT_NODES_COUNT	CONFIG_BT_MESH_DFW_DEPENDENT_NODES_COUNT
#else
#define BT_MESH_DFW_DISCOVERY_COUNT		0
#define BT_MESH_DFW_FORWARDING_COUNT		0
#define BT_MESH_DFW_DEPENDENT_NODES_COUNT	0
#endif

/** Directed Forwarding flags  */
enum bt_mesh_dfw_flags {
	BT_MESH_DFW_ENABLED,		/* Indicate the directed enabled or not. */
	BT_MESH_DFW_RELAY_ENABLED,	/* Indicate the directed relay enabled or not. */
	BT_MESH_DFW_FRIEND_ENABLED,	/* Indicate the directed friend enabled or not. */

	BT_MESH_DFW_CFG_STORE_PENDING,
	BT_MESH_DFW_FW_STORE_PENDING,

	BT_MESH_DFW_TOTALS,
};

/** Discovery flags */
enum {
	BT_MESH_DFW_FLAG_PATH_REPLY_RECVED,	/* Indicate the path reply receviced */
	BT_MESH_DFW_FLAG_PATH_CONFIRM_SENT,	/* Indicate path confirm has been sent */

	BT_MESH_DFW_FLAG_TOTALS,
};

/** Forwarding flags */
enum {
	BT_MESH_DFW_FORWARDING_FLAG_ECHO_REPLY, /* Indicate the echo reply received */
	BT_MESH_DFW_FORWARDING_FLAG_STORE_PENDING,

	BT_MESH_DFW_FORWARDING_FLAG_TOTALS,
};

/** Path Lifetime */
enum bt_mesh_dfw_path_lifetime {
	BT_MESH_DFW_PATH_LIFETIME_12_MINUTERS,	/* Path lifetime is 12 minutes */
	BT_MESH_DFW_PATH_LIFETIME_2_HOURS,	/* Path lifetime is 2 hours */
	BT_MESH_DFW_PATH_LIFETIME_24_HOURS,	/* Path lifetime is 24 hours */
	BT_MESH_DFW_PATH_LIFETIME_10_DAYS,	/* Path lifetime is 10 days */
};

/** Lane Discovery Guard Interval */
enum bt_mesh_dfw_lane_discov_guard_intv {
	BT_MESH_DFW_LANE_DISCOVERY_GUARD_2_S,	/* Lane discovery guard interval is 2 seconds */
	BT_MESH_DFW_LANE_DISCOVERY_GUARD_10_S,	/* Lane discovery guard interval is 10 seconds */
};

/** Path Discovery Interval */
enum bt_mesh_dfw_path_discov_intv {
	BT_MESH_DFW_PATH_DISCOV_INTV_5_S,	/* Path discovery interval is 5 seconds */
	BT_MESH_DFW_PATH_DISCOV_INTV_30_S,	/* Path discovery interval is 30 seconds */
};

/** Directed Forwarding node */
struct bt_mesh_dfw_node {
	uint16_t addr;			/* The primary element addresses of node */
	uint8_t  secondary_count;	/* The numbers of secondary elements of node */
};

/** Directed Discovery Table */
struct bt_mesh_dfw_discovery {
	/** flags */
	ATOMIC_DEFINE(flags, BT_MESH_DFW_FLAG_TOTALS);
	/** Path Origin */
	struct bt_mesh_dfw_node path_origin;
	/** List of dependent nodes of the Path Origin */
	struct bt_mesh_dfw_node dependent_origin[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	/** Forwarding number of the Path Origin */
	uint8_t	 forwarding_number;
	/** Path lifetime specified by the Path Origin */
	enum bt_mesh_dfw_path_lifetime lifetime;
	/** Path discovery interval specified by the Path Origin */
	enum bt_mesh_dfw_path_discov_intv interval;
	/** Path metric value for the path */
	uint8_t metric;
	/** The unicast address, group address, or virtual address of the destination */
	uint16_t destination;
	/** Primary element address of the node selected as the next hop toward the Path Origin */
	uint16_t next_toward_path_origin;
	/** The bearer index to be used for forwarding messages directed to the Path Origin */
	uint16_t bearer_toward_path_origin;
	/** Path Lane Guard timer */
	struct k_work_delayable lane_guard_timer;
	/** Path Request Delay timer */
	struct k_work_delayable request_delay_timer;
	/** Path Reply Delay timer */
	struct k_work_delayable reply_delay_timer;
	/** Path Discovery timer */
	struct k_work_delayable timer;
};

/** Directed Forwarding Table */
struct bt_mesh_dfw_forwarding {
	/** flags */
	ATOMIC_DEFINE(flags, BT_MESH_DFW_FORWARDING_FLAG_TOTALS);
	/** Indicating whether or not the path is a fixed path */
	bool fixed_path;
	/** Indicating whether or not the backward path has been validated */
	bool backward_path_validated;
	/** Indicating whether or not the path is ready for use */
	bool path_not_ready;
	/** Path Origin */
	struct bt_mesh_dfw_node path_origin;
	/** List of dependent nodes of the Path Origin */
	struct bt_mesh_dfw_node dependent_origin[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	/** Path Target */
	struct bt_mesh_dfw_node path_target;
	/** List of dependent nodes of the Path Target */
	struct bt_mesh_dfw_node dependent_target[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	/** Forwarding number of the Path Origin */
	uint8_t	 forwarding_number;
	/** Number of lanes discovered */
	uint8_t	 lane_count;
	/** The bearer index to be used for forwarding messages directed to the Path Origin */
	uint16_t bearer_toward_path_origin;
	/** The bearer index to be used for forwarding messages directed to the Path Target */
	uint16_t bearer_toward_path_target;
	/** Path Echo interval */
	k_timeout_t echo_intv;
	/** Path Echo timer */
	struct k_work_delayable echo_timer;
	/** Path lifetime timer */
	struct k_work_delayable timer;
};

struct bt_mesh_dfw_subnet {
	ATOMIC_DEFINE(flags, BT_MESH_DFW_TOTALS);

	uint16_t net_idx;

	uint16_t update_id;
	uint8_t max_concurr_init;
	uint8_t wanted_lanes;
	bool    two_way_path;
	uint8_t unicast_echo_intv;
	uint8_t multicast_echo_intv;
	enum bt_mesh_dfw_path_lifetime lifetime;

	/* Each node manages a forwarding number for each subnet */
	uint8_t forwarding_number;
	struct bt_mesh_dfw_discovery  discovery[BT_MESH_DFW_DISCOVERY_COUNT];
	struct bt_mesh_dfw_forwarding forwarding[BT_MESH_DFW_FORWARDING_COUNT];
};

uint8_t bt_mesh_dfw_net_transmit_get(void);
void bt_mesh_dfw_net_transmit_set(uint8_t xmit);
uint8_t bt_mesh_dfw_relay_retransmit_get(void);
void bt_mesh_dfw_relay_retransmit_set(uint8_t xmit);
uint8_t bt_mesh_dfw_ctl_net_transmit_get(void);
void bt_mesh_dfw_ctl_net_transmit_set(uint8_t xmit);
uint8_t bt_mesh_dfw_ctl_relay_retransmit_get(void);
void bt_mesh_dfw_ctl_relay_retransmit_set(uint8_t xmit);

void bt_mesh_dfw_pending_store(void);

int bt_mesh_dfw_get(uint16_t net_idx, enum bt_mesh_feat_state *state);
int bt_mesh_dfw_set(uint16_t net_idx, enum bt_mesh_feat_state state);
int bt_mesh_dfw_relay_get(uint16_t net_idx, enum bt_mesh_feat_state *state);
int bt_mesh_dfw_relay_set(uint16_t net_idx, enum bt_mesh_feat_state state);
int bt_mesh_dfw_friend_get(uint16_t net_idx, enum bt_mesh_feat_state *state);
int bt_mesh_dfw_friend_set(uint16_t net_idx, enum bt_mesh_feat_state state);

int bt_mesh_dfw_path_request(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_dfw_path_reply(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_dfw_path_confirm(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_dfw_path_echo_request(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_dfw_path_echo_reply(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_dfw_dependent_node_update(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_dfw_path_request_solicitation(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);

enum bt_mesh_net_if bt_mesh_dfw_path_existed(uint16_t net_idx, uint16_t src, uint16_t dst);

bool bt_mesh_dfw_path_origin_state_machine_existed(uint16_t net_idx, uint16_t dest);

void bt_mesh_dfw_path_origin_state_machine_msg_sent(uint16_t net_idx, uint16_t dest);

int bt_mesh_dfw_path_origin_state_machine_start(uint16_t net_idx,
						const struct bt_mesh_dfw_node *dependent,
						uint16_t dest, bool power_up);
bool bt_mesh_dfw_dependent_node_existed(uint16_t net_idx, uint16_t src, uint16_t dst,
					const struct bt_mesh_dfw_node *dependent);
int bt_mesh_dfw_dependent_node_update_start(uint16_t net_idx,
					    const struct bt_mesh_dfw_node *dependent, bool type);

int bt_mesh_dfw_path_request_solicitation_start(uint16_t net_idx, const uint16_t addr_list[],
						uint16_t len);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_DFW_H_ */

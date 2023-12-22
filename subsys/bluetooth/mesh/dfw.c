/*
 * Copyright (c) 2023 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "transport.h"
#include "heartbeat.h"
#include "access.h"
#include "beacon.h"
#include "foundation.h"
#include "friend.h"
#include "proxy.h"
#include "dfw.h"
#include "settings.h"

#define LOG_LEVEL CONFIG_BT_MESH_DFW_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_dfw);

/* 3.6.8.4 Forwarding number
 * For example, forwarding numbers 1 to 127 are greater than forwarding number 0, but forwarding
 * numbers 128 to 255 are less than forwarding number 0.
 */
#define DFW_NUM_A_LESS_B(_a, _b)	((uint8_t)((_a) - (_b)) >= 128)

#define DFW_NUM_INITIAL_VAL		0xff

/** Metric type  */
#define DFW_PATH_METRIC_NODE		0

#define _BITS_GET(_val, _bit, _mask)	(((_val) >> _bit) & _mask)
#define  BITS_GET(_val, _bit, _mask)	_BITS_GET((_val)->octer, _bit, _mask)

/** Length Present */
#define LEN_PST_BIT			15
#define LEN_PST(_val)			_BITS_GET(_val, LEN_PST_BIT, BIT_MASK(1))
/** On Behalf Of Dependent Origin */
#define OBO_DO_BIT			7
#define OBO_DO(_val)			BITS_GET(_val, OBO_DO_BIT, BIT_MASK(1))
/** Path Origin Path Metric Type */
#define PO_PMT_BIT			4
#define PO_PMT(_val)			BITS_GET(_val, PO_PMT_BIT, BIT_MASK(3))
/** Path Origin Path Lifetime */
#define PO_PLT_BIT			2
#define PO_PLT(_val)			BITS_GET(_val, PO_PLT_BIT, BIT_MASK(2))
/** Path Discovery Interval */
#define PD_INT_BIT			1
#define PD_INT(_val)			BITS_GET(_val, PD_INT_BIT, BIT_MASK(1))

/** Confirmation Request */
#define CFM_REQ_BIT			5
#define CFM_REQ(_val)			BITS_GET(_val, CFM_REQ_BIT, BIT_MASK(1))
/** On Behalf Of Dependent Target */
#define OBO_DT_BIT			6
#define OBO_DT(_val)			BITS_GET(_val, OBO_DT_BIT, BIT_MASK(1))
/** Unicast Destination */
#define UST_DST_BIT			7
#define UST_DST(_val)			BITS_GET(_val, UST_DST_BIT, BIT_MASK(1))

#define ADDR_RANGE_IN(_addr, _node)	(((_addr) >= (_node)->addr) && \
					 ((_addr) <= ((_node)->addr + (_node)->secondary_count)))

/** 3.6.8.7 Directed forwarding constants */
#define PATH_REPLY_DELAY_MS		(500)
#define PATH_REQUEST_DELAY_MS		(150)

#if defined(CONFIG_BT_MESH_DFW_PATH_LIFETIME_12_MINUTERS)
#define PATH_LIFETIME			BT_MESH_DFW_PATH_LIFETIME_12_MINUTERS
#elif defined(CONFIG_BT_MESH_DFW_PATH_LIFETIME_2_HOURS)
#define PATH_LIFETIME			BT_MESH_DFW_PATH_LIFETIME_2_HOURS
#elif defined(CONFIG_BT_MESH_DFW_PATH_LIFETIME_24_HOURS)
#define PATH_LIFETIME			BT_MESH_DFW_PATH_LIFETIME_24_HOURS
#else
#define PATH_LIFETIME			BT_MESH_DFW_PATH_LIFETIME_10_DAYS
#endif

#if defined(CONFIG_BT_MESH_DFW_PATH_DISCOV_INTV_5_S)
#define PATH_DISCOV_INTERVAL		BT_MESH_DFW_PATH_DISCOV_INTV_5_S
#else
#define PATH_DISCOV_INTERVAL		BT_MESH_DFW_PATH_DISCOV_INTV_30_S
#endif

#if defined(CONFIG_BT_MESH_DFW_LANE_DISCOV_GUARD_INTV_2_S)
#define LANE_DISCOVERY_GUARD		BT_MESH_DFW_LANE_DISCOVERY_GUARD_2_S
#else
#define LANE_DISCOVERY_GUARD		BT_MESH_DFW_LANE_DISCOVERY_GUARD_10_S
#endif

#if defined(CONFIG_BT_MESH_DFW_ENABLED)
#define DFW_ENABLED			BIT(BT_MESH_DFW_ENABLED)
#else
#define DFW_ENABLED			0
#endif

#if defined(CONFIG_BT_MESH_DFW_RELAY_ENABLED)
#define DFW_RELAY_ENABLED		BIT(BT_MESH_DFW_RELAY_ENABLED)
#else
#define DFW_RELAY_ENABLED		0
#endif

#if defined(CONFIG_BT_MESH_DFW_FRIEND_ENABLED)
#define DFW_FRIEND_ENABLED		BIT(BT_MESH_DFW_FRIEND_ENABLED)
#else
#define DFW_FRIEND_ENABLED		0
#endif

enum dfw_state_machine_states {
	DFW_STATE_MACHINE_INITIAL,
	DFW_STATE_MACHINE_POWER_UP,
	DFW_STATE_MACHINE_PATH_DISCOV,
	DFW_STATE_MACHINE_PATH_IN_USE,
	DFW_STATE_MACHINE_PATH_VALID,
	DFW_STATE_MACHINE_PATH_MON,
	DFW_STATE_MACHINE_PATH_DISCOV_RETRY_WAIT,
	DFW_STATE_MACHINE_FINAL,
};

enum dfw_state_machine_events {
	DFW_STATE_MACHINE_EVENT_PATH_NEEDED,
	DFW_STATE_MACHINE_EVENT_PATH_NOT_NEEDED,
	DFW_STATE_MACHINE_EVENT_POWER_UP_EXECUTED,
	DFW_STATE_MACHINE_EVENT_PATH_DISCOV_SUCCEED,
	DFW_STATE_MACHINE_EVENT_PATH_DISCOV_FAILED,
	DFW_STATE_MACHINE_EVENT_PATH_VALID_STARTED,
	DFW_STATE_MACHINE_EVENT_PATH_VALID_FAILED,
	DFW_STATE_MACHINE_EVENT_PATH_VALID_SUCCEED,
	DFW_STATE_MACHINE_EVENT_PATH_REMOVED,
	DFW_STATE_MACHINE_EVENT_PATH_SOLICITED,
	DFW_STATE_MACHINE_EVENT_PATH_MON_STARTED,
};

/* Fixed directed forwarding path information for persistent storage. */
struct dfw_forwarding_val {
	uint8_t backward_path_validated;
	uint8_t path_origin_secondary_count;
	uint8_t path_target_secondary_count;
	uint16_t bearer_toward_path_origin;
	uint16_t bearer_toward_path_target;
} __packed;

/* Dependent node information for persistent storage. */
struct dfw_dependent_val {
	uint8_t is_dependent_origin;
	uint8_t secondary_count;
} __packed;

/* Directed forwarding configuration information for persistent storage. */
struct dfw_cfg_val {
	uint8_t  rssi_margin;
	uint16_t monitor_intv;
	uint16_t discov_retry_intv;
	uint8_t discov_intv;
	uint8_t lane_discov_guard_intv;
	uint8_t directed_net_transmit;
	uint8_t directed_relay_retransmit;
	uint8_t directed_ctl_net_transmit;
	uint8_t directed_ctl_relay_retransmit;
} __packed;

/* Directed forwarding subnet configuration information for persistent storage. */
struct dfw_subnet_cfg_val {
	uint8_t dfw;
	uint8_t relay;
	uint8_t friend;
	uint8_t lifetime;
	uint8_t max_concurr_init;
	uint8_t wanted_lanes;
	uint8_t two_way_path;
	uint8_t unicast_echo_intv;
	uint8_t multicast_echo_intv;
} __packed;

static void dfw_lane_discov_guard_timer_expired(struct k_work *work);
static void dfw_path_request_delay_expired(struct k_work *work);
static void dfw_path_reply_delay_expired(struct k_work *work);
static void dfw_path_echo_expired(struct k_work *work);
static void dfw_path_lifetime_expired(struct k_work *work);
static int dfw_path_initialize_start(struct bt_mesh_subnet *sub,
				     const struct bt_mesh_dfw_node *dependent,
				     uint16_t dst);
static void dfw_state_machine_expire(struct k_work *work);

struct {
	atomic_t store;
	uint8_t rssi_margin;
	uint16_t monitor_intv;
	uint16_t discov_retry_intv;
	enum bt_mesh_dfw_path_discov_intv discov_intv;
	enum bt_mesh_dfw_lane_discov_guard_intv lane_discov_guard_intv;
	uint8_t directed_net_transmit;
	uint8_t directed_relay_retransmit;
	uint8_t directed_ctl_net_transmit;
	uint8_t directed_ctl_relay_retransmit;
} dfw_cfg = {
	.rssi_margin = CONFIG_BT_MESH_DFW_RSSI_MARGEN,
	.monitor_intv = CONFIG_BT_MESH_DFW_PATH_MON_INTV,
	.discov_retry_intv = CONFIG_BT_MESH_DFW_PATH_DISCOV_RETRY_INTV,
	.discov_intv = PATH_DISCOV_INTERVAL,
	.lane_discov_guard_intv = LANE_DISCOVERY_GUARD,
	.directed_net_transmit = BT_MESH_TRANSMIT(CONFIG_BT_MESH_DFW_NET_TRANS_COUNT,
						  CONFIG_BT_MESH_DFW_NET_TRANS_INTV),
	.directed_relay_retransmit = BT_MESH_TRANSMIT(CONFIG_BT_MESH_DFW_RELAY_RETRANS_COUNT,
						      CONFIG_BT_MESH_DFW_RELAY_RETRANS_INTV),
	.directed_ctl_net_transmit = BT_MESH_TRANSMIT(CONFIG_BT_MESH_DFW_CTL_NET_TRANS_COUNT,
						      CONFIG_BT_MESH_DFW_CTL_NET_TRANS_INTV),
	.directed_ctl_relay_retransmit =
			BT_MESH_TRANSMIT(CONFIG_BT_MESH_DFW_CTL_RELAY_RETRANS_COUNT,
					 CONFIG_BT_MESH_DFW_CTL_RELAY_RERETANS_INTV),
};

static struct bt_mesh_dfw_subnet dfw_subnets[CONFIG_BT_MESH_SUBNET_COUNT] = {
	[0 ... (CONFIG_BT_MESH_SUBNET_COUNT - 1)] = {
		.flags = {
			ATOMIC_INIT(DFW_ENABLED |
				    DFW_RELAY_ENABLED |
				    DFW_FRIEND_ENABLED),
		},
		.net_idx = BT_MESH_KEY_UNUSED,
		.forwarding_number = DFW_NUM_INITIAL_VAL,
		.lifetime = PATH_LIFETIME,
		.max_concurr_init = CONFIG_BT_MESH_DFW_DISCOVERY_MAX_CONCURR_INIT,
		.wanted_lanes = CONFIG_BT_MESH_DFW_WANTED_LANES_COUNT,
		.two_way_path = IS_ENABLED(CONFIG_BT_MESH_DFW_TWO_WAY_PATH),
		.unicast_echo_intv = CONFIG_BT_MESH_DFW_UNICAST_ECHO_INTV,
		.multicast_echo_intv = CONFIG_BT_MESH_DFW_MULTICAST_ECHO_INTV,
		.discovery = {
			[0 ... (CONFIG_BT_MESH_DFW_DISCOVERY_COUNT - 1)] = {
				.lane_guard_timer = Z_WORK_DELAYABLE_INITIALIZER(
							dfw_lane_discov_guard_timer_expired),
				.request_delay_timer = Z_WORK_DELAYABLE_INITIALIZER(
								dfw_path_request_delay_expired),
				.reply_delay_timer = Z_WORK_DELAYABLE_INITIALIZER(
							dfw_path_reply_delay_expired),
			},
		},
		.forwarding = {
			[0 ... (CONFIG_BT_MESH_DFW_FORWARDING_COUNT - 1)] = {
				.echo_timer = Z_WORK_DELAYABLE_INITIALIZER(dfw_path_echo_expired),
				.timer = Z_WORK_DELAYABLE_INITIALIZER(dfw_path_lifetime_expired),
			},
		},
	},
};

static struct dfw_state_machine {
	enum dfw_state_machine_states state;
	struct bt_mesh_dfw_node dependent;
	uint16_t net_idx;
	uint16_t dst;
	bool sent;
	struct bt_mesh_dfw_forwarding *fw;
	struct k_work_delayable timer;
} state_machines[CONFIG_BT_MESH_DFW_STATE_MACHINE_COUNT] = {
	[0 ... (CONFIG_BT_MESH_DFW_STATE_MACHINE_COUNT - 1)] = {
		.state = DFW_STATE_MACHINE_FINAL,
		.timer = Z_WORK_DELAYABLE_INITIALIZER(dfw_state_machine_expire),
	},
};

static inline k_timeout_t path_lifetime_get(enum bt_mesh_dfw_path_lifetime lifetime)
{
	if (lifetime == BT_MESH_DFW_PATH_LIFETIME_12_MINUTERS) {
		return K_MINUTES(12);
	} else if (lifetime == BT_MESH_DFW_PATH_LIFETIME_2_HOURS) {
		return K_HOURS(2);
	} else if (lifetime == BT_MESH_DFW_PATH_LIFETIME_24_HOURS) {
		return K_HOURS(24);
	} else {
		return K_HOURS(24 * 10);
	}
}

static inline k_timeout_t discovery_timeout_get(enum bt_mesh_dfw_path_discov_intv intv)
{
	if (intv == BT_MESH_DFW_PATH_DISCOV_INTV_5_S) {
		return K_SECONDS(5);
	} else {
		return K_SECONDS(30);
	}
}

static inline k_timeout_t lane_discovery_guard_get(enum bt_mesh_dfw_lane_discov_guard_intv intv)
{
	if (intv == BT_MESH_DFW_LANE_DISCOVERY_GUARD_2_S) {
		return K_SECONDS(2);
	} else {
		return K_SECONDS(10);
	}
}

static inline struct bt_mesh_dfw_subnet *
dfw_table_get_by_discovery(struct bt_mesh_dfw_discovery *dv)
{
	return &dfw_subnets[ARRAY_INDEX_FLOOR(dfw_subnets, dv)];
}

static inline struct bt_mesh_dfw_subnet *
dfw_table_get_by_forward(struct bt_mesh_dfw_forwarding *fw)
{
	return &dfw_subnets[ARRAY_INDEX_FLOOR(dfw_subnets, fw)];
}

static struct bt_mesh_dfw_discovery *
dfw_discovery_find_by_forwarding_number(struct bt_mesh_dfw_subnet *dfw,
					struct bt_mesh_dfw_discovery **new,
					uint16_t src,
					uint8_t forwarding_number)
{
	struct bt_mesh_dfw_discovery *dv = NULL;

	for (int i = 0; i < ARRAY_SIZE(dfw->discovery); i++) {
		if (new && dfw->discovery[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			*new = &dfw->discovery[i];
			continue;
		}

		if (dfw->discovery[i].path_origin.addr != src) {
			continue;
		}

		if (forwarding_number == dfw->discovery[i].forwarding_number) {
			dv = &dfw->discovery[i];
			break;
		}
	}

	return dv;
}

static struct bt_mesh_dfw_forwarding *
dfw_forwarding_find_by_dst(struct bt_mesh_dfw_subnet *dfw,
			   struct bt_mesh_dfw_forwarding **new,
			   uint16_t src, uint16_t dst,
			   bool dependent_include, bool fixed)
{
	struct bt_mesh_dfw_forwarding *fw = NULL;

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (new && dfw->forwarding[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			*new = &dfw->forwarding[i];
			continue;
		}

		if (dfw->forwarding[i].fixed_path != fixed) {
			continue;
		}

		if (dfw->forwarding[i].path_origin.addr != src) {
			continue;
		}

		if (ADDR_RANGE_IN(dst, &dfw->forwarding[i].path_target)) {
			fw = &dfw->forwarding[i];
			break;
		}

		if (!dependent_include) {
			continue;
		}

		for (int j = 0; i < ARRAY_SIZE(dfw->forwarding[i].dependent_target); j++) {
			if (ADDR_RANGE_IN(dst, &dfw->forwarding[i].dependent_target[j])) {
				fw = &dfw->forwarding[i];
				break;
			}
		}
	}

	return fw;
}

static struct bt_mesh_dfw_forwarding *
dfw_forwarding_find_by_forwarding_number(struct bt_mesh_dfw_subnet *dfw,
					 struct bt_mesh_dfw_forwarding **new,
					 uint16_t src,
					 uint8_t forwarding_number,
					 bool fixed)
{
	struct bt_mesh_dfw_forwarding *fw = NULL;

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (new && dfw->forwarding[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			*new = &dfw->forwarding[i];
			continue;
		}

		if (dfw->forwarding[i].fixed_path != fixed) {
			continue;
		}

		if (dfw->forwarding[i].path_origin.addr != src) {
			continue;
		}

		if (dfw->forwarding[i].forwarding_number == forwarding_number) {
			fw = &dfw->forwarding[i];
			break;
		}
	}

	return fw;
}

static struct bt_mesh_dfw_node *
dfw_forwarding_dependent_origin_find(struct bt_mesh_dfw_forwarding *fw,
				     const struct bt_mesh_dfw_node *dependent,
				     struct bt_mesh_dfw_node **dependent_new)
{
	for (int j = 0; j < ARRAY_SIZE(fw->dependent_origin); j++) {
		if (fw->dependent_origin[j].addr == dependent->addr) {
			return &fw->dependent_origin[j];
		} else if (dependent_new &&
			   fw->dependent_origin[j].addr == BT_MESH_ADDR_UNASSIGNED) {
			*dependent_new = &fw->dependent_origin[j];
		}
	}

	return NULL;
}

static struct bt_mesh_dfw_node *
dfw_forwarding_dependent_target_find(struct bt_mesh_dfw_forwarding *fw,
				     const struct bt_mesh_dfw_node *dependent,
				     struct bt_mesh_dfw_node **dependent_new)
{
	for (int j = 0; j < ARRAY_SIZE(fw->dependent_target); j++) {
		if (fw->dependent_target[j].addr == dependent->addr) {
			return &fw->dependent_target[j];
		} else if (dependent_new &&
			   fw->dependent_target[j].addr == BT_MESH_ADDR_UNASSIGNED) {
			*dependent_new = &fw->dependent_target[j];
		}
	}

	return NULL;
}

static int dfw_cfg_set(const char *name, size_t len_rd,
		       settings_read_cb read_cb, void *cb_arg)
{
	struct dfw_cfg_val cfg;
	int err;

	if (len_rd == 0) {
		LOG_DBG("Cleared directed forwarding configuration value");
		return 0;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &cfg, sizeof(cfg));
	if (err) {
		LOG_ERR("Failed to set \'cfg\'");
		return err;
	}

	dfw_cfg.rssi_margin = cfg.rssi_margin;
	dfw_cfg.monitor_intv = cfg.monitor_intv;
	dfw_cfg.lane_discov_guard_intv = cfg.lane_discov_guard_intv;
	dfw_cfg.discov_retry_intv = cfg.discov_retry_intv;
	dfw_cfg.discov_intv = cfg.discov_intv;
	dfw_cfg.directed_relay_retransmit = cfg.directed_relay_retransmit;
	dfw_cfg.directed_net_transmit = cfg.directed_net_transmit;
	dfw_cfg.directed_ctl_relay_retransmit = cfg.directed_ctl_relay_retransmit;
	dfw_cfg.directed_ctl_net_transmit = cfg.directed_ctl_net_transmit;

	LOG_DBG("Restored directed forwarding configuration value");

	return 0;
}

static struct bt_mesh_dfw_subnet *dfw_subnet_update_find(uint16_t net_idx,
							 struct bt_mesh_dfw_subnet **new)
{
	for (int i = 0; i < ARRAY_SIZE(dfw_subnets); i++) {
		if (new && dfw_subnets[i].net_idx == BT_MESH_KEY_UNUSED) {
			*new = &dfw_subnets[i];
			continue;
		}

		if (dfw_subnets[i].net_idx == net_idx) {
			return &dfw_subnets[i];
		}
	}

	return NULL;
}

static int dfw_subnet_cfg_set(const char *name, size_t len_rd,
			      settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_dfw_subnet *dfw, *new = NULL;
	struct dfw_subnet_cfg_val cfg;
	uint16_t net_idx;
	int err;

	if (len_rd == 0) {
		LOG_DBG("Cleared directed forwarding subnet configuration value");
		return 0;
	}

	net_idx = strtol(name, NULL, 16);

	dfw = dfw_subnet_update_find(net_idx, &new);
	if (!dfw) {
		if (!new) {
			LOG_ERR("Unable find entry for directed forwarding value");
			return -ENOENT;
		}

		new->net_idx = net_idx;
		dfw = new;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &cfg, sizeof(cfg));
	if (err) {
		LOG_ERR("Failed to set \'directed forwarding subnet configuration\'");
		return err;
	}

	atomic_set_bit_to(dfw->flags, BT_MESH_DFW_ENABLED, cfg.dfw);
	atomic_set_bit_to(dfw->flags, BT_MESH_DFW_RELAY_ENABLED, cfg.relay);
	atomic_set_bit_to(dfw->flags, BT_MESH_DFW_FRIEND_ENABLED, cfg.friend);

	dfw->lifetime = cfg.lifetime;
	dfw->max_concurr_init = cfg.max_concurr_init;
	dfw->wanted_lanes = cfg.wanted_lanes;
	dfw->two_way_path = cfg.two_way_path;
	dfw->unicast_echo_intv = cfg.unicast_echo_intv;
	dfw->multicast_echo_intv = cfg.multicast_echo_intv;

	LOG_DBG("Restored directed forwarding subnet configuration value");

	return 0;
}

static struct bt_mesh_dfw_forwarding *dfw_forwarding_get(const char *name, const char **after,
							 settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_dfw_forwarding *fw, *fw_new = NULL;
	struct bt_mesh_dfw_subnet *dfw, *new = NULL;
	uint16_t net_idx, path_origin, dst;
	const char *next;
	int rc;

	net_idx = strtol(name, NULL, 16);

	rc = settings_name_next(name, &next);
	if (!rc || !next) {
		return NULL;
	}

	path_origin = strtol(next, NULL, 16);

	rc = settings_name_next(next, &next);
	if (!rc || !next) {
		return NULL;
	}

	dst = strtol(next, NULL, 16);

	if (after) {
		*after = next;
	}

	dfw = dfw_subnet_update_find(net_idx, &new);
	if (!dfw) {
		if (!new) {
			LOG_ERR("Unable find entry for directed forwarding value");
			return NULL;
		}

		new->net_idx = net_idx;
		dfw = new;
	}

	fw = dfw_forwarding_find_by_dst(dfw, &fw_new, path_origin, dst, false, true);
	if (fw) {
		return fw;
	}

	if (!fw_new) {
		return NULL;
	}

	fw_new->fixed_path = true;
	fw_new->lane_count = 1;
	fw_new->path_origin.addr = path_origin;
	fw_new->path_target.addr = dst;

	return fw_new;
}

static int dfw_subnet_forwarding_set(const char *name, size_t len_rd,
				     settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_dfw_forwarding *fw = NULL;
	struct dfw_forwarding_val val;
	int err;

	if (len_rd == 0) {
		LOG_DBG("Cleared directed forwarding state");
		return 0;
	}

	fw = dfw_forwarding_get(name, NULL, read_cb, cb_arg);
	if (!fw) {
		return -ENOENT;
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &val, sizeof(val));
	if (err) {
		LOG_ERR("Failed to set \'directed forwarding entry\'");
		return err;
	}

	fw->backward_path_validated = val.backward_path_validated;
	fw->path_origin.secondary_count = val.path_origin_secondary_count;
	fw->path_target.secondary_count = val.path_target_secondary_count;
	fw->bearer_toward_path_origin = val.bearer_toward_path_origin;
	fw->bearer_toward_path_target = val.bearer_toward_path_target;

	LOG_DBG("Restored directed forwarding state");

	return 0;
}

static int dfw_subnet_dependent_set(const char *name, size_t len_rd,
				    settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_dfw_node dependent, *dependent_new = NULL;
	struct dfw_dependent_val val;
	struct bt_mesh_dfw_forwarding *fw;
	uint16_t dependent_addr;
	const char *after;
	int err;

	if (len_rd == 0) {
		LOG_DBG("Cleared directed forwarding dependent state");
		return 0;
	}

	fw = dfw_forwarding_get(name, &after, read_cb, cb_arg);
	if (!fw) {
		LOG_ERR("Unable get forwarding entry");
		return -ENOENT;
	}

	if (!after) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	dependent_addr = strtol(after, NULL, 16);

	err = bt_mesh_settings_set(read_cb, cb_arg, &val, sizeof(val));
	if (err) {
		LOG_ERR("Failed to set \'directed forwarding dependent entry\'");
		return err;
	}

	dependent.addr = dependent_addr;
	dependent.secondary_count = val.secondary_count;

	if (val.is_dependent_origin) {
		if (dfw_forwarding_dependent_origin_find(fw, &dependent, &dependent_new)) {
			/* duplicated dependent origin */
			return 0;
		}
	} else {
		if (dfw_forwarding_dependent_target_find(fw, &dependent, &dependent_new)) {
			/* duplicated dependent target */
			return 0;
		}
	}

	if (!dependent_new) {
		LOG_ERR("Unable find entry for directed forwarding dependent state");
		return -ENOENT;
	}

	dependent_new->addr = dependent.addr;
	dependent_new->secondary_count = dependent.secondary_count;

	LOG_DBG("Restored directed forwarding dependent state");

	return 0;
}

static int dfw_subnet_set(const char *name, size_t len_rd,
			  settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int len;

	len = settings_name_next(name, &next);

	if (!next) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	if (!strncmp(name, "Cfg", len)) {
		return dfw_subnet_cfg_set(next, len_rd, read_cb, cb_arg);
	}

	if (!strncmp(name, "Fw", len)) {
		return dfw_subnet_forwarding_set(next, len_rd, read_cb, cb_arg);
	}

	if (!strncmp(name, "Dep", len)) {
		return dfw_subnet_dependent_set(next, len_rd, read_cb, cb_arg);
	}

	LOG_WRN("Unknown module key %s", name);

	return -ENOENT;
}

static int dfw_set(const char *name, size_t len_rd,
		   settings_read_cb read_cb, void *cb_arg)
{
	int len;
	const char *next;

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	if (!strcmp(name, "Cfg")) {
		return dfw_cfg_set(name, len_rd, read_cb, cb_arg);
	}

	len = settings_name_next(name, &next);

	if (!next) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	if (!strncmp(name, "Sub", len)) {
		return dfw_subnet_set(next, len_rd, read_cb, cb_arg);
	}

	LOG_WRN("Unknown module key %s", name);

	return -ENOENT;
}

BT_MESH_SETTINGS_DEFINE(dfw, "DFW", dfw_set);

static void dfw_cfg_store(void)
{
	atomic_set(&dfw_cfg.store, true);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_DFW_PENDING);
}

static void dfw_subnet_cfg_store(struct bt_mesh_dfw_subnet *dfw)
{
	atomic_set_bit(dfw->flags, BT_MESH_DFW_CFG_STORE_PENDING);
	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_DFW_PENDING);
}

static void dfw_subnet_forwarding_store(struct bt_mesh_dfw_forwarding *fw)
{
	atomic_set_bit(fw->flags, BT_MESH_DFW_FORWARDING_FLAG_STORE_PENDING);
	atomic_set_bit(dfw_table_get_by_forward(fw)->flags, BT_MESH_DFW_FW_STORE_PENDING);

	bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_DFW_PENDING);
}

static void store_dfw_pending_cfg(void)
{
	struct dfw_cfg_val cfg;
	int err;

	LOG_DBG("");

	cfg.rssi_margin = dfw_cfg.rssi_margin;
	cfg.monitor_intv = dfw_cfg.monitor_intv;
	cfg.lane_discov_guard_intv = dfw_cfg.lane_discov_guard_intv;
	cfg.discov_retry_intv = dfw_cfg.discov_retry_intv;
	cfg.discov_intv = dfw_cfg.discov_intv;
	cfg.directed_relay_retransmit = dfw_cfg.directed_relay_retransmit;
	cfg.directed_net_transmit = dfw_cfg.directed_net_transmit;
	cfg.directed_ctl_relay_retransmit = dfw_cfg.directed_ctl_relay_retransmit;
	cfg.directed_ctl_net_transmit = dfw_cfg.directed_ctl_net_transmit;

	err = settings_save_one("bt/mesh/DFW/Cfg", &cfg, sizeof(cfg));
	if (err) {
		LOG_ERR("Failed to store directed forwarding configuration value");
	} else {
		LOG_DBG("Stored directed forwarding configuration value");
	}
}

static void clear_dfw_pending_cfg(void)
{
	int err;

	LOG_DBG("");

	err = settings_delete("bt/mesh/DFW/Cfg");
	if (err) {
		LOG_ERR("Failed to clear directed forwarding configuration value");
	} else {
		LOG_DBG("Cleared directed forwarding configuration value");
	}
}

static void store_dfw_pending_subnet_cfg(struct bt_mesh_dfw_subnet *dfw)
{
	struct dfw_subnet_cfg_val cfg;
	char path[36];
	int err;

	LOG_DBG("");

	cfg.dfw = atomic_test_bit(dfw->flags, BT_MESH_DFW_ENABLED);
	cfg.relay = atomic_test_bit(dfw->flags, BT_MESH_DFW_RELAY_ENABLED);
	cfg.friend = atomic_test_bit(dfw->flags, BT_MESH_DFW_FRIEND_ENABLED);

	cfg.lifetime = dfw->lifetime;
	cfg.max_concurr_init = dfw->max_concurr_init;
	cfg.wanted_lanes = dfw->wanted_lanes;
	cfg.two_way_path = dfw->two_way_path;
	cfg.unicast_echo_intv = dfw->unicast_echo_intv;
	cfg.multicast_echo_intv = dfw->multicast_echo_intv;

	snprintk(path, sizeof(path), "bt/mesh/DFW/Sub/Cfg/%x", dfw->net_idx);

	err = settings_save_one(path, &cfg, sizeof(cfg));
	if (err) {
		LOG_ERR("Failed to store directed forwarding subnet configuration value");
	} else {
		LOG_DBG("Stored directed forwarding subnet configuration value");
	}
}

static void dfw_subnet_cfg_clear(struct bt_mesh_dfw_subnet *dfw)
{
	char path[36];
	int err;

	LOG_DBG("");

	snprintk(path, sizeof(path), "bt/mesh/DFW/Sub/Cfg/%x", dfw->net_idx);

	err = settings_delete(path);
	if (err) {
		LOG_ERR("Failed to clear directed forwarding subnet configuration value");
	} else {
		LOG_DBG("Cleared directed forwarding subnet configuration value");
	}
}

static void store_dfw_pending_subnet_forwarding(struct bt_mesh_dfw_forwarding *fw)
{
	struct dfw_forwarding_val val;
	char path[40];
	int err;

	LOG_DBG("");

	val.backward_path_validated = fw->backward_path_validated;
	val.path_origin_secondary_count = fw->path_origin.secondary_count;
	val.path_target_secondary_count = fw->path_target.secondary_count;
	val.bearer_toward_path_origin = fw->bearer_toward_path_origin;
	val.bearer_toward_path_target = fw->bearer_toward_path_target;

	snprintk(path, sizeof(path), "bt/mesh/DFW/Sub/Fw/%x/%x/%x",
		 dfw_table_get_by_forward(fw)->net_idx, fw->path_origin.addr,
		 fw->path_target.addr);

	err = settings_save_one(path, &val, sizeof(val));
	if (err) {
		LOG_ERR("Failed to store directed forwarding subnet forwarding value");
	} else {
		LOG_DBG("Stored directed forwarding subnet forwarding value");
	}
}

static void dfw_subnet_forwarding_dependent_store(struct bt_mesh_dfw_forwarding *fw,
						  struct bt_mesh_dfw_node *dependent,
						  bool is_dependent_origin)
{
	struct dfw_dependent_val val;
	char path[40];
	int err;

	snprintk(path, sizeof(path), "bt/mesh/DFW/Sub/Dep/%x/%x/%x/%x",
		 dfw_table_get_by_forward(fw)->net_idx,
		 fw->path_origin.addr, fw->path_target.addr,
		 dependent->addr);

	val.is_dependent_origin = is_dependent_origin;
	val.secondary_count = dependent->secondary_count;

	err = settings_delete(path);
	if (err) {
		LOG_ERR("Failed to store directed forwarding dependent value");
	} else {
		LOG_DBG("Stored directed forwarding dependent value");
	}
}

void bt_mesh_dfw_pending_store(void)
{
	if (atomic_cas(&dfw_cfg.store, 1, 0)) {
		if (atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
			store_dfw_pending_cfg();
		} else {
			clear_dfw_pending_cfg();
		}
	}

	for (int i = 0; i < ARRAY_SIZE(dfw_subnets); i++) {
		struct bt_mesh_dfw_subnet *dfw = &dfw_subnets[i];

		/* Only process forwarding store here, no need to check whether it needs to be
		 * clear, due to forwarding clear will be handle by subnet deleted event callback.
		 */
		if (atomic_test_and_clear_bit(dfw->flags, BT_MESH_DFW_CFG_STORE_PENDING)) {
			store_dfw_pending_subnet_cfg(dfw);
		}

		if (!atomic_test_and_clear_bit(dfw->flags, BT_MESH_DFW_FW_STORE_PENDING)) {
			continue;
		}

		for (int j = 0; j < ARRAY_SIZE(dfw->forwarding); j++) {
			if (!atomic_test_and_clear_bit(dfw->forwarding[j].flags,
						BT_MESH_DFW_FORWARDING_FLAG_STORE_PENDING)) {
				continue;
			}

			store_dfw_pending_subnet_forwarding(&dfw->forwarding[j]);
		}
	}
}

static struct dfw_state_machine *dfw_state_machine_find(enum dfw_state_machine_states state,
							uint16_t dst)
{
	for (int i = 0; i < ARRAY_SIZE(state_machines); i++) {
		if (state_machines[i].state == state &&
		    state_machines[i].dst == dst) {
			return &state_machines[i];
		}
	}

	return NULL;
}

static struct dfw_state_machine *dfw_state_machine_find_by_dst(uint16_t dst)
{
	for (int i = 0; i < ARRAY_SIZE(state_machines); i++) {
		if (state_machines[i].dst == dst) {
			return &state_machines[i];
		}
	}

	return NULL;
}

static void dfw_state_machine_state_to_str(struct dfw_state_machine *machine,
					   enum dfw_state_machine_states new)
{
	static const char * const strs[] = {
		"Initial",
		"Power Up",
		"Path Discovery",
		"Path In Use",
		"Path Validation",
		"Path Monitoring",
		"Path Discovery Retry Wait",
		"Final",
	};

	if (machine->state != DFW_STATE_MACHINE_FINAL) {
		LOG_INF("Path State Machine: %s --> %s", strs[machine->state], strs[new]);
	}

}

static void dfw_state_machine_state_set(struct dfw_state_machine *machine,
					enum dfw_state_machine_states state)
{
	enum dfw_state_machine_states old_state = machine->state;
	struct bt_mesh_dfw_node *dependent = NULL;
	k_timeout_t discov_intv, path_intv;
	struct bt_mesh_subnet *sub;
	uint32_t random;
	int err;

	dfw_state_machine_state_to_str(machine, state);

	machine->state = state;

	/** Actions */
	switch (state) {
	case DFW_STATE_MACHINE_INITIAL:
		dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_PATH_DISCOV);
		break;
	case DFW_STATE_MACHINE_POWER_UP:
		dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_PATH_MON);

		/* For the Nth instance (N>0) of the Path Origin State Machine, the Power Up
		 * Monitoring timer for the Path Monitoring state shall be started from the
		 * initial value set to a random value in the range (N‑1)×2000 to (N×2000‑1)
		 * in milliseconds.
		 */
		(void)bt_rand(&random, sizeof(random));
		random %= 2000;
		random += ARRAY_INDEX(state_machines, machine) * 2000;
		(void)k_work_reschedule(&machine->timer, K_MSEC(random));
		break;
	case DFW_STATE_MACHINE_PATH_DISCOV:
		sub = bt_mesh_subnet_get(machine->net_idx);
		if (!sub) {
			dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_FINAL);
			return;
		}

		if (machine->dependent.addr != BT_MESH_ADDR_UNASSIGNED) {
			dependent = &machine->dependent;
		}

		err = dfw_path_initialize_start(sub, dependent, machine->dst);
		if (err) {
			dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_FINAL);
			return;
		}

		break;
	case DFW_STATE_MACHINE_PATH_MON:
		machine->sent = false;

		if (old_state != DFW_STATE_MACHINE_POWER_UP) {
			/* No Path Validation state */
			(void)k_work_cancel_delayable(&machine->fw->echo_timer);
			(void)k_work_reschedule(&machine->timer,
					K_SECONDS(dfw_cfg.monitor_intv));
		}

		break;
	case DFW_STATE_MACHINE_PATH_IN_USE:
		if (old_state == DFW_STATE_MACHINE_PATH_VALID) {
			/* Have possiable use timer expire during path validation state ? */
			if (!k_work_delayable_is_pending(&machine->timer)) {
				k_work_reschedule(&machine->timer, K_NO_WAIT);
			}

			return;
		}

		discov_intv = discovery_timeout_get(dfw_cfg.discov_intv);
		path_intv = path_lifetime_get(dfw_table_get_by_forward(machine->fw)->lifetime);

		path_intv.ticks -= MIN(path_intv.ticks, K_SECONDS(dfw_cfg.monitor_intv).ticks);
		path_intv.ticks = path_intv.ticks > discov_intv.ticks << 1 ?
				  path_intv.ticks - discov_intv.ticks : discov_intv.ticks;

		(void)k_work_reschedule(&machine->timer, path_intv);
		break;
	case DFW_STATE_MACHINE_PATH_DISCOV_RETRY_WAIT:
		machine->sent = false;
		(void)k_work_reschedule(&machine->timer, K_SECONDS(dfw_cfg.discov_retry_intv));
		break;
	case DFW_STATE_MACHINE_FINAL:
		(void)memset(&machine->dependent, 0,
			     offsetof(struct dfw_state_machine, timer) -
			     offsetof(struct dfw_state_machine, dependent));

		(void)k_work_cancel_delayable(&machine->timer);
		break;
	default:
		break;
	}
}

static void dfw_state_machine_event(enum dfw_state_machine_events event,
				    struct bt_mesh_dfw_forwarding *fw,
				    uint16_t dst)
{
	struct dfw_state_machine *machine;

	static const struct {
		enum dfw_state_machine_states curr;
		enum dfw_state_machine_states next;
	} events[] = {
		[DFW_STATE_MACHINE_EVENT_PATH_DISCOV_SUCCEED] = {
			DFW_STATE_MACHINE_PATH_DISCOV,
			DFW_STATE_MACHINE_PATH_IN_USE,
		},
		[DFW_STATE_MACHINE_EVENT_PATH_DISCOV_FAILED]  = {
			DFW_STATE_MACHINE_PATH_DISCOV,
			DFW_STATE_MACHINE_PATH_DISCOV_RETRY_WAIT,
		},
		[DFW_STATE_MACHINE_EVENT_PATH_VALID_STARTED]  = {
			DFW_STATE_MACHINE_PATH_IN_USE,
			DFW_STATE_MACHINE_PATH_VALID,
		},
		[DFW_STATE_MACHINE_EVENT_PATH_VALID_SUCCEED]  = {
			DFW_STATE_MACHINE_PATH_VALID,
			DFW_STATE_MACHINE_PATH_IN_USE,
		},
		[DFW_STATE_MACHINE_EVENT_PATH_SOLICITED]      = {
			DFW_STATE_MACHINE_PATH_IN_USE,
			DFW_STATE_MACHINE_PATH_DISCOV,
		}
	};

	/* Maybe in any state, not only path in use state. */
	if (event == DFW_STATE_MACHINE_EVENT_PATH_REMOVED) {
		machine = dfw_state_machine_find_by_dst(dst);
		if (!machine) {
			LOG_WRN("State machine not found for dst 0x%04x", dst);
			return;
		}

		dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_FINAL);

		return;
	}

	machine = dfw_state_machine_find(events[event].curr, dst);
	if (!machine) {
		LOG_WRN("Ignore state machine event %d", event);
		return;
	}

	if (!machine->fw) {
		machine->fw = fw;
	}

	dfw_state_machine_state_set(machine, events[event].next);
}

static void dfw_state_machine_expire(struct k_work *work)
{
	struct dfw_state_machine *machine = CONTAINER_OF(work, struct dfw_state_machine,
							 timer.work);
	enum dfw_state_machine_states state;

	switch (machine->state) {
	case DFW_STATE_MACHINE_PATH_IN_USE:
		dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_PATH_MON);
		break;
	case DFW_STATE_MACHINE_PATH_MON:
		__fallthrough;
	case DFW_STATE_MACHINE_PATH_DISCOV_RETRY_WAIT:
		if (machine->sent) {
			state = DFW_STATE_MACHINE_PATH_DISCOV;
		} else {
			state = DFW_STATE_MACHINE_FINAL;
		}

		dfw_state_machine_state_set(machine, state);
		break;
	default:
		break;
	}
}

static void dfw_discovery_clear(struct bt_mesh_dfw_discovery *dv)
{
	LOG_INF("Discovery entry[0x%04x:0x%04x] cleared", dv->path_origin.addr, dv->destination);

	(void)k_work_cancel_delayable(&dv->lane_guard_timer);
	(void)k_work_cancel_delayable(&dv->request_delay_timer);
	(void)k_work_cancel_delayable(&dv->reply_delay_timer);
	(void)k_work_cancel_delayable(&dv->timer);

	(void)memset(dv, 0, offsetof(struct bt_mesh_dfw_discovery, lane_guard_timer));
}

static void dfw_dependent_node_setting_clear(struct bt_mesh_dfw_forwarding *fw,
					     struct bt_mesh_dfw_node *dependent)
{
	char path[40];
	int err;

	snprintk(path, sizeof(path), "bt/mesh/DFW/Sub/Dep/%x/%x/%x/%x",
		 dfw_table_get_by_forward(fw)->net_idx,
		 fw->path_origin.addr, fw->path_target.addr,
		 dependent->addr);

	err = settings_delete(path);
	if (err) {
		LOG_ERR("Failed to clear directed forwarding dependent value");
	} else {
		LOG_DBG("Cleared directed forwarding dependent value");
	}
}

static void dfw_fixed_path_setting_clear(struct bt_mesh_dfw_forwarding *fw)
{
	char path[36];
	int err;

	snprintk(path, sizeof(path), "bt/mesh/DFW/Sub/Fw/%x/%x/%x",
		 dfw_table_get_by_forward(fw)->net_idx,
		 fw->path_origin.addr, fw->path_target.addr);

	err = settings_delete(path);
	if (err) {
		LOG_ERR("Failed to clear directed forwarding value");
	} else {
		LOG_DBG("Cleared directed forwarding value");
	}

	for (int i = 0; i < BT_MESH_DFW_DEPENDENT_NODES_COUNT; i++) {
		if (fw->dependent_origin[i].addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		dfw_dependent_node_setting_clear(fw, &fw->dependent_origin[i]);
	}

	for (int i = 0; i < BT_MESH_DFW_DEPENDENT_NODES_COUNT; i++) {
		if (fw->dependent_target[i].addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		dfw_dependent_node_setting_clear(fw, &fw->dependent_target[i]);
	}
}

static void dfw_forwarding_clear(struct bt_mesh_dfw_forwarding *fw)
{
	LOG_DBG("Forwarding entry [0x%04x:0x%04x] cleared",
		fw->path_origin.addr, fw->path_target.addr);

	if (IS_ENABLED(CONFIG_BT_SETTINGS) && fw->fixed_path) {
		dfw_fixed_path_setting_clear(fw);
	}

	if (!fw->fixed_path && fw->path_origin.addr == bt_mesh_primary_addr()) {
		dfw_state_machine_event(DFW_STATE_MACHINE_EVENT_PATH_REMOVED, fw,
					fw->path_target.addr);
	}

	(void)k_work_cancel_delayable(&fw->echo_timer);
	(void)k_work_cancel_delayable(&fw->timer);

	(void)memset(fw, 0, offsetof(struct bt_mesh_dfw_forwarding, echo_timer));
}

static int dfw_feature_get(uint16_t net_idx, enum bt_mesh_dfw_flags flag,
			   enum bt_mesh_feat_state *state)
{
	struct bt_mesh_subnet *sub = bt_mesh_subnet_get(net_idx);

	if (!sub) {
		LOG_ERR("Unable get subnet for network index 0x%04x", net_idx);
		return -ENOENT;
	}

	*state = atomic_test_bit(sub->dfw->flags, flag) ?
			BT_MESH_FEATURE_ENABLED : BT_MESH_FEATURE_DISABLED;

	return 0;
}

static int dfw_feature_set_by_subnet(struct bt_mesh_subnet *sub,
				     enum bt_mesh_dfw_flags flag,
				     enum bt_mesh_feat_state state)
{
	if (state != BT_MESH_FEATURE_DISABLED && state != BT_MESH_FEATURE_ENABLED) {
		LOG_ERR("Invalid state value provided %d", state);
		return -EINVAL;
	}

	if (atomic_test_bit(sub->dfw->flags, flag) == (state == BT_MESH_FEATURE_ENABLED)) {
		return 0;
	}

	atomic_set_bit_to(sub->dfw->flags, flag, state);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_cfg_store(sub->dfw);
	}

	return 0;
}

static int dfw_feature_set(uint16_t net_idx, enum bt_mesh_dfw_flags flag,
			   enum bt_mesh_feat_state state)
{
	struct bt_mesh_subnet *sub = bt_mesh_subnet_get(net_idx);

	if (!sub) {
		LOG_ERR("Unable get subnet for network index 0x%04x", net_idx);
		return -ENOENT;
	}

	return dfw_feature_set_by_subnet(sub, flag, state);
}

uint8_t bt_mesh_dfw_net_transmit_get(void)
{
	return dfw_cfg.directed_net_transmit;
}

void bt_mesh_dfw_net_transmit_set(uint8_t xmit)
{
	if (dfw_cfg.directed_net_transmit == xmit) {
		return;
	}

	dfw_cfg.directed_net_transmit = xmit;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}
}

uint8_t bt_mesh_dfw_relay_retransmit_get(void)
{
	return dfw_cfg.directed_relay_retransmit;
}

void bt_mesh_dfw_relay_retransmit_set(uint8_t xmit)
{
	if (dfw_cfg.directed_relay_retransmit == xmit) {
		return;
	}

	dfw_cfg.directed_relay_retransmit = xmit;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}
}

uint8_t bt_mesh_dfw_ctl_net_transmit_get(void)
{
	return dfw_cfg.directed_ctl_net_transmit;
}

void bt_mesh_dfw_ctl_net_transmit_set(uint8_t xmit)
{
	if (dfw_cfg.directed_ctl_net_transmit == xmit) {
		return;
	}

	dfw_cfg.directed_ctl_net_transmit = xmit;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}
}

uint8_t bt_mesh_dfw_ctl_relay_retransmit_get(void)
{
	return dfw_cfg.directed_ctl_relay_retransmit;
}

void bt_mesh_dfw_ctl_relay_retransmit_set(uint8_t xmit)
{
	if (dfw_cfg.directed_ctl_relay_retransmit == xmit) {
		return;
	}

	dfw_cfg.directed_ctl_relay_retransmit = xmit;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}
}

int bt_mesh_dfw_get(uint16_t net_idx, enum bt_mesh_feat_state *state)
{
	return dfw_feature_get(net_idx, BT_MESH_DFW_ENABLED, state);
}

static void dfw_table_clear(struct bt_mesh_dfw_subnet *dfw, bool all)
{
	/* Clear all discovery of the subnet */
	for (int i = 0; i < ARRAY_SIZE(dfw->discovery); i++) {
		if (dfw->discovery[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		dfw_discovery_clear(&dfw->discovery[i]);
	}

	/* Clear all forwarding path of the subnet */
	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		/* except fixed path */
		if (!all && dfw->forwarding[i].fixed_path) {
			continue;
		}

		dfw->update_id++;

		dfw_forwarding_clear(&dfw->forwarding[i]);
	}
}

int bt_mesh_dfw_set(uint16_t net_idx, enum bt_mesh_feat_state state)
{
	struct bt_mesh_subnet *sub = bt_mesh_subnet_get(net_idx);

	if (!sub) {
		LOG_ERR("Unable get subnet for network index 0x%04x", net_idx);
		return -ENOENT;
	}

	if (state == BT_MESH_FEATURE_ENABLED) {
		return dfw_feature_set_by_subnet(sub, BT_MESH_DFW_ENABLED, state);
	}

	dfw_table_clear(sub->dfw, false);

	/* When the Directed Forwarding state is set to 0x00 for a subnet,
	 * then Directed Relay state shall be set to 0x00 for the subnet.
	 */
	(void)dfw_feature_set_by_subnet(sub, BT_MESH_DFW_RELAY_ENABLED, state);

	/* When the Directed Forwarding state is set to 0x00 for a subnet,
	 * and directed friend functionality is supported, then the Directed
	 * Friend state shall be set to 0x00 for that subnet.
	 */
	(void)bt_mesh_dfw_friend_set(net_idx, state);

	return dfw_feature_set_by_subnet(sub, BT_MESH_DFW_ENABLED, state);
}

int bt_mesh_dfw_relay_get(uint16_t net_idx, enum bt_mesh_feat_state *state)
{
	return dfw_feature_get(net_idx, BT_MESH_DFW_RELAY_ENABLED, state);
}

int bt_mesh_dfw_relay_set(uint16_t net_idx, enum bt_mesh_feat_state state)
{
	return dfw_feature_set(net_idx, BT_MESH_DFW_RELAY_ENABLED, state);
}

int bt_mesh_dfw_proxy_get(uint16_t net_idx, enum bt_mesh_feat_state *state)
{
	return BT_MESH_FEATURE_NOT_SUPPORTED;
}

int bt_mesh_dfw_friend_get(uint16_t net_idx, enum bt_mesh_feat_state *state)
{
	if (!IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND)) {
		*state = BT_MESH_FEATURE_NOT_SUPPORTED;
		return 0;
	}

	return dfw_feature_get(net_idx, BT_MESH_DFW_FRIEND_ENABLED, state);
}

int bt_mesh_dfw_friend_set(uint16_t net_idx, enum bt_mesh_feat_state state)
{
	struct bt_mesh_subnet *sub = bt_mesh_subnet_get(net_idx);

	if (!sub) {
		LOG_ERR("Unable get subnet for network index 0x%04x", net_idx);
		return -ENOENT;
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND)) {
		return -ENOTSUP;
	}

	/* When the Friend state is set to 0x00, and directed friend functionality
	 * is supported, then the Directed Friend state for all subnets shall be set
	 * to 0x00 and shall not be changed.
	 */
	if ((bt_mesh_friend_get() == BT_MESH_FEATURE_DISABLED) &&
	    (state == BT_MESH_FEATURE_ENABLED)) {
		return -EACCES;
	}

	if (state != BT_MESH_FEATURE_DISABLED) {
		return dfw_feature_set_by_subnet(sub, BT_MESH_DFW_FRIEND_ENABLED, state);
	}

	for (int i = 0; i < ARRAY_SIZE(bt_mesh.frnd); i++) {
		struct bt_mesh_friend *frnd = &bt_mesh.frnd[i];
		struct bt_mesh_dfw_node dependent;

		if (!frnd->subnet || !frnd->established ||
		    frnd->subnet->net_idx != net_idx) {
			continue;
		}

		dependent.addr = frnd->lpn;
		dependent.secondary_count = frnd->num_elem - 1;

		(void)bt_mesh_dfw_dependent_node_update_start(sub->net_idx, &dependent, false);
	}


	return dfw_feature_set_by_subnet(sub, BT_MESH_DFW_FRIEND_ENABLED, state);
}

static inline bool dfw_is_dependent_node_enable(uint16_t net_idx)
{
	enum bt_mesh_feat_state friend_state = BT_MESH_FEATURE_DISABLED;
	enum bt_mesh_feat_state proxy_state = BT_MESH_FEATURE_DISABLED;

	(void)bt_mesh_dfw_friend_get(net_idx, &friend_state);
	(void)bt_mesh_dfw_proxy_get(net_idx, &proxy_state);

	return (friend_state == BT_MESH_FEATURE_ENABLED ||
		proxy_state == BT_MESH_FEATURE_ENABLED);
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	struct bt_mesh_dfw_subnet *dfw = NULL;
	uint16_t net_idx;

	switch (evt) {
	case BT_MESH_KEY_ADDED:
		for (int i = 0; i < ARRAY_SIZE(dfw_subnets); i++) {
			net_idx = dfw_subnets[i].net_idx;
			if (net_idx == BT_MESH_KEY_UNUSED) {
				dfw = &dfw_subnets[i];
				continue;
			}

			if (net_idx == sub->net_idx) {
				sub->dfw = &dfw_subnets[i];
				sub->dfw->net_idx = sub->net_idx;
				return;
			}
		}

		/* There is a one-to-one correspondence between dfw and subnet, and do not expect
		 * that there will be no empty dfw for given subnet.
		 */
		if (!dfw) {
			__ASSERT_NO_MSG(false);
		}

		sub->dfw = dfw;
		sub->dfw->net_idx = sub->net_idx;

		/* When a node is added to a subnet, the forwarding number for
		 * that subnet shall be set to 255.
		 */
		dfw->forwarding_number = DFW_NUM_INITIAL_VAL;

		/* Use default value for new added subnet */
		atomic_set_bit_to(dfw->flags, BT_MESH_DFW_ENABLED,
				  IS_ENABLED(CONFIG_BT_MESH_DFW_ENABLED));
		atomic_set_bit_to(dfw->flags, BT_MESH_DFW_RELAY_ENABLED,
				  IS_ENABLED(CONFIG_BT_MESH_DFW_RELAY_ENABLED));
		atomic_set_bit_to(dfw->flags, BT_MESH_DFW_FRIEND_ENABLED,
				  IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND_ENABLED));

		dfw->update_id = 0;
		dfw->lifetime = PATH_LIFETIME;
		dfw->max_concurr_init = CONFIG_BT_MESH_DFW_DISCOVERY_MAX_CONCURR_INIT;
		dfw->wanted_lanes = CONFIG_BT_MESH_DFW_WANTED_LANES_COUNT;
		dfw->two_way_path = IS_ENABLED(CONFIG_BT_MESH_DFW_TWO_WAY_PATH);
		dfw->unicast_echo_intv = CONFIG_BT_MESH_DFW_UNICAST_ECHO_INTV;
		dfw->multicast_echo_intv = CONFIG_BT_MESH_DFW_MULTICAST_ECHO_INTV;
		break;
	case BT_MESH_KEY_DELETED:
		for (int i = 0; i < ARRAY_SIZE(state_machines); i++) {
			if (state_machines[i].net_idx != sub->net_idx) {
				continue;
			}

			dfw_state_machine_state_set(&state_machines[i], DFW_STATE_MACHINE_FINAL);
		}

		dfw_table_clear(sub->dfw, true);

		dfw_subnet_cfg_clear(sub->dfw);

		/* Remove All pending store flags */
		atomic_clear(sub->dfw->flags);

		sub->dfw->net_idx = BT_MESH_KEY_UNUSED;
		sub->dfw = NULL;
		break;
	default:
		break;
	}
}

BT_MESH_SUBNET_CB_DEFINE(dfw) = {
	.evt_handler = subnet_evt,
};

static bool dfw_forwarding_corresponding_to_path_existed(struct bt_mesh_dfw_forwarding *fw,
							 uint16_t src, uint16_t dst)
{
	int i;

	if (!ADDR_RANGE_IN(src, &fw->path_origin)) {
		for (i = 0; i < ARRAY_SIZE(fw->dependent_origin); i++) {
			if (ADDR_RANGE_IN(src, &fw->dependent_origin[i])) {
				break;
			}
		}

		if (i == ARRAY_SIZE(fw->dependent_origin)) {
			return false;
		}
	}

	if (ADDR_RANGE_IN(dst, &fw->path_target)) {
		return true;
	}

	for (i = 0; i < ARRAY_SIZE(fw->dependent_target); i++) {
		if (ADDR_RANGE_IN(dst, &fw->dependent_target[i])) {
			return true;
		}
	}

	return false;
}

static inline bool dfw_forwarding_dest_addr_is_valid(uint16_t dst)
{
	if (BT_MESH_ADDR_IS_RFU(dst) ||
	    (dst == BT_MESH_ADDR_UNASSIGNED) ||
	    (dst == BT_MESH_ADDR_ALL_NODES)  ||
	    (dst == BT_MESH_ADDR_RELAYS) ||
	    (dst == BT_MESH_ADDR_DFW_NODES))  {
		return false;
	}

	return true;
}

enum bt_mesh_net_if bt_mesh_dfw_path_existed(uint16_t net_idx, uint16_t src, uint16_t dst)
{
	struct bt_mesh_dfw_subnet *dfw;
	struct bt_mesh_subnet *sub;

	LOG_DBG("");

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		return BT_MESH_NET_IF_NONE;
	}

	dfw = sub->dfw;

	/* A path shall exist for a destination address that is the
	 * all-directed-forwarding-nodes fixed group address (see Section 3.4.2.4).
	 */
	if (dst == BT_MESH_ADDR_DFW_NODES) {
		return BT_MESH_NET_IF_ADV | BT_MESH_NET_IF_PROXY;
	}

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (dfw_forwarding_corresponding_to_path_existed(&dfw->forwarding[i], src, dst)) {
			return dfw->forwarding[i].bearer_toward_path_target;
		}

		if (!dfw->forwarding[i].backward_path_validated) {
			continue;
		}

		if (dfw_forwarding_corresponding_to_path_existed(&dfw->forwarding[i], dst, src)) {
			return dfw->forwarding[i].bearer_toward_path_origin;
		}
	}

	LOG_DBG("The path [0x%04x:0x%04x] not existed", src, dst);

	return BT_MESH_NET_IF_NONE;
}

static int dfw_send_path_request(struct bt_mesh_dfw_subnet *dfw, struct bt_mesh_dfw_discovery *dv)
{
	NET_BUF_SIMPLE_DEFINE(buf, 12);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = BT_MESH_ADDR_DFW_NODES,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = bt_mesh_subnet_get(dfw->net_idx),
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	bool dependent = dv->dependent_origin[0].addr != BT_MESH_ADDR_UNASSIGNED;

	/* On_Behalf_Of_Dependent_Origin | Path_Origin_Path_Metric_Type |
	 * Path_Origin_Path_Lifetime | Path_Discovery_Interval
	 */
	net_buf_simple_add_u8(&buf, ((uint8_t)dv->interval << PD_INT_BIT) |
				    ((uint8_t)dv->lifetime << PO_PLT_BIT) |
				    ((uint8_t)DFW_PATH_METRIC_NODE << PO_PMT_BIT) |
				    ((uint8_t)dependent << OBO_DO_BIT));

	/* Forwarding number generated by the Path Origin */
	net_buf_simple_add_u8(&buf, dv->forwarding_number);

	/* During the Directed Forwarding Discovery procedure (see Section 3.6.8.2.2),
	 * a Path_Origin_Path_Metric value is calculated based on the current
	 * Path_Origin_Path_Metric value stored in the Discovery Table.
	 */
	if (dv->path_origin.addr == bt_mesh_primary_addr()) {
		net_buf_simple_add_u8(&buf, 0x00);
	} else {
		struct bt_mesh_dfw_forwarding *fw =
				dfw_forwarding_find_by_dst(dfw, NULL, dv->path_origin.addr,
							   dv->destination, true, false);
		uint8_t metric = dv->metric + 1 + (fw ? fw->lane_count : 0);

		net_buf_simple_add_u8(&buf, metric);
	}

	/* Destination address of the path */
	net_buf_simple_add_be16(&buf, dv->destination);

	/* Path Origin unicast address range */
	if (dv->path_origin.secondary_count) {
		net_buf_simple_add_be16(&buf, dv->path_origin.addr | BIT(LEN_PST_BIT));
		net_buf_simple_add_u8(&buf, dv->path_origin.secondary_count + 1);
	} else {
		net_buf_simple_add_be16(&buf, dv->path_origin.addr);
	}

	/* Unicast address range of the dependent node of the Path Origin */
	if (dependent) {
		uint16_t range_present = dv->dependent_origin[0].secondary_count ?
					 BIT(LEN_PST_BIT) : 0;

		net_buf_simple_add_be16(&buf, dv->dependent_origin[0].addr | range_present);

		if (range_present) {
			net_buf_simple_add_u8(&buf, dv->dependent_origin[0].secondary_count + 1);
		}
	}

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_REQUEST, buf.data, buf.len, NULL, NULL);
}

static void dfw_lane_discov_guard_timer_expired(struct k_work *work)
{
	struct bt_mesh_dfw_discovery *dv = CONTAINER_OF(work, struct bt_mesh_dfw_discovery,
							lane_guard_timer.work);
	struct bt_mesh_dfw_subnet *dfw = dfw_table_get_by_discovery(dv);
	int err;

	LOG_DBG("lane discovery guard timer expired");

	(void)k_work_reschedule(&dv->timer, discovery_timeout_get(dv->interval));

	err = dfw_send_path_request(dfw, dv);
	if (err) {
		LOG_ERR("Unable to send path request (err:%d)", err);
	}
}

static void dfw_initialization_discovery_expired(struct k_work *work)
{
	struct bt_mesh_dfw_discovery *dv = CONTAINER_OF(work, struct bt_mesh_dfw_discovery,
							timer.work);
	struct bt_mesh_dfw_subnet *dfw = dfw_table_get_by_discovery(dv);
	struct bt_mesh_dfw_forwarding *fw = NULL;

	LOG_DBG("Discovery timer expired");

	fw = dfw_forwarding_find_by_dst(dfw, NULL, bt_mesh_primary_addr(),
					dv->destination, true, false);
	if (!fw) {
		LOG_WRN("Unable find forwarding table for dst 0x%04x", dv->destination);
		goto clear;
	}

	if (fw->path_not_ready) {
		fw->path_not_ready = false;
	}

	if (!atomic_test_and_clear_bit(dv->flags, BT_MESH_DFW_FLAG_PATH_REPLY_RECVED)) {
		goto clear;
	}

	if (fw->lane_count < dfw->wanted_lanes) {
		LOG_DBG("More lane needed (%d < %d)", fw->lane_count, dfw->wanted_lanes);

		k_work_reschedule(&dv->lane_guard_timer,
				  lane_discovery_guard_get(dfw_cfg.lane_discov_guard_intv));
		return;
	}

clear:
	if (fw && fw->lane_count) {
		LOG_INF("Path discovery succeeded");
		dfw_state_machine_event(DFW_STATE_MACHINE_EVENT_PATH_DISCOV_SUCCEED, fw,
					dv->destination);
	} else {
		LOG_INF("Path discovery failed");
		dfw_state_machine_event(DFW_STATE_MACHINE_EVENT_PATH_DISCOV_FAILED, NULL,
					dv->destination);
	}

	dfw_discovery_clear(dv);
}

static int dfw_path_initialize_start(struct bt_mesh_subnet *sub,
				     const struct bt_mesh_dfw_node *dependent,
				     uint16_t dst)
{
	struct bt_mesh_dfw_subnet *dfw = sub->dfw;
	struct bt_mesh_dfw_discovery *dv = NULL;
	size_t total_concurr_cnt = 0;

	if (dst == BT_MESH_ADDR_ALL_NODES ||
	    dst == BT_MESH_ADDR_RELAYS) {
		LOG_ERR("Invalid dst addr provided 0x%04x", dst);
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(dfw->discovery); i++) {
		if (dfw->discovery[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			dv = &dfw->discovery[i];
		} else if (dfw->discovery[i].path_origin.addr == bt_mesh_primary_addr()) {
			if (dfw->discovery[i].destination == dst) {
				LOG_ERR("Path initialize already started for 0x%04x", dst);
				return -EALREADY;
			}

			total_concurr_cnt++;
		}
	}

	/* If the number of executed Directed Forwarding Initialization procedures is equal
	 * to the Max Concurrent Init state value, then the Directed Forwarding Initialization
	 * procedure shall fail.
	 */
	if (!dv || total_concurr_cnt == dfw->max_concurr_init) {
		LOG_ERR("Insufficient discovery for path initialize procedure");
		return -EBUSY;
	}

	/* A new entry shall be added to the Discovery Table according to Section 3.6.8.6.1 */
	dv->path_origin.addr = bt_mesh_primary_addr();
	dv->path_origin.secondary_count = bt_mesh_elem_count() - 1;

	if (dependent) {
		dv->dependent_origin[0].addr = dependent->addr;
		dv->dependent_origin[0].secondary_count = dependent->secondary_count;
	}

	dv->destination = dst;

	dv->interval = dfw_cfg.discov_intv;
	dv->lifetime = dfw->lifetime;
	dv->forwarding_number = ++dfw->forwarding_number;
	dv->metric = 0;

	k_work_init_delayable(&dv->timer, dfw_initialization_discovery_expired);

	k_work_reschedule(&dv->timer, discovery_timeout_get(dv->interval));

	return dfw_send_path_request(dfw, dv);
}

static void dfw_discovery_expired(struct k_work *work)
{
	struct bt_mesh_dfw_discovery *dv = CONTAINER_OF(work, struct bt_mesh_dfw_discovery,
							timer.work);

	LOG_DBG("Path discovery timer expired");

	dfw_discovery_clear(dv);
}

static void dfw_path_lifetime_expired(struct k_work *work)
{
	struct bt_mesh_dfw_forwarding *fw = CONTAINER_OF(work, struct bt_mesh_dfw_forwarding,
							 timer.work);
	struct bt_mesh_dfw_subnet *dfw = dfw_table_get_by_forward(fw);

	LOG_INF("Path lifetime expired");

	dfw->update_id++;

	dfw_forwarding_clear(fw);
}

static void dfw_path_reply_delay_expired(struct k_work *work)
{
	struct bt_mesh_dfw_discovery *dv = CONTAINER_OF(work, struct bt_mesh_dfw_discovery,
							reply_delay_timer.work);
	struct bt_mesh_dfw_subnet *dfw = dfw_table_get_by_discovery(dv);
	struct bt_mesh_dfw_forwarding *fw, *fw_new = NULL;
	bool is_unicast, is_local, confirm_req = false;
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = bt_mesh_subnet_get(dfw->net_idx),
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	NET_BUF_SIMPLE_DEFINE(buf, 12);
	int err;

	LOG_INF("Start path reply for 0x%04x", dv->path_origin.addr);

	/* If the Forwarding Table contains other non-fixed path entries with the same Path_Origin
	 * and Destination values as in the added Forwarding Table entry, these other entries
	 * shall be removed.
	 */
	fw = dfw_forwarding_find_by_dst(dfw, NULL, dv->path_origin.addr, dv->destination,
					false, false);
	if (fw && fw->forwarding_number != dv->forwarding_number) {
		dfw->update_id++;
		LOG_WRN("Removed duplicate forwarding entry");
		dfw_forwarding_clear(fw);
	}

	is_unicast = BT_MESH_ADDR_IS_UNICAST(dv->destination);
	is_local = bt_mesh_has_addr(dv->destination);

	fw = dfw_forwarding_find_by_forwarding_number(dfw, &fw_new, dv->path_origin.addr,
						      dv->forwarding_number, false);
	if (fw) {
		dfw->update_id++;
		fw->bearer_toward_path_origin |= dv->bearer_toward_path_origin;
		fw->lane_count++;
		goto reply;
	} else if (!fw_new) {
		LOG_WRN("Insuffcient Fowarding Table entry");
		return;
	}

	/* Set to the Destination value of the Discovery Table entry if it is a group address or
	 * a virtual address; otherwise, set to the primary element address of the Path Target.
	 */
	if (!is_local) {
		/* Initialized with the primary element address of a dependent node if the
		 * Destination value of the Discovery Table entry is an element address of that
		 * dependent node; otherwise, the Dependent_Target_List is empty.
		 */
		if (is_unicast) {
			if (IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND) &&
			    bt_mesh_friend_match(dfw->net_idx, dv->destination)) {
				struct bt_mesh_friend *frnd;

				frnd = bt_mesh_friend_find(dfw->net_idx, dv->destination,
							   true, true);
				if (!frnd) {
					LOG_WRN("Not found lpn for dependent addr %x",
						dv->destination);
					return;
				}

				fw_new->dependent_target[0].addr = frnd->lpn;
				fw_new->dependent_target[0].secondary_count = frnd->num_elem - 1;

				fw_new->path_target.addr = bt_mesh_primary_addr();
				fw_new->path_target.secondary_count = bt_mesh_elem_count() - 1;
			}
		} else {
			fw_new->path_target.addr = dv->destination;
			fw_new->path_target.secondary_count = 0;
		}
	} else {
		/* Set to the Destination value of the Discovery Table entry if it is a group
		 * address or a virtual address; otherwise, set to the primary element address
		 * of the Path Target.
		 */
		if (is_unicast) {
			fw_new->path_target.addr = bt_mesh_primary_addr();
			fw_new->path_target.secondary_count = bt_mesh_elem_count() - 1;
		} else {
			fw_new->path_target.addr = dv->destination;
			fw_new->path_target.secondary_count = 0;
		}
	}

	fw_new->backward_path_validated = false;
	fw_new->path_not_ready = false;
	fw_new->fixed_path = false;

	fw_new->path_origin.addr = dv->path_origin.addr;
	fw_new->path_origin.secondary_count = dv->path_origin.secondary_count;

	fw_new->dependent_origin[0].addr = dv->dependent_origin[0].addr;
	fw_new->dependent_origin[0].secondary_count = dv->dependent_origin[0].secondary_count;

	fw_new->forwarding_number = dv->forwarding_number;
	fw_new->bearer_toward_path_origin = dv->bearer_toward_path_origin;

	fw_new->bearer_toward_path_target = 0;
	fw_new->lane_count = 1;

	fw = fw_new;

	dfw->update_id++;

	/* Start path lifetime */
	(void)k_work_reschedule(&fw_new->timer, path_lifetime_get(dv->lifetime));

reply:
	/* If the Destination value in the Discovery Table entry is a unicast address, and a
	 * Forwarding Table entry corresponding to a path from the primary element address of
	 * the node to the Path_Origin value in the Discovery Table entry does not exist
	 * (see Section 3.6.8.5.1), the Confirmation_Request field shall be set to the Two Way
	 * Path state value (see Section 4.2.31); otherwise, it shall be set to 0.
	 */
	if (is_unicast) {
		bool path_not_exist = bt_mesh_dfw_path_existed(
						dfw->net_idx,
						dv->destination,
						dv->path_origin.addr) == BT_MESH_NET_IF_NONE;
		if (path_not_exist) {
			confirm_req = dfw->two_way_path;
		}
	}

	/* If the Destination value in the Discovery Table entry is a unicast address, and the
	 * PATH_REPLY message is originated on behalf of a dependent node of the Path Target,
	 * the On_Behalf_Of_Dependent_Target field shall be set to 1, and the
	 * Dependent_Target_Unicast_Addr_Range field shall be set to the unicast address range
	 * of the dependent node. Otherwise, the On_Behalf_Of_Dependent_Target field shall be
	 * set to 0, and the Dependent_Target_Unicast_Addr_Range field shall not be present.
	 */
	net_buf_simple_add_u8(&buf, (((uint8_t)is_unicast << UST_DST_BIT) |
				     ((uint8_t)(!is_local && is_unicast) << OBO_DT_BIT) |
				     ((uint8_t)confirm_req << CFM_REQ_BIT)));

	net_buf_simple_add_be16(&buf, dv->path_origin.addr);
	net_buf_simple_add_u8(&buf, dv->forwarding_number);

	if (is_unicast) {
		if (fw->path_target.secondary_count) {
			net_buf_simple_add_be16(&buf, fw->path_target.addr | BIT(LEN_PST_BIT));
			net_buf_simple_add_u8(&buf, fw->path_target.secondary_count + 1);
		} else {
			net_buf_simple_add_be16(&buf, fw->path_target.addr);
		}

		/* Unicast address range of the dependent node of the Path Target */
		if (!is_local) {
			if (fw->dependent_target[0].secondary_count) {
				net_buf_simple_add_be16(&buf,
					fw->dependent_target[0].addr | BIT(LEN_PST_BIT));
				net_buf_simple_add_u8(&buf,
					fw->dependent_target[0].secondary_count + 1);
			} else {
				net_buf_simple_add_be16(&buf, fw->dependent_target[0].addr);
			}
		}
	}

	/* The DST field shall be set to the Next_Toward_Path_Origin value in the Discovery
	 * Table entry.
	 */
	ctx.addr = dv->next_toward_path_origin;

	err = bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_REPLY, buf.data, buf.len, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send path reply (err:%d)", err);
	}
}

static void dfw_path_request_delay_expired(struct k_work *work)
{
	struct bt_mesh_dfw_discovery *dv = CONTAINER_OF(work, struct bt_mesh_dfw_discovery,
							request_delay_timer.work);
	struct bt_mesh_dfw_subnet *dfw = dfw_table_get_by_discovery(dv);
	int err;

	LOG_DBG("Path request delay expired");

	err = dfw_send_path_request(dfw, dv);
	if (err) {
		LOG_ERR("Unable to send path request (err:%d)", err);
	}
}

static bool dfw_rx_is_target(struct bt_mesh_net_rx *rx, uint16_t dst)
{
	if (bt_mesh_has_addr(dst)) {
		return true;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND)) {
		enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;

		(void)bt_mesh_dfw_friend_get(rx->ctx.net_idx, &state);
		if (state == BT_MESH_FEATURE_ENABLED &&
		    bt_mesh_friend_match(rx->ctx.net_idx, dst)) {
			return true;
		}
	}

	return false;
}

int bt_mesh_dfw_path_request(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	const size_t min_len = offsetof(struct bt_mesh_ctl_path_request, path_origin_addr_range);
	struct bt_mesh_ctl_path_request *req = (void *)buf->data;
	enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;
	struct bt_mesh_dfw_node path_origin, dependent_origin;
	struct bt_mesh_dfw_discovery *dv, *dv_new = NULL;
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	struct bt_mesh_dfw_forwarding *fw = NULL;
	bool is_target = false;
	uint16_t destination;
	uint32_t random;
	uint8_t range;

	if (buf->len < min_len) {
		LOG_ERR("Path Request message size less than minimum required(%d < %d)",
			buf->len, min_len);
		return -EINVAL;
	}

	/* Prohibit */
	if (req->octer & BIT(0)) {
		LOG_ERR("Path Request message prohibit value not zero");
		return -EINVAL;
	}

	if (rx->ctx.recv_dst != BT_MESH_ADDR_DFW_NODES || rx->ctx.recv_ttl != 0) {
		return -EINVAL;
	}

	/* The RSSI value measured for the PATH_REQUEST message is less than the sum of the
	 * Default RSSI Threshold state value (see Section 4.2.35.1) and the RSSI Margin state
	 * value (see Section 4.2.35.2).
	 */
	if (rx->ctx.recv_rssi < CONFIG_BT_MESH_DFW_DEFAULT_RSSI_THRESHOLD +
				dfw_cfg.rssi_margin) {
		LOG_WRN("Path Request message rssi less than minimum required(%d < %d)",
			rx->ctx.recv_rssi,
			CONFIG_BT_MESH_DFW_DEFAULT_RSSI_THRESHOLD + dfw_cfg.rssi_margin);

		return -EINVAL;
	}

	if (PO_PMT(req) != DFW_PATH_METRIC_NODE) {
		LOG_ERR("Path Matric type not support %ld", PO_PMT(req));
		return -ENOTSUP;
	}

	destination = sys_get_be16((void *)&req->dest);
	if (!dfw_forwarding_dest_addr_is_valid(destination)) {
		LOG_ERR("Path destination address not valid 0x%04x", destination);
		return -EINVAL;
	}

	path_origin.addr = sys_get_be16((void *)&req->path_origin);
	if (LEN_PST(path_origin.addr)) {
		if (buf->len < min_len + 1) {
			return -EINVAL;
		}

		path_origin.addr ^= BIT(LEN_PST_BIT);

		if (req->path_origin_addr_range < 2) {
			return -EINVAL;
		}

		path_origin.secondary_count = req->path_origin_addr_range - 1;
		(void)net_buf_simple_pull(buf, min_len + 1);
	} else {
		path_origin.secondary_count = 0;
		(void)net_buf_simple_pull(buf, min_len);
	}

	if (!BT_MESH_ADDR_IS_UNICAST(path_origin.addr) ||
	    !BT_MESH_ADDR_IS_UNICAST(path_origin.addr + path_origin.secondary_count)) {
		LOG_ERR("Invalid path origin address 0x%04x", path_origin.addr);
		return -EINVAL;
	}

	if (OBO_DO(req)) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		dependent_origin.addr = net_buf_simple_pull_be16(buf);
		if (LEN_PST(dependent_origin.addr)) {
			dependent_origin.addr ^= BIT(LEN_PST_BIT);

			if (buf->len < 1) {
				return -EINVAL;
			}

			range = net_buf_simple_pull_u8(buf);
			if (range < 2) {
				return -EINVAL;
			}

			dependent_origin.secondary_count = range - 1;
		} else {
			dependent_origin.secondary_count = 0;
		}

		if (!BT_MESH_ADDR_IS_UNICAST(dependent_origin.addr) ||
		    !BT_MESH_ADDR_IS_UNICAST(dependent_origin.addr +
					     dependent_origin.secondary_count)) {
			LOG_ERR("Invalid path dependent origin address 0x%04x",
				dependent_origin.addr);

			return -EINVAL;
		}

		if (ADDR_RANGE_IN(dependent_origin.addr, &path_origin)) {
			return -EINVAL;
		}
	} else {
		dependent_origin.addr = BT_MESH_ADDR_UNASSIGNED;
		dependent_origin.secondary_count = 0;
	}

	/* A non-fixed Forwarding Table entry corresponding to the PATH_REQUEST message exists
	 * (see Section 3.6.8.5.2), and the Path_Origin_Forwarding_Number field value in the
	 * message is less than the Forwarding_Number value in the table entry.
	 */
	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin.addr, destination, true, false);
	if (fw && DFW_NUM_A_LESS_B(req->path_origin_forwarding_number, fw->forwarding_number)) {
		LOG_WRN("Ignore path request forwarding number %d less than %d",
			req->path_origin_forwarding_number, fw->forwarding_number);
		return -EINVAL;
	}

	(void)bt_mesh_dfw_relay_get(rx->ctx.net_idx, &state);

	/* Directed relay, directed friend, and directed proxy functionalities are all disabled,
	 * and the Destination field value of the PATH_REQUEST message is not an element address
	 * of the node or a group address or a virtual address that the node is subscribed to.
	 */

	if (state != BT_MESH_FEATURE_ENABLED &&
	    !dfw_is_dependent_node_enable(rx->ctx.net_idx) &&
	    !bt_mesh_has_addr(destination)) {
		LOG_WRN("Ignore path request 0x%04x, not for us", destination);
		return -ENOTSUP;
	}

	/* Directed relay functionality is disabled; and either directed friend functionality,
	 * directed proxy functionality, or both are enabled; and the Destination field value
	 * of the PATH_REQUEST message is not an element address of the node or of a dependent
	 * node; and the Destination field value of the PATH_REQUEST message is not a group
	 * address or a virtual address that the node or a dependent node is subscribed to.
	 */

	is_target = dfw_rx_is_target(rx, destination);

	if (state != BT_MESH_FEATURE_ENABLED && !is_target) {
		LOG_WRN("Directed friend not enabled");
		return -ENODEV;
	}

	/* A Discovery Table entry corresponds to the PATH_REQUEST message
	 * if both of the following conditions are met:
	 *
	 * • The Path_Origin field value in the message is equal to the Path_Origin
	 *   value of the table entry.
	 *
	 * • The Path_Origin_Forwarding_Number field value in the message is equal to
	 *   the Path_Origin_Forwarding_Number value of the table entry.
	 */
	dv = dfw_discovery_find_by_forwarding_number(dfw, &dv_new, path_origin.addr,
						     req->path_origin_forwarding_number);
	if (!dv && !dv_new) {
		LOG_WRN("No such discovery table entry for 0x%04x", path_origin.addr);
		return -EBUSY;
	}

	/* A Discovery Table entry corresponding to the PATH_REQUEST message exists, and the
	 * Path_Origin_Path_Metric field value in the message is not less than the
	 * Path_Origin_Path_Metric value in the table entry.
	 *
	 * The Discovery Table entry shall be updated according to Section 3.6.8.6.2.
	 */
	if (dv) {
		if (dv->metric <= req->path_origin_path_metric) {
			LOG_WRN("Path Request Matric %d not less than %d",
				req->path_origin_path_metric, dv->metric);
			return -EINVAL;
		}

		/* Set to the Path_Origin_Path_Metric field value of the message. */
		dv->metric = req->path_origin_path_metric;

		/* The bit representing the bearer from which the Network PDU of the PATH_REQUEST
		 * message was received shall be set to 1 (see Section 4.3.1.4). All other bits
		 * shall be set to 0.
		 */
		dv->bearer_toward_path_origin = rx->net_if;

		goto resend;
	} else if (!dv_new) {
		return -ENOMEM;
	}

	dv_new->path_origin.addr = path_origin.addr;
	dv_new->path_origin.secondary_count = path_origin.secondary_count;

	if (dependent_origin.addr != BT_MESH_ADDR_UNASSIGNED) {
		dv_new->dependent_origin[0].addr = dependent_origin.addr;
		dv_new->dependent_origin[0].secondary_count = dependent_origin.secondary_count;
	}

	dv_new->forwarding_number = req->path_origin_forwarding_number;
	dv_new->destination = destination;
	dv_new->metric = req->path_origin_path_metric;
	dv_new->lifetime = PO_PLT(req);
	dv_new->interval = PD_INT(req);

	dv_new->next_toward_path_origin = rx->ctx.addr;
	dv_new->bearer_toward_path_origin = rx->net_if;

	k_work_init_delayable(&dv_new->timer, dfw_discovery_expired);

	(void)k_work_reschedule(&dv_new->timer, discovery_timeout_get(dv_new->interval));

	dv = dv_new;

	if (!is_target) {
		LOG_DBG("Path Request destination address not for us, resend it");
		goto resend;
	}

	LOG_INF("Path request destination address for us, reply it");

	/* If the Destination field value in the PATH_REQUEST message is a group address
	 * or a virtual address, the Path Reply Delay timer initial value shall be set to
	 * the sum of the Path_Reply_Delay value and a random delay of 0 milliseconds to
	 * 500 milliseconds.
	 */
	if (BT_MESH_ADDR_IS_GROUP(destination) || BT_MESH_ADDR_IS_VIRTUAL(destination)) {
		(void)bt_rand(&random, sizeof(random));
		random %= PATH_REPLY_DELAY_MS;
	} else {
		random = 0;
	}

	(void)k_work_reschedule(&dv->reply_delay_timer,
				K_MSEC(PATH_REPLY_DELAY_MS + random));

resend:
	/* The node processing the PATH_REQUEST message shall start a Path Request Delay timer for
	 * the corresponding Discovery Table entry if all the following conditions are met:
	 *
	 * The Path Request Delay timer for the corresponding Discovery Table entry is inactive.
	 *
	 * The node is a Directed Relay node.
	 *
	 * The node is not a Path Target for the PATH_REQUEST message. OR
	 * The node is a Path Target for the PATH_REQUEST message, and the Destination field value
	 * in the PATH_REQUEST message is a group address or a virtual address.
	 */
	if (!k_work_delayable_is_pending(&dv->request_delay_timer) &&
	    state && (!is_target || BT_MESH_ADDR_IS_GROUP(destination) ||
				    BT_MESH_ADDR_IS_VIRTUAL(destination))) {

		(void)bt_rand(&random, sizeof(random));
		random %= 30;

		(void)k_work_reschedule(&dv->request_delay_timer,
					K_MSEC(PATH_REQUEST_DELAY_MS + random));
	}

	return 0;
}

static int dfw_send_path_confirm(struct bt_mesh_subnet *sub, struct bt_mesh_dfw_forwarding *fw)
{
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = BT_MESH_ADDR_DFW_NODES,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = sub,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	NET_BUF_SIMPLE_DEFINE(buf, 4);

	LOG_DBG("Send Path Confirm");

	net_buf_simple_add_be16(&buf, fw->path_origin.addr);
	net_buf_simple_add_be16(&buf, fw->path_target.addr);

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_CONFIRM, buf.data, buf.len,
				NULL, NULL);
}

static void dfw_path_echo_expired(struct k_work *work)
{
	struct bt_mesh_dfw_forwarding *fw = CONTAINER_OF(work, struct bt_mesh_dfw_forwarding,
							 echo_timer.work);
	struct bt_mesh_dfw_subnet *dfw = dfw_table_get_by_forward(fw);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = fw->path_target.addr,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0x7f,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = bt_mesh_subnet_get(dfw->net_idx),
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	int err;

	LOG_DBG("Path Echo expired");

	if (atomic_test_and_set_bit(fw->flags, BT_MESH_DFW_FORWARDING_FLAG_ECHO_REPLY)) {
		LOG_ERR("Not received Echo Reply");
		dfw->update_id++;
		dfw_forwarding_clear(fw);

		(void)bt_mesh_dfw_path_origin_state_machine_start(dfw->net_idx, NULL,
								  ctx.addr, false);
		return;
	}

	LOG_DBG("Echo Reply Received");

	dfw_state_machine_event(DFW_STATE_MACHINE_EVENT_PATH_VALID_STARTED, fw,
				fw->path_target.addr);

	(void)k_work_reschedule(&fw->echo_timer, discovery_timeout_get(dfw_cfg.discov_intv));

	err = bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_ECHO_REQ, NULL, 0, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send path echo request (err:%d)", err);
	}
}

int bt_mesh_dfw_path_reply(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	const size_t min_len = offsetof(struct bt_mesh_ctl_path_reply, path_target);
	struct bt_mesh_dfw_node path_target = { 0 }, dependent_target = { 0 };
	struct bt_mesh_ctl_path_reply *reply = (void *)buf->data;
	struct bt_mesh_dfw_forwarding *fw, *fw_new = NULL;
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	struct bt_mesh_dfw_discovery *dv = NULL;
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = rx->sub,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	uint16_t path_origin, buf_len = buf->len;
	uint8_t percent, range;
	bool is_local;
	int err;

	if (buf->len < min_len) {
		return -EINVAL;
	}

	if (rx->ctx.recv_dst != bt_mesh_primary_addr() ||
	    rx->ctx.recv_ttl != 0) {
		return -EINVAL;
	}

	/* Prohibit */
	if (reply->octer & BIT_MASK(5)) {
		LOG_ERR("Path Reply message prohibit value not zero");
		return -EINVAL;
	}

	path_origin = sys_get_be16((void *)&reply->path_origin);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
		LOG_ERR("Invalid path origin address 0x%04x", path_origin);
		return -EINVAL;
	}

	if (UST_DST(reply)) {
		path_target.addr = sys_get_be16((void *)&reply->path_target);
		if (LEN_PST(path_target.addr)) {
			if (buf->len < min_len + 1) {
				return -EINVAL;
			}

			if (reply->path_target_range < 2) {
				return -EINVAL;
			}

			path_target.secondary_count = reply->path_target_range - 1;
			path_target.addr ^= BIT(LEN_PST_BIT);
			(void)net_buf_simple_pull(buf, min_len + 1);
		} else {
			path_target.secondary_count = 0;
			(void)net_buf_simple_pull(buf, min_len);
		}

		if (!BT_MESH_ADDR_IS_UNICAST(path_target.addr)) {
			LOG_ERR("Invalid path target address 0x%04x", path_target.addr);
			return -EINVAL;
		}

		if (OBO_DT(reply)) {
			if (buf->len < 2) {
				return -EINVAL;
			}

			dependent_target.addr = net_buf_simple_pull_be16(buf);
			if (LEN_PST(dependent_target.addr)) {
				if (buf->len < 1) {
					return -EINVAL;
				}

				range = net_buf_simple_pull_u8(buf);
				if (range < 2) {
					return -EINVAL;
				}

				dependent_target.secondary_count = range - 1;
				dependent_target.addr ^= BIT(LEN_PST_BIT);
			} else {
				dependent_target.secondary_count = 0;
			}

			if (!BT_MESH_ADDR_IS_UNICAST(dependent_target.addr)) {
				LOG_ERR("Invalid dependent target address 0x%04x",
					dependent_target.addr);
				return -EINVAL;
			}
		}
	}

	/* A Discovery Table entry corresponds to a PATH_REPLY message when all the following
	 * conditions are met:
	 */
	for (int i = 0; i < ARRAY_SIZE(dfw->discovery); i++) {
		/* The Path_Origin field value in the message is equal to the Path_Origin
		 * value of the table entry.
		 */
		if (dfw->discovery[i].path_origin.addr != path_origin) {
			continue;
		}

		/* The Path_Origin_Forwarding_Number field value in the message is equal to the
		 * Path_Origin_Forwarding_Number value of the table entry.
		 */
		if (reply->path_origin_forwarding_number != dfw->discovery[i].forwarding_number) {
			continue;
		}

		/*
		 * The Destination value of the table entry is a group address, a virtual address
		 */
		if (BT_MESH_ADDR_IS_GROUP(dfw->discovery[i].destination) ||
		    BT_MESH_ADDR_IS_VIRTUAL(dfw->discovery[i].destination)) {
			dv = &dfw->discovery[i];
			break;
		} else if (UST_DST(reply)) {
			/* A unicast address in the range [PathTarget, PathTarget + PathTarget
			 * Secondary ElementsCount] (i.e., an element address of the Path Target
			 * node).
			 */
			if (ADDR_RANGE_IN(dfw->discovery[i].destination, &path_target)) {
				dv = &dfw->discovery[i];
				break;
			}

			if (!OBO_DT(reply)) {
				continue;
			}

			/* A unicast address in the range [DependentTarget, DependentTarget +
			 * Dependent TargetSecondaryElementsCount] (i.e., an element address
			 * of the dependent node of the Path Target node).
			 */
			if (ADDR_RANGE_IN(dfw->discovery[i].destination, &dependent_target)) {
				dv = &dfw->discovery[i];
				break;
			}
		}
	}

	if (!dv) {
		LOG_WRN("No such discovery entry");
		return -ENOENT;
	}

	is_local = (path_origin == bt_mesh_primary_addr());

	/* If the Forwarding Table contains other non-fixed path entries with the same Path_Origin
	 * and Destination values as in the added Forwarding Table entry, these other entries
	 * shall be removed.
	 */
	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin, dv->destination, false, false);
	if (fw && fw->forwarding_number != reply->path_origin_forwarding_number) {
		dfw->update_id++;
		dfw_forwarding_clear(fw);
	}

	fw = dfw_forwarding_find_by_forwarding_number(dfw, &fw_new, path_origin,
						      reply->path_origin_forwarding_number,
						      false);
	if (fw) {
		dfw->update_id++;

		if (!is_local) {
			fw->bearer_toward_path_origin |= dv->bearer_toward_path_origin;
		}

		fw->bearer_toward_path_target |= rx->net_if;

		/* Shall be incremented by 1 if this is the first PATH_REPLY received after the
		 * Path Discovery timer of the corresponding Discovery Table entry started;
		 */
		if (!atomic_test_and_set_bit(dv->flags, BT_MESH_DFW_FLAG_PATH_REPLY_RECVED)) {
			fw->lane_count++;
			goto first_recv;
		} else {
			return 0;
		}
	} else if (!fw_new) {
		return -ENOBUFS;
	}

	/* If the Forwarding Table does not already contain an entry that corresponds to the
	 * PATH_REPLY message, the Forwarding Table Update Identifier shall change
	 * (see Section 4.2.29.1), and a new entry shall be added to the Forwarding Table
	 * state, based on values in the Discovery Table entry that corresponds to the
	 * PATH_REPLY message (see Section 3.6.8.6.3).
	 */
	dfw->update_id++;

	fw_new->fixed_path = false;
	fw_new->backward_path_validated = false;

	if (is_local && !UST_DST(reply)) {
		fw_new->path_not_ready = true;
	}

	fw_new->path_origin.addr = dv->path_origin.addr;
	fw_new->path_origin.secondary_count = dv->path_origin.secondary_count;

	fw_new->forwarding_number = dv->forwarding_number;

	if (is_local) {
		fw_new->bearer_toward_path_origin = 0;
	} else {
		fw_new->bearer_toward_path_origin = dv->bearer_toward_path_origin;
	}

	fw_new->dependent_origin[0].addr = dv->dependent_origin[0].addr;
	fw_new->dependent_origin[0].secondary_count = dv->dependent_origin[0].secondary_count;

	if (BT_MESH_ADDR_IS_GROUP(dv->destination) || BT_MESH_ADDR_IS_VIRTUAL(dv->destination)) {
		fw_new->path_target.addr = dv->destination;
		fw_new->path_target.secondary_count = 0;
	} else {
		fw_new->path_target.addr = path_target.addr;
		fw_new->path_target.secondary_count = path_target.secondary_count;
	}

	fw_new->dependent_target[0].addr = dependent_target.addr;
	fw_new->dependent_target[0].secondary_count = dependent_target.secondary_count;

	fw_new->bearer_toward_path_target = rx->net_if;

	fw_new->lane_count = 1;

	atomic_set_bit(dv->flags, BT_MESH_DFW_FLAG_PATH_REPLY_RECVED);

first_recv:
	if (!is_local) {
		ctx.addr = dv->next_toward_path_origin;
		return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_REPLY, (void *)reply, buf_len,
					NULL, NULL);
	}

	LOG_INF("Lane established");

	if (CFM_REQ(reply)) {
		/* Execute a Directed Forwarding Confirmation procedure */
		fw_new->backward_path_validated = true;

		err = dfw_send_path_confirm(rx->sub, fw_new);
		if (err) {
			LOG_ERR("Unable to send path confirmation");
		}
	}

	if (UST_DST(reply)) {
		if (!dfw->unicast_echo_intv) {
			return 0;
		}

		percent = dfw->unicast_echo_intv;
	} else {
		if (!dfw->multicast_echo_intv) {
			return 0;
		}

		percent = dfw->multicast_echo_intv;
	}

	fw_new->echo_intv = path_lifetime_get(dv->lifetime);

	fw_new->echo_intv.ticks *= percent;
	fw_new->echo_intv.ticks /= 100;

	(void)k_work_reschedule(&fw_new->echo_timer, fw_new->echo_intv);

	return 0;
}

int bt_mesh_dfw_path_confirm(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_path_comfirm *confirm = (void *)buf->data;
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	uint16_t path_origin, path_target;
	struct bt_mesh_dfw_forwarding *fw;
	struct bt_mesh_dfw_discovery *dv;
	bool is_local;

	if (buf->len != sizeof(struct bt_mesh_ctl_path_comfirm)) {
		return -EINVAL;
	}

	if (rx->ctx.recv_dst != BT_MESH_ADDR_DFW_NODES ||
	    rx->ctx.recv_ttl != 0) {
		return -EINVAL;
	}

	path_origin = sys_get_be16((void *)&confirm->path_origin);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
		LOG_ERR("Invalid path origin address 0x%04x", path_origin);
		return -EINVAL;
	}

	path_target = sys_get_be16((void *)&confirm->path_target);

	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin, path_target, false, false);
	if (!fw) {
		return -ENOENT;
	}

	dv = dfw_discovery_find_by_forwarding_number(dfw, NULL, path_origin,
						     fw->forwarding_number);
	if (!dv) {
		return -ENOENT;
	}

	is_local = (path_target == bt_mesh_primary_addr());

	if (fw->backward_path_validated) {
		if (is_local) {
			return 0;
		}

		if (!atomic_test_bit(dv->flags, BT_MESH_DFW_FLAG_PATH_CONFIRM_SENT)) {
			goto send_status;
		}

		return 0;
	}

	dfw->update_id++;
	fw->backward_path_validated = true;

	if (is_local) {
		return 0;
	}

send_status:
	atomic_set_bit(dv->flags, BT_MESH_DFW_FLAG_PATH_CONFIRM_SENT);

	return dfw_send_path_confirm(rx->sub, fw);
}

int bt_mesh_dfw_path_echo_request(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr = rx->ctx.addr,
		.send_ttl = 0x7f,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = rx->sub,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	struct bt_mesh_dfw_forwarding *fw;
	uint16_t dst;

	if (!rx->ctx.recv_ttl) {
		return -EINVAL;
	}

	fw = dfw_forwarding_find_by_dst(dfw, NULL, rx->ctx.addr, rx->ctx.recv_dst,
					false, false);
	if (!fw) {
		return -ENOENT;
	}

	if (fw->backward_path_validated) {
		ctx.cred = BT_MESH_CRED_DIRECTED;
	}

	sys_put_be16(fw->path_target.addr, (void *)&dst);

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_ECHO_REPLY, (void *)&dst, 2, NULL, NULL);
}

int bt_mesh_dfw_path_echo_reply(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	struct bt_mesh_dfw_forwarding *fw;
	uint16_t dst;

	if (buf->len != sizeof(dst)) {
		return -EINVAL;
	}

	dst = sys_get_be16(buf->data);

	fw = dfw_forwarding_find_by_dst(dfw, NULL, rx->ctx.recv_dst, dst,
					false, false);
	if (!fw) {
		return -ENOENT;
	}

	if (!fw->backward_path_validated) {
		if (rx->ctx.cred != BT_MESH_CRED_FLOODING) {
			return -EINVAL;
		}
	} else if (rx->ctx.cred != BT_MESH_CRED_DIRECTED) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(fw->flags, BT_MESH_DFW_FORWARDING_FLAG_ECHO_REPLY)) {
		return 0;
	}

	if (K_TIMEOUT_EQ(fw->echo_intv, K_NO_WAIT)) {
		return k_work_cancel_delayable(&fw->echo_timer);
	} else {
		return k_work_reschedule(&fw->echo_timer, fw->echo_intv);
	}
}

static bool dfw_dependent_node_update(struct bt_mesh_dfw_subnet *dfw,
				      const struct bt_mesh_dfw_node *dependent,
				      uint16_t addr, bool type)
{
	struct bt_mesh_dfw_node *found, *dependent_new = NULL;
	bool update = false;

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].fixed_path != false) {
			continue;
		}

		if (dfw->forwarding[i].path_origin.addr == addr) {
			found = dfw_forwarding_dependent_origin_find(&dfw->forwarding[i],
								     dependent, &dependent_new);
			if (type) {
				if (found || !dependent_new) {
					continue;
				}

				dependent_new->addr = dependent->addr;
				dependent_new->secondary_count = dependent->secondary_count;

				update = true;
			} else if (found) {
				found->addr = BT_MESH_ADDR_UNASSIGNED;
				found->secondary_count = 0;

				update = true;
			}
		} else if (dfw->forwarding[i].path_target.addr == addr) {
			found = dfw_forwarding_dependent_target_find(&dfw->forwarding[i],
								     dependent, &dependent_new);
			if (type) {
				/* The Path_Endpoint field value in the message is equal to the
				 * Destination value of the table entry, the Type field value in
				 * the message is 1, the Backward_Path_Validated value of the
				 * table entry is 1, and the DependentNode is not included in the
				 * Dependent_Target_List field of the table entry.
				 */
				if (!dfw->forwarding[i].backward_path_validated ||
				    found || !dependent_new) {
					continue;
				}

				dependent_new->addr = dependent->addr;
				dependent_new->secondary_count = dependent->secondary_count;

				update = true;
			} else if (found) {
				found->addr = BT_MESH_ADDR_UNASSIGNED;
				found->secondary_count = 0;

				update = true;
			}
		}
	}

	return update;
}

bool bt_mesh_dfw_dependent_node_existed(uint16_t net_idx, uint16_t src,
					uint16_t dst, const struct bt_mesh_dfw_node *dependent)
{
	struct bt_mesh_dfw_subnet *dfw;
	struct bt_mesh_dfw_node *found;
	struct bt_mesh_subnet *sub;

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		return false;
	}

	dfw = sub->dfw;

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].fixed_path != false) {
			continue;
		}

		if (dfw->forwarding[i].path_origin.addr == src) {
			if (dfw->forwarding[i].path_target.addr != dst) {
				continue;
			}

			found = dfw_forwarding_dependent_origin_find(&dfw->forwarding[i],
								     dependent, NULL);
			if (found) {
				return true;
			}
		} else if (dfw->forwarding[i].path_target.addr == src) {
			if (dfw->forwarding[i].path_origin.addr != dst) {
				continue;
			}

			found = dfw_forwarding_dependent_target_find(&dfw->forwarding[i],
								     dependent, NULL);
			if (found) {
				return true;
			}
		}
	}

	return false;
}

int bt_mesh_dfw_dependent_node_update_start(uint16_t net_idx,
					    const struct bt_mesh_dfw_node *dependent,
					    bool type)
{
	NET_BUF_SIMPLE_DEFINE(buf, 6);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = BT_MESH_ADDR_DFW_NODES,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;
	struct bt_mesh_dfw_subnet *dfw;
	uint16_t local_addr;
	bool update;

	(void)bt_mesh_dfw_get(net_idx, &state);
	if (state != BT_MESH_FEATURE_ENABLED) {
		return -EACCES;
	}

	tx.sub = bt_mesh_subnet_get(net_idx);
	if (!tx.sub) {
		return -EINVAL;
	}

	dfw = tx.sub->dfw;
	local_addr = bt_mesh_primary_addr();

	update = dfw_dependent_node_update(dfw, dependent, local_addr, type);
	if (!update) {
		return -EALREADY;
	}

	net_buf_simple_add_u8(&buf, (uint8_t)type << 7);
	net_buf_simple_add_be16(&buf, local_addr);

	if (dependent->secondary_count) {
		net_buf_simple_add_be16(&buf, dependent->addr | BIT(LEN_PST_BIT));
		net_buf_simple_add_u8(&buf, dependent->secondary_count + 1);
	} else {
		net_buf_simple_add_be16(&buf, dependent->addr);
	}

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_DEPENDENT_NODE_UPDATE, buf.data, buf.len,
				NULL, NULL);
}

int bt_mesh_dfw_dependent_node_update(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	const size_t min_len = offsetof(struct bt_mesh_ctl_depend_node_update, dependent_addr);
	struct bt_mesh_ctl_depend_node_update *update = (void *)buf->data;
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = BT_MESH_ADDR_DFW_NODES,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.sub  = rx->sub,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	struct bt_mesh_dfw_node dependent;
	bool is_update, type;
	uint16_t addr;

	if (buf->len < min_len) {
		return -EINVAL;
	}

	if (rx->ctx.recv_dst != BT_MESH_ADDR_DFW_NODES ||
	    rx->ctx.recv_ttl != 0) {
		return -EINVAL;
	}

	if (update->octer & BIT_MASK(7)) {
		return -EINVAL;
	}

	type = (update->octer >> 7);

	addr = sys_get_be16((void *)&update->path_endpoint);
	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		return -EINVAL;
	}

	dependent.addr = sys_get_be16((void *)&update->dependent_addr);
	if (LEN_PST(dependent.addr)) {
		dependent.addr ^= BIT(LEN_PST_BIT);

		if (update->dependent_range < 2) {
			return -EINVAL;
		}

		dependent.secondary_count = update->dependent_range - 1;
	} else {
		dependent.secondary_count = 0;
	}

	if (!BT_MESH_ADDR_IS_UNICAST(dependent.addr) ||
	    !BT_MESH_ADDR_IS_UNICAST(dependent.addr + dependent.secondary_count)) {
		return -EINVAL;
	}

	is_update = dfw_dependent_node_update(dfw, &dependent, addr, type);
	if (!is_update) {
		return 0;
	}

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_DEPENDENT_NODE_UPDATE, buf->data, buf->len,
				NULL, NULL);
}

int bt_mesh_dfw_path_request_solicitation_start(uint16_t net_idx,
						const uint16_t addr_list[], uint16_t len)
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_SDU_UNSEG_MAX);
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = BT_MESH_ADDR_DFW_NODES,
		.cred     = BT_MESH_CRED_DIRECTED,
		.send_ttl = 0x7f,
	};
	struct bt_mesh_net_tx tx = {
		.ctx  = &ctx,
		.src  = bt_mesh_primary_addr(),
		.xmit = bt_mesh_dfw_ctl_net_transmit_get(),
	};
	enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;

	(void)bt_mesh_dfw_get(net_idx, &state);
	if (state != BT_MESH_FEATURE_ENABLED) {
		return -EACCES;
	}

	tx.sub = bt_mesh_subnet_get(net_idx);
	if (!tx.sub) {
		return -EINVAL;
	}

	for (int i = 0; i < len; i++) {
		if (!dfw_forwarding_dest_addr_is_valid(addr_list[i])) {
			continue;
		}

		net_buf_simple_add_be16(&buf, addr_list[i]);
	}

	/*
	 * Ignore for all-directed-forwarding-nodes, all-nodes, and all-relays
	 * fixed group addresses.
	 */
	if (!buf.len) {
		return -EINVAL;
	}

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_PATH_REQ_SOLICITATION, buf.data, buf.len,
				NULL, NULL);
}

int bt_mesh_dfw_path_request_solicitation(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	uint16_t addr, local_addr = bt_mesh_primary_addr();
	struct bt_mesh_dfw_subnet *dfw = rx->sub->dfw;
	struct bt_mesh_dfw_forwarding *fw;

	if (rx->ctx.recv_dst != BT_MESH_ADDR_DFW_NODES) {
		return -EINVAL;
	}

	if (buf->len < 2 || (buf->len % 2)) {
		return -EINVAL;
	}

	for (int i = 0; i < buf->len; i += 2) {
		addr = sys_get_be16(&buf->data[i]);
		if (!dfw_forwarding_dest_addr_is_valid(addr)) {
			continue;
		}

		fw = dfw_forwarding_find_by_dst(dfw, NULL, local_addr, addr,
						false, false);
		if (!fw) {
			continue;
		}

		dfw->update_id++;

		for (int j = 0; j < ARRAY_SIZE(dfw->discovery); j++) {
			if (dfw->discovery[j].path_origin.addr != local_addr ||
			    dfw->discovery[j].destination != addr) {
				continue;
			}

			dfw_discovery_clear(&dfw->discovery[j]);

			dfw_state_machine_event(DFW_STATE_MACHINE_EVENT_PATH_DISCOV_SUCCEED,
						fw, addr);
		}

		dfw_forwarding_clear(fw);

		(void)bt_mesh_dfw_path_origin_state_machine_start(rx->sub->net_idx, NULL,
								  addr, false);
	}

	return 0;
}

bool bt_mesh_dfw_path_origin_state_machine_existed(uint16_t net_idx, uint16_t dst)
{
	struct dfw_state_machine *machine = dfw_state_machine_find_by_dst(dst);

	if (machine) {
		return true;
	}

	return false;
}

void bt_mesh_dfw_path_origin_state_machine_msg_sent(uint16_t net_idx, uint16_t dst)
{
	struct dfw_state_machine *machine = dfw_state_machine_find_by_dst(dst);

	if (!machine) {
		return;
	}

	LOG_INF("State machine for dst 0x%04x message sent", dst);

	machine->sent = true;
}

int bt_mesh_dfw_path_origin_state_machine_start(uint16_t net_idx,
						const struct bt_mesh_dfw_node *dependent,
						uint16_t dst, bool power_up)
{
	enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;
	struct dfw_state_machine *machine;
	int err;

	err = bt_mesh_dfw_get(net_idx, &state);
	if (err) {
		return err;
	}

	if (state != BT_MESH_FEATURE_ENABLED) {
		LOG_ERR("Directed forwarding feature not enabled");
		return -ENOTSUP;
	}

	if (!dfw_forwarding_dest_addr_is_valid(dst)) {
		LOG_ERR("State Machine dst address not valid 0x%04x", dst);
		return -EINVAL;
	}

	machine = dfw_state_machine_find_by_dst(dst);
	if (machine) {
		LOG_ERR("State Machine instance already existed for 0x%04x", dst);
		return -EALREADY;
	}

	machine = dfw_state_machine_find(DFW_STATE_MACHINE_FINAL, BT_MESH_ADDR_UNASSIGNED);
	if (!machine) {
		LOG_ERR("Insuffcient state machine instance for 0x%04x", dst);
		return -ENOBUFS;
	}

	machine->dst    = dst;
	machine->net_idx = net_idx;
	machine->fw      = NULL;
	machine->sent    = false;

	if (dependent) {
		machine->dependent.addr = dependent->addr;
		machine->dependent.secondary_count = dependent->secondary_count;
	}

	if (power_up) {
		dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_POWER_UP);
	} else {
		dfw_state_machine_state_set(machine, DFW_STATE_MACHINE_INITIAL);
	}

	return 0;
}

static void dfw_node_encode(const struct bt_mesh_dfw_node *node,
			    struct net_buf_simple *buf)
{
	if (node->secondary_count) {
		net_buf_simple_add_le16(buf, ((node->addr << 1) | BIT(0)));
		net_buf_simple_add_u8(buf, node->secondary_count + 1);
	} else {
		net_buf_simple_add_le16(buf, node->addr << 1);
	}
}

static void dfw_node_decode(struct net_buf_simple *buf,
			    struct bt_mesh_dfw_node *node)
{
	node->addr = net_buf_simple_pull_le16(buf);
	if (node->addr & BIT(0)) {
		node->secondary_count = net_buf_simple_pull_u8(buf) - 1;
	} else {
		node->secondary_count = 0;
	}

	node->addr >>= 1;
}

static int send_ctl_status(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct bt_mesh_subnet *sub,
			   uint8_t status,
			   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_CTL_STATUS, 8);
	enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_CTL_STATUS);

	net_buf_simple_add_u8(&msg, status);

	net_buf_simple_add_le16(&msg, net_buf_simple_pull_le16(buf));

	if (status == STATUS_SUCCESS) {
		(void)bt_mesh_dfw_get(sub->net_idx, &state);
		net_buf_simple_add_u8(&msg, state);

		(void)bt_mesh_dfw_relay_get(sub->net_idx, &state);
		net_buf_simple_add_u8(&msg, state);

		/* Directed Proxy not support currently */
		net_buf_simple_add_u8(&msg, BT_MESH_FEATURE_NOT_SUPPORTED);
		net_buf_simple_add_u8(&msg, BT_MESH_FEATURE_NOT_SUPPORTED);

		(void)bt_mesh_dfw_friend_get(sub->net_idx, &state);
		net_buf_simple_add_u8(&msg, state);
	} else if (buf->len == 0) {
		(void)memset(net_buf_simple_add(&msg, 5), 0x00, 5);
	} else {
		(void)net_buf_simple_add_mem(&msg, net_buf_simple_pull_mem(buf, 5), 5);
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Directed Control Status");
	}

	return err;
}

static int directed_ctl_get(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
	}

	return send_ctl_status(model, ctx, sub, status, buf);
}

static int directed_ctl_set(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	uint8_t enable, relay_enable, friend_enable, status = STATUS_SUCCESS;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;
	uint16_t net_idx, proxy;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	enable = buf->data[2];
	relay_enable = buf->data[3];

	/* Unsupport Directed Proxy and Use Directed */
	proxy = sys_get_le16(&buf->data[4]);
	if (proxy != 0xffff) {
		return -EINVAL;
	}

	friend_enable = buf->data[6];

	LOG_DBG("Directed enabled %d relay %d friend %d", enable, relay_enable, friend_enable);

	(void)bt_mesh_dfw_set(net_idx, enable);
	(void)bt_mesh_dfw_relay_set(net_idx, relay_enable);

	if (IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND) &&
	    friend_enable != 0xFF) {
		(void)bt_mesh_dfw_friend_set(net_idx, friend_enable);
	}

send_status:
	return send_ctl_status(model, ctx, sub, status, buf);
}

static int send_path_metric_status(const struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_dfw_subnet *dfw,
				   uint8_t status,
				   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_PATH_METRIC_STATUS, 4);
	int err;

	bt_mesh_model_msg_init(&msg, OP_PATH_METRIC_STATUS);

	net_buf_simple_add_u8(&msg, status);

	/* net idx */
	net_buf_simple_add_le16(&msg, net_buf_simple_pull_le16(buf));

	if (status == STATUS_SUCCESS) {
		net_buf_simple_add_u8(&msg,
			DFW_PATH_METRIC_NODE | (uint8_t)(dfw->lifetime) << 3);
	} else if (buf->len == 0) {
		net_buf_simple_add_u8(&msg, 0x00);
	} else {
		net_buf_simple_add_u8(&msg, net_buf_simple_pull_u8(buf));
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Path Metric Status");
	}

	return err;
}

static int path_metric_get(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
	}

	return send_path_metric_status(model, ctx, sub->dfw, status, buf);
}

static const char *path_lifetime_to_str(enum bt_mesh_dfw_path_lifetime lifetime)
{
	static const char * const strs[] = {
		"12 Minuters",
		"2 Hours",
		"24 Hours",
		"10 Days",
	};

	return strs[lifetime];
}

static int path_metric_set(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_dfw_subnet *dfw = NULL;
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;
	uint8_t val;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	val = buf->data[2];
	if ((val & BIT_MASK(3)) != DFW_PATH_METRIC_NODE) {
		return -EINVAL;
	}

	/* Prohibit */
	if ((val >> 5) & BIT_MASK(3)) {
		return -EINVAL;
	}

	dfw->lifetime = (val >> 3) & BIT_MASK(2);

	LOG_DBG("Path lifetime %s", path_lifetime_to_str(dfw->lifetime));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_cfg_store(dfw);
	}

send_status:
	return send_path_metric_status(model, ctx, dfw, status, buf);
}

static int send_discovery_capabilities_status(const struct bt_mesh_model *model,
					      struct bt_mesh_msg_ctx *ctx,
					      struct bt_mesh_dfw_subnet *dfw,
					      uint8_t status,
					      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DISCOV_CAP_STATUS, 5);
	int err;

	bt_mesh_model_msg_init(&msg, OP_DISCOV_CAP_STATUS);

	net_buf_simple_add_u8(&msg, status);

	/* net idx */
	net_buf_simple_add_le16(&msg, net_buf_simple_pull_le16(buf));

	if (status == STATUS_SUCCESS) {
		net_buf_simple_add_u8(&msg, dfw->max_concurr_init);
	} else if (buf->len == 0) {
		net_buf_simple_add_u8(&msg, 0x00);
	} else {
		net_buf_simple_add_u8(&msg, net_buf_simple_pull_u8(buf));
	}

	if (status == STATUS_INVALID_NETKEY) {
		net_buf_simple_add_u8(&msg, 0x00);
	} else {
		net_buf_simple_add_u8(&msg, CONFIG_BT_MESH_DFW_DISCOVERY_COUNT);
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Discovery Capabilities Status");
	}

	return err;
}

static int discovery_capabilities_get(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
	}

	return send_discovery_capabilities_status(model, ctx, sub->dfw, status, buf);
}

static int discovery_capabilities_set(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	struct bt_mesh_dfw_subnet *dfw = NULL;
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint8_t max_concurr_init;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	max_concurr_init = buf->data[2];
	if (!max_concurr_init) {
		LOG_WRN("Invalid max_concurr_init %d", max_concurr_init);
		return -EINVAL;
	}

	if (max_concurr_init > CONFIG_BT_MESH_DFW_DISCOVERY_COUNT) {
		status = STATUS_CANNOT_GET;
	} else {
		dfw->max_concurr_init = max_concurr_init;
	}

	LOG_DBG("net_idx 0x%04x max_concurr_init %d", net_idx, max_concurr_init);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_cfg_store(dfw);
	}

send_status:
	return send_discovery_capabilities_status(model, ctx, dfw, status, buf);
}

static int send_forwarding_table_status(const struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					uint16_t net_idx,
					uint8_t status,
					uint16_t path_origin,
					uint16_t dst)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_FW_TABLE_STATUS, 7);
	int err;

	bt_mesh_model_msg_init(&msg, OP_FW_TABLE_STATUS);

	net_buf_simple_add_u8(&msg, status);

	/* net idx */
	net_buf_simple_add_le16(&msg, net_idx);

	net_buf_simple_add_le16(&msg, path_origin);
	net_buf_simple_add_le16(&msg, dst);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send forwarding table status");
	}

	return err;
}

static int forwarding_table_add(const struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	uint16_t net_idx, local_addr, bearer_toward_origin, bearer_toward_target;
	enum bt_mesh_feat_state state = BT_MESH_FEATURE_DISABLED;
	struct bt_mesh_dfw_node path_origin, path_target;
	bool unicast_dest_flag, backward_path_validated;
	struct bt_mesh_dfw_forwarding *fw, *fw_new = NULL;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;
	uint8_t status = STATUS_SUCCESS;

	net_idx = net_buf_simple_pull_le16(buf);
	unicast_dest_flag = (net_idx & BIT(14)) != 0;
	backward_path_validated = (net_idx & BIT(15)) != 0;

	/* Prohibit */
	if ((net_idx >> 12) & BIT_MASK(2)) {
		return -EINVAL;
	}

	dfw_node_decode(buf, &path_origin);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin.addr) ||
	    !BT_MESH_ADDR_IS_UNICAST(path_origin.addr +
		path_origin.secondary_count)) {
		return -EINVAL;
	}

	if (!unicast_dest_flag) {
		path_target.addr = net_buf_simple_pull_le16(buf);
		path_target.secondary_count = 0;
	} else {
		dfw_node_decode(buf, &path_target);
	}

	if (!dfw_forwarding_dest_addr_is_valid(path_target.addr)) {
		LOG_WRN("Invalid dst 0x%04x", path_target.addr);
		return -EINVAL;
	}

	if (ADDR_RANGE_IN(path_target.addr, &path_origin)) {
		return -EINVAL;
	}

	net_idx &= BIT_MASK(12);
	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	if (buf->len != 4) {
		return -EINVAL;
	}

	bearer_toward_origin = net_buf_simple_pull_le16(buf);
	bearer_toward_target = net_buf_simple_pull_le16(buf);

	if ((bearer_toward_origin | bearer_toward_target) >> 2) {
		status = STATUS_INVALID_BEARER;
		goto send_status;
	}

	local_addr = bt_mesh_primary_addr();

	if (!unicast_dest_flag) {
		(void)bt_mesh_dfw_friend_get(net_idx, &state);

		if (bt_mesh_has_addr(path_target.addr) &&
		    IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND) &&
		    state != BT_MESH_FEATURE_DISABLED &&
		    bt_mesh_friend_match(net_idx, path_target.addr)) {
			if (!bearer_toward_origin) {
				status = STATUS_INVALID_BEARER;
				goto send_status;
			}
		} else if (!ADDR_RANGE_IN(local_addr, &path_origin)) {
			if (!bearer_toward_origin || !bearer_toward_target) {
				status = STATUS_INVALID_BEARER;
				goto send_status;
			}
		}
	} else {
		if (ADDR_RANGE_IN(local_addr, &path_target)) {
			if (!bearer_toward_origin || bearer_toward_target) {
				status = STATUS_INVALID_BEARER;
				goto send_status;
			}

		} else if (!ADDR_RANGE_IN(local_addr, &path_origin)) {
			if (!bearer_toward_origin || !bearer_toward_target) {
				status = STATUS_INVALID_BEARER;
				goto send_status;
			}
		}
	}

	if (ADDR_RANGE_IN(local_addr, &path_origin) &&
	    (!bearer_toward_target || bearer_toward_origin)) {
		status = STATUS_INVALID_BEARER;
		goto send_status;
	}

	dfw = sub->dfw;

	fw = dfw_forwarding_find_by_dst(dfw, &fw_new, path_origin.addr,
					path_target.addr, false, true);
	if (fw) {
		dfw->update_id++;

		fw->backward_path_validated = backward_path_validated;
		fw->bearer_toward_path_origin = bearer_toward_origin;
		fw->bearer_toward_path_target = bearer_toward_target;
		goto send_status;
	} else if (!fw_new) {
		status = STATUS_INSUFF_RESOURCES;
		goto send_status;
	}

	dfw->update_id++;

	fw_new->path_origin.addr = path_origin.addr;
	fw_new->path_origin.secondary_count = path_origin.secondary_count;

	fw_new->path_target.addr = path_target.addr;
	fw_new->path_target.secondary_count = path_target.secondary_count;

	fw_new->backward_path_validated = backward_path_validated;

	fw_new->bearer_toward_path_origin = bearer_toward_origin;
	fw_new->bearer_toward_path_target = bearer_toward_target;

	fw_new->fixed_path = true;
	fw_new->lane_count = 1;
	fw_new->path_not_ready = 0;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_forwarding_store(fw_new);
	}

send_status:
	return send_forwarding_table_status(model, ctx, net_idx, status,
					    path_origin.addr, path_target.addr);
}

static int forwarding_table_del(const struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	uint16_t net_idx, path_origin, dst;
	struct bt_mesh_dfw_forwarding *fw;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;
	uint8_t status = STATUS_SUCCESS;

	net_idx = net_buf_simple_pull_le16(buf);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	path_origin = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
		LOG_WRN("Invalid path_origin 0x%04x", path_origin);
		return -EINVAL;
	}

	dst = net_buf_simple_pull_le16(buf);
	if ((path_origin == dst) || !dfw_forwarding_dest_addr_is_valid(dst))  {
		LOG_WRN("Invalid dst 0x%04x", dst);
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin, dst, false, true);
	if (fw) {
		dfw->update_id++;
		dfw_forwarding_clear(fw);
	}

send_status:
	return send_forwarding_table_status(model, ctx, net_idx, status,
					    path_origin, dst);
}

static int send_forwarding_table_dependents_status(const struct bt_mesh_model *model,
						   struct bt_mesh_msg_ctx *ctx,
						   uint16_t net_idx,
						   uint8_t status,
						   uint16_t path_origin,
						   uint16_t dst)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_FW_TABLE_DEP_STATUS, 7);
	int err;

	bt_mesh_model_msg_init(&msg, OP_FW_TABLE_DEP_STATUS);

	net_buf_simple_add_u8(&msg, status);

	/* net idx */
	net_buf_simple_add_le16(&msg, net_idx);

	net_buf_simple_add_le16(&msg, path_origin);
	net_buf_simple_add_le16(&msg, dst);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send forwarding table dependents status");
	}

	return err;
}

static int forwarding_table_dependents_add(const struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	uint8_t dependent_origin_list_size, dependent_target_list_size, status = STATUS_SUCCESS;
	struct bt_mesh_dfw_node dependent_origin_list[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	struct bt_mesh_dfw_node dependent_target_list[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	struct bt_mesh_dfw_node local, *dependent_new;
	uint16_t net_idx, path_origin, dst;
	struct bt_mesh_dfw_forwarding *fw;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;

	net_idx = net_buf_simple_pull_le16(buf);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Invalid network index");
		return -EINVAL;
	}

	path_origin = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
		LOG_WRN("Invalid path_origin 0x%04x", path_origin);
		return -EINVAL;
	}

	dst = net_buf_simple_pull_le16(buf);
	if ((path_origin == dst) || !dfw_forwarding_dest_addr_is_valid(dst))  {
		LOG_WRN("Invalid dst 0x%04x", dst);
		return -EINVAL;
	}

	dependent_origin_list_size = net_buf_simple_pull_u8(buf);
	dependent_target_list_size = net_buf_simple_pull_u8(buf);

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	if (dependent_origin_list_size > BT_MESH_DFW_DEPENDENT_NODES_COUNT ||
	    dependent_target_list_size > BT_MESH_DFW_DEPENDENT_NODES_COUNT) {
		status = STATUS_INSUFF_RESOURCES;
		goto send_status;
	}

	for (int i = 0; i < dependent_origin_list_size; i++) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		dfw_node_decode(buf, &dependent_origin_list[i]);
		if (!BT_MESH_ADDR_IS_UNICAST(dependent_origin_list[i].addr) ||
		    !BT_MESH_ADDR_IS_UNICAST(dependent_origin_list[i].addr +
					     dependent_origin_list[i].secondary_count)) {
			return -EINVAL;
		}

		if (ADDR_RANGE_IN(path_origin, &dependent_origin_list[i]) ||
		    ADDR_RANGE_IN(dst, &dependent_origin_list[i])) {
			return -EINVAL;
		}
	}

	for (int i = 0; i < dependent_target_list_size; i++) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		dfw_node_decode(buf, &dependent_target_list[i]);
		if (!BT_MESH_ADDR_IS_UNICAST(dependent_target_list[i].addr) ||
		    !BT_MESH_ADDR_IS_UNICAST(dependent_target_list[i].addr +
					     dependent_target_list[i].secondary_count)) {
			return -EINVAL;
		}

		if (ADDR_RANGE_IN(path_origin, &dependent_target_list[i]) ||
		    ADDR_RANGE_IN(dst, &dependent_target_list[i])) {
			return -EINVAL;
		}

		for (int j = 0; j < dependent_origin_list_size; j++) {
			if (ADDR_RANGE_IN(dependent_target_list[i].addr,
					  &dependent_origin_list[j])) {
				return -EINVAL;
			}
		}
	}

	local.addr = bt_mesh_primary_addr();
	local.secondary_count = bt_mesh_elem_count() - 1;
	if (ADDR_RANGE_IN(path_origin, &local)) {
		if (dependent_origin_list_size && dfw_is_dependent_node_enable(net_idx)) {
			status = STATUS_FEAT_NOT_SUPP;
			goto send_status;
		}
	} else if (ADDR_RANGE_IN(dst, &local)) {
		if (dependent_target_list_size && dfw_is_dependent_node_enable(net_idx)) {
			status = STATUS_FEAT_NOT_SUPP;
			goto send_status;
		}
	}

	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin, dst, false, true);
	if (!fw) {
		status = STATUS_INVALID_PATH_ENTRY;
		goto send_status;
	}

	for (int i = 0; i < dependent_origin_list_size; i++) {
		/* duplicate */
		if (dfw_forwarding_dependent_origin_find(fw, &dependent_origin_list[i],
							 &dependent_new)) {
			continue;
		}

		if (!dependent_new) {
			status = STATUS_INSUFF_RESOURCES;
			goto send_status;
		}

		dfw->update_id++;

		dependent_new->addr = dependent_origin_list[i].addr;
		dependent_new->secondary_count = dependent_origin_list[i].secondary_count;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			dfw_subnet_forwarding_dependent_store(fw, dependent_new, true);
		}
	}

	for (int i = 0; i < dependent_target_list_size; i++) {
		/* duplicate */
		if (dfw_forwarding_dependent_target_find(fw, &dependent_target_list[i],
							 &dependent_new)) {
			continue;
		}

		if (!dependent_new) {
			status = STATUS_INSUFF_RESOURCES;
			goto send_status;
		}

		dfw->update_id++;

		dependent_new->addr = dependent_target_list[i].addr;
		dependent_new->secondary_count = dependent_target_list[i].secondary_count;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			dfw_subnet_forwarding_dependent_store(fw, dependent_new, false);
		}
	}

send_status:
	return send_forwarding_table_dependents_status(model, ctx, net_idx, status,
						       path_origin, dst);
}

static int forwarding_table_dependents_del(const struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	uint8_t dependent_origin_list_size, dependent_target_list_size, status = STATUS_SUCCESS;
	uint16_t dependent_origin_list[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	uint16_t dependent_target_list[BT_MESH_DFW_DEPENDENT_NODES_COUNT];
	struct bt_mesh_dfw_node *found, dependent;
	uint16_t net_idx, path_origin, dst;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;
	struct bt_mesh_dfw_forwarding *fw;

	net_idx = net_buf_simple_pull_le16(buf);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	path_origin = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
		LOG_WRN("Invalid path origin 0x%04x", path_origin);
		return -EINVAL;
	}

	dst = net_buf_simple_pull_le16(buf);
	if ((path_origin == dst) || !dfw_forwarding_dest_addr_is_valid(dst))  {
		LOG_WRN("Invalid dst 0x%04x", dst);
		return -EINVAL;
	}

	dependent_origin_list_size = net_buf_simple_pull_u8(buf);
	dependent_target_list_size = net_buf_simple_pull_u8(buf);

	if (dependent_origin_list_size > BT_MESH_DFW_DEPENDENT_NODES_COUNT ||
	    dependent_target_list_size > BT_MESH_DFW_DEPENDENT_NODES_COUNT) {
		return -EINVAL;
	}

	for (int i = 0; i < dependent_origin_list_size; i++) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		dependent_origin_list[i] = net_buf_simple_pull_le16(buf);

		if (!BT_MESH_ADDR_IS_UNICAST(dependent_origin_list[i])) {
			return -EINVAL;
		}

		if (dependent_origin_list[i] == path_origin || dependent_origin_list[i] == dst) {
			return -EINVAL;
		}
	}

	for (int i = 0; i < dependent_target_list_size; i++) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		dependent_target_list[i] = net_buf_simple_pull_le16(buf);

		if (!BT_MESH_ADDR_IS_UNICAST(dependent_target_list[i])) {
			return -EINVAL;
		}

		if (dependent_target_list[i] == path_origin ||
		    dependent_target_list[i] == dst) {
			return -EINVAL;
		}

		for (int j = 0; j < dependent_origin_list_size; j++) {
			if (dependent_target_list[i] == dependent_origin_list[j]) {
				return -EINVAL;
			}
		}
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin, dst, false, true);
	if (!fw) {
		status = STATUS_INVALID_PATH_ENTRY;
		goto send_status;
	}

	for (int i = 0; i < dependent_origin_list_size; i++) {
		dependent.addr = dependent_origin_list[i];
		dependent.secondary_count = 0;

		found = dfw_forwarding_dependent_origin_find(fw, &dependent, NULL);
		if (!found) {
			continue;
		}

		dfw->update_id++;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			dfw_dependent_node_setting_clear(fw, found);
		}

		found->addr = BT_MESH_ADDR_UNASSIGNED;
		found->secondary_count = 0;
	}

	for (int i = 0; i < dependent_target_list_size; i++) {
		dependent.addr = dependent_target_list[i];
		dependent.secondary_count = 0;

		found = dfw_forwarding_dependent_target_find(fw, &dependent, NULL);
		if (!found) {
			continue;
		}

		dfw->update_id++;

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			dfw_dependent_node_setting_clear(fw, found);
		}

		found->addr = BT_MESH_ADDR_UNASSIGNED;
		found->secondary_count = 0;
	}

send_status:
	return send_forwarding_table_dependents_status(model, ctx, net_idx, status,
						       path_origin, dst);
}

static int forwarding_table_entries_count_get(const struct bt_mesh_model *model,
					      struct bt_mesh_msg_ctx *ctx,
					      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_FW_TABLE_ENT_COUNT_STATUS, 9);
	uint16_t net_idx, fixed_path_count = 0, no_fixed_path_count = 0;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;
	int err;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	bt_mesh_model_msg_init(&msg, OP_FW_TABLE_ENT_COUNT_STATUS);

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		net_buf_simple_add_u8(&msg, STATUS_INVALID_NETKEY);
		net_buf_simple_add_le16(&msg, net_idx);
		goto send_status;
	}

	dfw = sub->dfw;

	net_buf_simple_add_u8(&msg, STATUS_SUCCESS);
	net_buf_simple_add_le16(&msg, net_idx);

	net_buf_simple_add_le16(&msg, dfw->update_id);

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].path_origin.addr ==
					BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (dfw->forwarding[i].fixed_path) {
			fixed_path_count++;
		} else {
			no_fixed_path_count++;
		}
	}

	net_buf_simple_add_le16(&msg, fixed_path_count);
	net_buf_simple_add_le16(&msg, no_fixed_path_count);

send_status:
	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send forwarding table entries count Status");
	}

	return err;
}

static int forwarding_table_dependents_get(const struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_FW_TABLE_DEP_GET_STATUS, BT_MESH_TX_SDU_MAX);
	uint8_t filter_list, *status, *dependent_origin_range, *dependent_target_range;
	uint16_t net_idx, idx = 0, start_idx, path_origin, dst, update_id = 0;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_forwarding *fw;
	struct bt_mesh_dfw_subnet *dfw;
	bool fixed;
	int err;

	net_idx = net_buf_simple_pull_le16(buf);

	filter_list = (net_idx >> 12) & BIT_MASK(2);
	fixed = (net_idx >> 14) & BIT(0);

	/* Prohibit Or Filter None */
	if ((net_idx >> 15) || !filter_list) {
		return -EINVAL;
	}

	start_idx = net_buf_simple_pull_le16(buf);

	path_origin = net_buf_simple_pull_le16(buf);
	if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
		LOG_WRN("Invalid path_origin 0x%04x", path_origin);
		return -EINVAL;
	}

	dst = net_buf_simple_pull_le16(buf);
	if ((path_origin == dst) || !dfw_forwarding_dest_addr_is_valid(dst))  {
		LOG_WRN("Invalid dst 0x%04x", dst);
		return -EINVAL;
	}

	if (buf->len == 2) {
		update_id = sys_get_le16(buf->data);
	}

	bt_mesh_model_msg_init(&msg, OP_FW_TABLE_DEP_GET_STATUS);

	status = net_buf_simple_add(&msg, 1);
	net_buf_simple_add_le16(&msg, net_idx);
	net_buf_simple_add_le16(&msg, start_idx);
	net_buf_simple_add_le16(&msg, path_origin);
	net_buf_simple_add_le16(&msg, dst);

	net_idx &= BIT_MASK(12);
	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		*status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	if (buf->len && (dfw->update_id != update_id)) {
		net_buf_simple_add_le16(&msg, dfw->update_id);
		*status = STATUS_OBS_INFO;
		goto send_status;
	}

	fw = dfw_forwarding_find_by_dst(dfw, NULL, path_origin, dst, false, fixed);
	if (!fw) {
		*status = STATUS_INVALID_PATH_ENTRY;
		goto send_status;
	}

	net_buf_simple_add_le16(&msg, dfw->update_id);

	*status = STATUS_SUCCESS;

	dependent_origin_range = net_buf_simple_add(&msg, 1);
	*dependent_origin_range = 0x00;

	dependent_target_range = net_buf_simple_add(&msg, 1);
	*dependent_target_range = 0x00;

	if (filter_list & BIT(0)) {
		for (int i = 0; i < ARRAY_SIZE(fw->dependent_origin); i++) {
			if (fw->dependent_origin[i].addr == BT_MESH_ADDR_UNASSIGNED) {
				continue;
			}

			if (start_idx > idx++) {
				continue;
			}

			(*dependent_origin_range)++;

			dfw_node_encode(&fw->dependent_origin[i], &msg);
		}
	}

	if (filter_list & BIT(1)) {
		for (int i = 0; i < ARRAY_SIZE(fw->dependent_target); i++) {
			if (fw->dependent_target[i].addr == BT_MESH_ADDR_UNASSIGNED) {
				continue;
			}

			if (start_idx > idx++) {
				continue;
			}

			(*dependent_target_range)++;

			dfw_node_encode(&fw->dependent_target[i], &msg);
		}
	}

send_status:
	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send forwarding table dependents get status");
	}

	return err;
}

static void forwarding_table_encode(struct net_buf_simple *buf,
				    struct bt_mesh_dfw_forwarding *fw)
{
	uint16_t *p_header, header = 0, local_addr = bt_mesh_primary_addr();
	uint16_t dependent_origin_range = 0, dependent_target_range = 0;

	p_header = net_buf_simple_add(buf, 2);

	if (fw->fixed_path) {
		header |= BIT(0);

		dfw_node_encode(&fw->path_origin, buf);
	} else {
		net_buf_simple_add_u8(buf, fw->lane_count);
		net_buf_simple_add_le16(buf, k_ticks_to_ms_near32(
						k_work_delayable_remaining_get(&fw->timer)) /
						(60 * 1000));
		net_buf_simple_add_u8(buf, fw->forwarding_number);

		dfw_node_encode(&fw->path_origin, buf);
	}

	for (int i = 0; i < ARRAY_SIZE(fw->dependent_origin); i++) {
		if (fw->dependent_origin[i].addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		dependent_origin_range++;
	}

	if (dependent_origin_range > 0xff) {
		header |= (0x0002 << 5);
		net_buf_simple_add_le16(buf, dependent_origin_range);
	} else {
		header |= (0x0001 << 5);
		net_buf_simple_add_u8(buf, (uint8_t)dependent_origin_range);
	}

	if (fw->path_origin.addr != local_addr) {
		header |= BIT(3);

		net_buf_simple_add_le16(buf, fw->bearer_toward_path_origin);
	}

	if (BT_MESH_ADDR_IS_UNICAST(fw->path_target.addr)) {
		header |= BIT(1);
		dfw_node_encode(&fw->path_target, buf);
	} else {
		net_buf_simple_add_le16(buf, fw->path_target.addr);
	}

	if (fw->backward_path_validated) {
		header |= BIT(2);
	}

	for (int i = 0; i < ARRAY_SIZE(fw->dependent_target); i++) {
		if (fw->dependent_target[i].addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		dependent_target_range++;
	}

	if (dependent_target_range > 0xff) {
		header |= (0x0002 << 7);
		net_buf_simple_add_le16(buf, dependent_target_range);
	} else {
		header |= (0x0001 << 7);
		net_buf_simple_add_u8(buf, (uint8_t)dependent_target_range);
	}

	if (fw->path_target.addr != local_addr) {
		header |= BIT(4);

		net_buf_simple_add_le16(buf, fw->bearer_toward_path_target);
	}

	(void)memcpy(p_header, &header, 2);
}

static int forwarding_table_entries_get(const struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_FW_TABLE_ENT_STATUS, BT_MESH_TX_SDU_MAX);
	uint16_t path_origin = 0, dst = 0, update_id = 0;
	uint16_t net_idx, idx = 0, start_idx;
	struct bt_mesh_subnet *sub = NULL;
	struct bt_mesh_dfw_subnet *dfw;
	uint8_t filter_list, *status;
	int err;

	net_idx = net_buf_simple_pull_le16(buf);

	filter_list = (net_idx >> 12) & BIT_MASK(4);

	/* Bits 0 and 1 of the Filter_Mask field shall not both be set to 0. */
	if (!(filter_list & BIT_MASK(2))) {
		LOG_WRN("Invalid filter_list 0x%02x", filter_list);
		return -EINVAL;
	}

	start_idx = net_buf_simple_pull_le16(buf);

	LOG_DBG("net_idx 0x%04x, filter_list 0x%x, start_idx 0x%04x",
		net_idx, filter_list, start_idx);

	if (filter_list & BIT(2)) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		path_origin = net_buf_simple_pull_le16(buf);
		if (!BT_MESH_ADDR_IS_UNICAST(path_origin)) {
			return -EINVAL;
		}

		LOG_DBG("Path Origin 0x%04x", path_origin);
	}

	if (filter_list & BIT(3)) {
		if (buf->len < 2) {
			return -EINVAL;
		}

		dst = net_buf_simple_pull_le16(buf);
		if (((filter_list & BIT(2)) && (path_origin == dst)) ||
		    !dfw_forwarding_dest_addr_is_valid(dst))  {
			return -EINVAL;
		}

		LOG_DBG("Destination 0x%04x", dst);
	}

	if (buf->len == 2) {
		update_id = sys_get_le16(buf->data);

		LOG_DBG("Update Id 0x%04x", update_id);
	}

	bt_mesh_model_msg_init(&msg, OP_FW_TABLE_ENT_STATUS);

	status = net_buf_simple_add(&msg, 1);
	net_buf_simple_add_le16(&msg, net_idx);
	net_buf_simple_add_le16(&msg, start_idx);

	if (filter_list & BIT(2)) {
		net_buf_simple_add_le16(&msg, path_origin);
	}
	if (filter_list & BIT(3)) {
		net_buf_simple_add_le16(&msg, dst);
	}

	net_idx &= BIT_MASK(12);
	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		*status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	net_buf_simple_add_le16(&msg, dfw->update_id);

	if (buf->len && (dfw->update_id != update_id)) {
		*status = STATUS_OBS_INFO;
		goto send_status;
	}

	*status = STATUS_SUCCESS;

	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (dfw->forwarding[i].fixed_path && !(filter_list & BIT(0))) {
			continue;
		}

		if (!dfw->forwarding[i].fixed_path && !(filter_list & BIT(1))) {
			continue;
		}

		if ((filter_list & BIT(2)) &&
		    dfw->forwarding[i].path_origin.addr != path_origin) {
			continue;
		}

		if ((filter_list & BIT(3)) &&
		    dfw->forwarding[i].path_target.addr != dst) {
			continue;
		}

		if (start_idx > idx++) {
			continue;
		}

		forwarding_table_encode(&msg, &dfw->forwarding[i]);
	}

send_status:
	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send forwarding table entries get status");
	}

	return err;
}

static int send_wanted_lanes_status(const struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_dfw_subnet *dfw,
				    uint8_t status,
				    struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_WANTED_LANES_STATUS, 4);
	int err;

	bt_mesh_model_msg_init(&msg, OP_WANTED_LANES_STATUS);

	net_buf_simple_add_u8(&msg, status);

	net_buf_simple_add_le16(&msg, net_buf_simple_pull_le16(buf));

	if (status == STATUS_SUCCESS) {
		net_buf_simple_add_u8(&msg, dfw->wanted_lanes);
	} else if (buf->len == 0) {
		net_buf_simple_add_u8(&msg, 0x00);
	} else {
		net_buf_simple_add_u8(&msg, net_buf_simple_pull_u8(buf));
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send wanted lane status");
	}

	return err;
}

static int wanted_lanes_get(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
	}

	return send_wanted_lanes_status(model, ctx, sub->dfw, status, buf);
}

static int wanted_lanes_set(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	uint8_t wanted_lanes, status = STATUS_SUCCESS;
	struct bt_mesh_dfw_subnet *dfw = NULL;
	struct bt_mesh_subnet *sub = NULL;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	wanted_lanes = buf->data[2];
	if (!wanted_lanes) {
		LOG_WRN("Invalid wanted_lanes %d", wanted_lanes);
		return -EINVAL;
	}

	dfw->wanted_lanes = wanted_lanes;

	LOG_DBG("net_idx 0x%04x wanted_lanes %d", net_idx, wanted_lanes);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_cfg_store(dfw);
	}

send_status:
	return send_wanted_lanes_status(model, ctx, dfw, status, buf);
}

static int send_two_way_path_status(const struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_dfw_subnet *dfw,
				    uint8_t status,
				    struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_TWO_WAY_PATH_STATUS, 4);
	int err;

	bt_mesh_model_msg_init(&msg, OP_TWO_WAY_PATH_STATUS);

	net_buf_simple_add_u8(&msg, status);

	net_buf_simple_add_le16(&msg, net_buf_simple_pull_le16(buf));

	if (status == STATUS_SUCCESS) {
		net_buf_simple_add_u8(&msg, dfw->two_way_path);
	} else if (buf->len == 0) {
		net_buf_simple_add_u8(&msg, 0x00);
	} else {
		net_buf_simple_add_u8(&msg, net_buf_simple_pull_u8(buf));
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send wanted lane status");
	}

	return err;
}

static int two_way_path_get(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
	}

	return send_two_way_path_status(model, ctx, sub->dfw, status, buf);
}

static int two_way_path_set(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	uint8_t two_way_path, status = STATUS_SUCCESS;
	struct bt_mesh_dfw_subnet *dfw = NULL;
	struct bt_mesh_subnet *sub = NULL;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	two_way_path = buf->data[2];
	if (two_way_path >> 1) {
		LOG_WRN("Invalid two_way_path %d", two_way_path);
		return -EINVAL;
	}

	dfw->two_way_path = !!(two_way_path);

	LOG_DBG("net_idx 0x%04x two_way_path %d", net_idx, two_way_path);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_cfg_store(dfw);
	}

send_status:
	return send_two_way_path_status(model, ctx, dfw, status, buf);
}

static int directed_network_transmit_get(const struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_NET_TRANSMIT_STATUS, 1);
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_NET_TRANSMIT_STATUS);

	net_buf_simple_add_u8(&msg, dfw_cfg.directed_net_transmit);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send directed network transmit status");
	}

	return err;
}

static int directed_network_transmit_set(const struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	uint8_t xmit = net_buf_simple_pull_u8(buf);

	dfw_cfg.directed_net_transmit = xmit;

	LOG_DBG("Directed network transmit 0x%02x (count %u interval %ums)",
		xmit, BT_MESH_TRANSMIT_COUNT(xmit), BT_MESH_TRANSMIT_INT(xmit));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}

	return directed_network_transmit_get(model, ctx, buf);
}

static int directed_relay_retransmit_get(const struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_RELAY_RETRANS_STATUS, 1);
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_RELAY_RETRANS_STATUS);

	net_buf_simple_add_u8(&msg, dfw_cfg.directed_relay_retransmit);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send directed relay retransmit status");
	}

	return err;
}

static int directed_relay_retransmit_set(const struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	uint8_t xmit = net_buf_simple_pull_u8(buf);

	dfw_cfg.directed_relay_retransmit = xmit;

	LOG_DBG("Directed relay retransmit 0x%02x (count %u interval %ums)",
		xmit, BT_MESH_TRANSMIT_COUNT(xmit), BT_MESH_TRANSMIT_INT(xmit));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}

	return directed_relay_retransmit_get(model, ctx, buf);
}

static int send_path_echo_interval_status(const struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct bt_mesh_dfw_subnet *dfw,
					  uint8_t status,
					  struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_PATH_ECHO_INTV_STATUS, 5);
	int err;

	bt_mesh_model_msg_init(&msg, OP_PATH_ECHO_INTV_STATUS);

	net_buf_simple_add_u8(&msg, status);

	net_buf_simple_add_le16(&msg, net_buf_simple_pull_le16(buf));

	if (status == STATUS_SUCCESS) {
		net_buf_simple_add_u8(&msg, dfw->unicast_echo_intv);
		net_buf_simple_add_u8(&msg, dfw->multicast_echo_intv);
	} else if (buf->len == 0) {
		(void)memset(net_buf_simple_add(&msg, 2), 0x00, 2);
	} else {
		(void)net_buf_simple_add_mem(&msg, net_buf_simple_pull_mem(buf, 2), 2);
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send path echo interval status");
	}

	return err;
}

static int path_echo_interval_get(const struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub = NULL;
	uint8_t status = STATUS_SUCCESS;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
	}

	return send_path_echo_interval_status(model, ctx, sub->dfw, status, buf);
}

static void dfw_path_echo_intv_set(struct bt_mesh_dfw_subnet *dfw,
				   struct bt_mesh_dfw_forwarding *fw,
				   uint8_t percent)
{
	fw->echo_intv = path_lifetime_get(dfw->lifetime);
	fw->echo_intv.ticks *= percent;
	fw->echo_intv.ticks /= 100;

	(void)k_work_reschedule(&fw->echo_timer, fw->echo_intv);
}

static void dfw_path_echo_reschedule(struct bt_mesh_dfw_subnet *dfw)
{
	for (int i = 0; i < ARRAY_SIZE(dfw->forwarding); i++) {
		if (dfw->forwarding[i].path_origin.addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (BT_MESH_ADDR_IS_UNICAST(dfw->forwarding[i].path_target.addr)) {
			if (dfw->unicast_echo_intv) {
				dfw_path_echo_intv_set(dfw, &dfw->forwarding[i],
						       dfw->unicast_echo_intv);
				continue;
			}
		} else if (dfw->multicast_echo_intv) {
			dfw_path_echo_intv_set(dfw, &dfw->forwarding[i],
					       dfw->multicast_echo_intv);
			continue;
		}

		(void)k_work_cancel_delayable(&dfw->forwarding[i].echo_timer);
	}
}

static int path_echo_interval_set(const struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	uint8_t unicast_echo_intv, multicast_echo_intv, status = STATUS_SUCCESS;
	struct bt_mesh_dfw_subnet *dfw = NULL;
	struct bt_mesh_subnet *sub = NULL;
	uint16_t net_idx;

	net_idx = sys_get_le16(buf->data);
	if (net_idx == BT_MESH_KEY_UNUSED) {
		LOG_WRN("Prohibited network index");
		return -EINVAL;
	}

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		status = STATUS_INVALID_NETKEY;
		goto send_status;
	}

	dfw = sub->dfw;

	unicast_echo_intv = buf->data[2];
	multicast_echo_intv = buf->data[3];

	if ((unicast_echo_intv < 0xff && unicast_echo_intv > 0x63) ||
	    (multicast_echo_intv < 0xff && multicast_echo_intv > 0x63)) {
		return -EINVAL;
	}

	if (unicast_echo_intv == 0xff && multicast_echo_intv == 0xff) {
		return -EINVAL;
	}

	if (unicast_echo_intv != 0xff) {
		dfw->unicast_echo_intv = unicast_echo_intv;
	}

	if (multicast_echo_intv != 0xff) {
		dfw->multicast_echo_intv = multicast_echo_intv;
	}

	LOG_DBG("net_idx 0x%04x unicast echo interval %d(s) multicast echo interval %d(s)",
		net_idx, unicast_echo_intv, multicast_echo_intv);

	dfw_path_echo_reschedule(dfw);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_subnet_cfg_store(dfw);
	}

send_status:
	return send_path_echo_interval_status(model, ctx, dfw, status, buf);
}

static int directed_path_get(const struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_PATH_STATUS, 8);
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_PATH_STATUS);

	net_buf_simple_add_le16(&msg, CONFIG_BT_MESH_SUBNET_COUNT *
				      CONFIG_BT_MESH_DFW_FORWARDING_COUNT);

	net_buf_simple_add_le16(&msg, CONFIG_BT_MESH_SUBNET_COUNT *
				      CONFIG_BT_MESH_DFW_FORWARDING_COUNT);

	net_buf_simple_add_le16(&msg, 0x0000);

	if (IS_ENABLED(CONFIG_BT_MESH_DFW_FRIEND)) {
		net_buf_simple_add_le16(&msg, CONFIG_BT_MESH_SUBNET_COUNT *
					      CONFIG_BT_MESH_DFW_FORWARDING_COUNT);
	} else {
		net_buf_simple_add_le16(&msg, 0x0000);
	}

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Forwarding Table Status");
	}

	return err;
}


static int rssi_threshold_get(const struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_RSSI_THRESHOLD_STATUS, 2);
	int err;

	bt_mesh_model_msg_init(&msg, OP_RSSI_THRESHOLD_STATUS);

	net_buf_simple_add_u8(&msg, CONFIG_BT_MESH_DFW_DEFAULT_RSSI_THRESHOLD);
	net_buf_simple_add_u8(&msg, dfw_cfg.rssi_margin);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Forwarding Table Status");
	}

	return err;
}

static int rssi_threshold_set(const struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	uint8_t rssi_margin;

	rssi_margin = net_buf_simple_pull_u8(buf);

	if (rssi_margin > 0x32) {
		LOG_WRN("Invalid rssi margen %d", rssi_margin);
		return -EINVAL;
	}

	dfw_cfg.rssi_margin = rssi_margin;

	LOG_DBG("rssi margen %d", rssi_margin);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}

	return rssi_threshold_get(model, ctx, buf);
}

static int send_directed_publish_policy_status(const struct bt_mesh_model *model,
					       struct bt_mesh_msg_ctx *ctx,
					       struct bt_mesh_model_pub *pub,
					       uint8_t status,
					       struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_PUB_POLICY_STATUS, 8);
	uint16_t len = buf->len;
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_PUB_POLICY_STATUS);

	net_buf_simple_add_u8(&msg, status);

	/* Directed Publish Policy Get Message */
	if (buf->len == 4 || buf->len == 6) {
		net_buf_simple_add_u8(&msg,
			status == STATUS_SUCCESS ? pub->cred == BT_MESH_CRED_DIRECTED : 0x00);
	}

	(void)net_buf_simple_add_mem(&msg, net_buf_simple_pull_mem(buf, len), len);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send path echo interval status");
	}

	return err;
}

static const struct bt_mesh_model *get_model(const struct bt_mesh_elem *elem,
					     uint8_t *data, uint16_t len)
{
	uint16_t company, id;

	if (len < 4) {
		id = sys_get_le16(data);

		LOG_DBG("ID 0x%04x addr 0x%04x", id, elem->rt->addr);

		return bt_mesh_model_find(elem, id);
	}

	company = sys_get_le16(data);
	id = sys_get_le16(&data[2]);

	LOG_DBG("Company 0x%04x ID 0x%04x addr 0x%04x", company, id, elem->rt->addr);

	return bt_mesh_model_find_vnd(elem, company, id);
}

static int directed_publish_policy_get(const struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct bt_mesh_model_pub *pub = NULL;
	const struct bt_mesh_model *mod;
	uint8_t status = STATUS_SUCCESS;
	const struct bt_mesh_elem *elem;
	uint16_t elem_addr;

	elem_addr = sys_get_le16(buf->data);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		LOG_WRN("Invalid element address 0x%04x", elem_addr);
		return -EINVAL;
	}

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, &buf->data[2], buf->len - 2);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!mod->pub) {
		status = STATUS_NVAL_PUB_PARAM;
		goto send_status;
	}

	pub = mod->pub;

send_status:
	return send_directed_publish_policy_status(model, ctx, pub, status, buf);
}

static int directed_publish_policy_set(const struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct bt_mesh_model_pub *pub = NULL;
	const struct bt_mesh_model *mod;
	uint8_t status = STATUS_SUCCESS;
	const struct bt_mesh_elem *elem;
	uint16_t elem_addr;
	uint8_t policy;

	policy = buf->data[0];
	if (policy > 0x01) {
		LOG_WRN("Invalid policy 0x%02x", policy);
		return -EINVAL;
	}

	elem_addr = sys_get_le16(&buf->data[1]);
	if (!BT_MESH_ADDR_IS_UNICAST(elem_addr)) {
		LOG_WRN("Invalid element address 0x%04x", elem_addr);
		return -EINVAL;
	}

	elem = bt_mesh_elem_find(elem_addr);
	if (!elem) {
		status = STATUS_INVALID_ADDRESS;
		goto send_status;
	}

	mod = get_model(elem, &buf->data[3], buf->len - 3);
	if (!mod) {
		status = STATUS_INVALID_MODEL;
		goto send_status;
	}

	if (!mod->pub) {
		status = STATUS_NVAL_PUB_PARAM;
		goto send_status;
	}

	pub = mod->pub;

	if (!policy) {
		pub->cred = BT_MESH_CRED_FLOODING;
	} else {
		pub->cred = BT_MESH_CRED_DIRECTED;
	}

	LOG_DBG("Publish policy %d", policy);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_model_pub_store(mod);
	}

send_status:
	return send_directed_publish_policy_status(model, ctx, pub, status, buf);
}

static int path_discovery_timing_control_get(const struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_PATH_DISCOV_TIM_CTL_STATUS, 5);
	int err;

	bt_mesh_model_msg_init(&msg, OP_PATH_DISCOV_TIM_CTL_STATUS);

	net_buf_simple_add_le16(&msg, dfw_cfg.monitor_intv);
	net_buf_simple_add_le16(&msg, dfw_cfg.discov_retry_intv);
	net_buf_simple_add_u8(&msg, (uint8_t)dfw_cfg.discov_intv |
				    (uint8_t)dfw_cfg.lane_discov_guard_intv << 1);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send Forwarding Table Status");
	}

	return err;
}

static int path_discovery_timing_control_set(const struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	uint16_t monitor_intv, discov_retry_intv;
	uint8_t flags;

	monitor_intv = net_buf_simple_pull_le16(buf);
	discov_retry_intv = net_buf_simple_pull_le16(buf);
	flags = net_buf_simple_pull_u8(buf);

	/* Prohibit */
	if (flags >> 2) {
		LOG_WRN("Invalid flags 0x%02x", flags);
		return -EINVAL;
	}

	dfw_cfg.monitor_intv = monitor_intv;
	dfw_cfg.discov_retry_intv = discov_retry_intv;
	dfw_cfg.discov_intv = flags & BIT(0);
	dfw_cfg.lane_discov_guard_intv = (flags >> 1) & BIT(0);

	LOG_DBG("Monitor interval %d(s)", dfw_cfg.monitor_intv);
	LOG_DBG("Discovery retry interval %d(s)", dfw_cfg.discov_retry_intv);
	LOG_DBG("Discovery interval %s(s)",
		dfw_cfg.discov_intv == BT_MESH_DFW_PATH_DISCOV_INTV_5_S ?
		"5" : "30");
	LOG_DBG("Lane discovery guard interval %s(s)",
		dfw_cfg.lane_discov_guard_intv == BT_MESH_DFW_LANE_DISCOVERY_GUARD_2_S ?
		"2" : "10");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}

	return path_discovery_timing_control_get(model, ctx, buf);
}

static int directed_ctl_network_transmit_get(const struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_CTL_NET_TRANS_STATUS, 1);
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_CTL_NET_TRANS_STATUS);

	net_buf_simple_add_u8(&msg, dfw_cfg.directed_ctl_net_transmit);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send directed control network transmit status");
	}

	return err;
}

static int directed_ctl_network_transmit_set(const struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	uint8_t xmit = net_buf_simple_pull_u8(buf);

	dfw_cfg.directed_ctl_net_transmit = xmit;

	LOG_DBG("Directed control network transmit:0x%02x (count %u interval %ums)",
		xmit, BT_MESH_TRANSMIT_COUNT(xmit), BT_MESH_TRANSMIT_INT(xmit));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}

	return directed_ctl_network_transmit_get(model, ctx, buf);
}

static int dfw_ctl_relay_retrans_get(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, OP_DIRECTED_CTL_RELAY_RETRANS_STATUS, 1);
	int err;

	bt_mesh_model_msg_init(&msg, OP_DIRECTED_CTL_RELAY_RETRANS_STATUS);

	net_buf_simple_add_u8(&msg, dfw_cfg.directed_ctl_relay_retransmit);

	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Unable to send directed control relay retransmit status");
	}

	return err;
}

static int dfw_ctl_relay_retrans_set(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	uint8_t xmit = net_buf_simple_pull_u8(buf);

	dfw_cfg.directed_ctl_relay_retransmit = xmit;

	LOG_DBG("Directed control relay retransmit:0x%02x (count %u interval %ums)",
		xmit, BT_MESH_TRANSMIT_COUNT(xmit), BT_MESH_TRANSMIT_INT(xmit));

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		dfw_cfg_store();
	}

	return dfw_ctl_relay_retrans_get(model, ctx, buf);
}

const struct bt_mesh_model_op bt_mesh_dfw_cfg_srv_op[] = {
	{ OP_DIRECTED_CTL_GET,         BT_MESH_LEN_EXACT(2),   directed_ctl_get },
	{ OP_DIRECTED_CTL_SET,         BT_MESH_LEN_EXACT(7),   directed_ctl_set },
	{ OP_PATH_METRIC_GET,          BT_MESH_LEN_EXACT(2),   path_metric_get  },
	{ OP_PATH_METRIC_SET,          BT_MESH_LEN_EXACT(3),   path_metric_set  },
	{ OP_DISCOV_CAP_GET,           BT_MESH_LEN_EXACT(2),   discovery_capabilities_get   },
	{ OP_DISCOV_CAP_SET,           BT_MESH_LEN_EXACT(3),   discovery_capabilities_set   },
	{ OP_FW_TABLE_ADD,             BT_MESH_LEN_MIN(10),    forwarding_table_add },
	{ OP_FW_TABLE_DEL,             BT_MESH_LEN_EXACT(6),   forwarding_table_del },
	{ OP_FW_TABLE_DEP_ADD,         BT_MESH_LEN_MIN(10),    forwarding_table_dependents_add },
	{ OP_FW_TABLE_DEP_DEL,         BT_MESH_LEN_MIN(10),    forwarding_table_dependents_del },
	{ OP_FW_TABLE_ENT_COUNT_GET,   BT_MESH_LEN_MIN(2),     forwarding_table_entries_count_get},
	{ OP_FW_TABLE_DEP_GET,         BT_MESH_LEN_MIN(8),     forwarding_table_dependents_get   },
	{ OP_FW_TABLE_ENT_GET,         BT_MESH_LEN_MIN(4),     forwarding_table_entries_get   },
	{ OP_WANTED_LANES_GET,         BT_MESH_LEN_EXACT(2),   wanted_lanes_get },
	{ OP_WANTED_LANES_SET,         BT_MESH_LEN_EXACT(3),   wanted_lanes_set },
	{ OP_TWO_WAY_PATH_GET,         BT_MESH_LEN_EXACT(2),   two_way_path_get },
	{ OP_TWO_WAY_PATH_SET,         BT_MESH_LEN_EXACT(3),   two_way_path_set },
	{ OP_PATH_ECHO_INTV_GET,       BT_MESH_LEN_EXACT(2),   path_echo_interval_get },
	{ OP_PATH_ECHO_INTV_SET,       BT_MESH_LEN_EXACT(4),   path_echo_interval_set },
	{ OP_DIRECTED_NET_TRANSMIT_GET,  BT_MESH_LEN_EXACT(0), directed_network_transmit_get },
	{ OP_DIRECTED_NET_TRANSMIT_SET,  BT_MESH_LEN_EXACT(1), directed_network_transmit_set },
	{ OP_DIRECTED_RELAY_RETRANS_GET, BT_MESH_LEN_EXACT(0), directed_relay_retransmit_get },
	{ OP_DIRECTED_RELAY_RETRANS_SET, BT_MESH_LEN_EXACT(1), directed_relay_retransmit_set },
	{ OP_DIRECTED_PATH_GET,        BT_MESH_LEN_EXACT(0),   directed_path_get },
	{ OP_RSSI_THRESHOLD_GET,       BT_MESH_LEN_EXACT(0),   rssi_threshold_get },
	{ OP_RSSI_THRESHOLD_SET,       BT_MESH_LEN_EXACT(1),   rssi_threshold_set },
	{ OP_DIRECTED_PUB_POLICY_GET,  BT_MESH_LEN_MIN(4),     directed_publish_policy_get },
	{ OP_DIRECTED_PUB_POLICY_SET,  BT_MESH_LEN_MIN(5),     directed_publish_policy_set },
	{ OP_PATH_DISCOV_TIM_CTL_GET,  BT_MESH_LEN_EXACT(0),   path_discovery_timing_control_get },
	{ OP_PATH_DISCOV_TIM_CTL_SET,  BT_MESH_LEN_EXACT(5),   path_discovery_timing_control_set },
	{ OP_DIRECTED_CTL_NET_TRANS_GET, BT_MESH_LEN_EXACT(0), directed_ctl_network_transmit_get },
	{ OP_DIRECTED_CTL_NET_TRANS_SET, BT_MESH_LEN_EXACT(1), directed_ctl_network_transmit_set },
	{ OP_DIRECTED_CTL_RELAY_RETRANS_GET, BT_MESH_LEN_EXACT(0), dfw_ctl_relay_retrans_get },
	{ OP_DIRECTED_CTL_RELAY_RETRANS_SET, BT_MESH_LEN_EXACT(1), dfw_ctl_relay_retrans_set },

	BT_MESH_MODEL_OP_END,
};

static int dfw_cfg_srv_init(const struct bt_mesh_model *model)
{
	if (!bt_mesh_model_in_primary(model)) {
		LOG_ERR("Directed Forwarding Configuration Server only allowed in primary element");
		return -EINVAL;
	}

	/*
	 * Configuration Model security is device-key based and only the local
	 * device-key is allowed to access this model.
	 */
	model->keys[0] = BT_MESH_KEY_DEV_LOCAL;
	model->rt->flags |= BT_MESH_MOD_DEVKEY_ONLY;

	return 0;
}

const struct bt_mesh_model_cb bt_mesh_dfw_cfg_srv_cb = {
	.init = dfw_cfg_srv_init,
};

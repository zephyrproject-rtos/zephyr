/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_DSA_TAG_NETC_H_
#define ZEPHYR_DRIVERS_DSA_TAG_NETC_H_

struct dsa_tag_netc_data {
#ifdef CONFIG_NET_L2_PTP
	void (*twostep_timestamp_handler)(const struct dsa_switch_context *ctx, uint8_t ts_req_id,
					  uint64_t ts);
#endif
};

#define NETC_SWITCH_ETHER_TYPE 0x3AFD

enum netc_switch_tag_type {
	NETC_SWITCH_TAG_TYPE_FORWARD,
	NETC_SWITCH_TAG_TYPE_TO_PORT,
	NETC_SWITCH_TAG_TYPE_TO_HOST,
};

enum netc_switch_tag_subtype {
	NETC_SWITCH_TAG_SUBTYPE_TO_PORT_NO_TS = 0,
	NETC_SWITCH_TAG_SUBTYPE_TO_PORT_ONESTEP_TS,
	NETC_SWITCH_TAG_SUBTYPE_TO_PORT_TWOSTEP_TS,
	NETC_SWITCH_TAG_SUBTYPE_TO_PORT_ALL_TS,

	NETC_SWITCH_TAG_SUBTYPE_TO_HOST_NO_TS = 0,
	NETC_SWITCH_TAG_SUBTYPE_TO_HOST_RX_TS,
	NETC_SWITCH_TAG_SUBTYPE_TO_HOST_TX_TS,
};

#pragma pack(1)
/* Switch tag common part */
struct netc_switch_tag_common {
	uint16_t tpid;       /* Tag Protocol Identifier to identify the tag as an NXP switch tag. */
	uint16_t subtype: 4; /* Specific feature is based on tag type. */
	uint16_t type: 4;    /* Tag type. */
	uint16_t qv: 1;      /* QoS Valid. */
	uint16_t: 1;         /* Reserved. */
	uint16_t ipv: 3;     /* Internal Priority Value. */
	uint16_t: 1;         /* Reserved. */
	uint16_t dr: 2;      /* Drop Resilience. */
	uint8_t swtid: 3;    /* Switch ID. */
	uint8_t port: 5;     /* Switch port. */
};

/* Switch tag for forward */
struct netc_switch_tag_forward {
	struct netc_switch_tag_common common;
	uint8_t pm: 1; /* Port Masquerading. */
	uint8_t: 7;
};

/* Switch tag for to_port without timestamp */
struct netc_switch_tag_port_no_ts {
	struct netc_switch_tag_common common;
	uint8_t: 8;
};

/* Switch tag for to_port with one-step timestamp */
struct netc_switch_tag_port_one_step_ts {
	struct netc_switch_tag_common common;
	uint8_t: 8;
	uint32_t timestamp;
};

/* Switch tag for to_port without two-step timestamp */
struct netc_switch_tag_port_two_step_ts {
	struct netc_switch_tag_common common;
	uint8_t ts_req_id: 4;
	uint8_t: 4;
};

/* Switch tag for to_port with all timestamps */
struct netc_switch_tag_port_all_ts {
	struct netc_switch_tag_common common;
	uint8_t ts_req_id: 4;
	uint8_t: 4;
	uint32_t timestamp;
};

/* Switch tag for to_host */
struct netc_switch_tag_host {
	struct netc_switch_tag_common common;
	uint16_t: 4;
	uint16_t host_reason: 4;
};

/* Switch tag for to_host with timestamp */
struct netc_switch_tag_host_rx_ts {
	struct netc_switch_tag_common common;
	uint16_t: 4;
	uint16_t host_reason: 4;
	uint64_t timestamp;
};

/* Switch tag for to_host with timestamp */
struct netc_switch_tag_host_tx_ts {
	struct netc_switch_tag_common common;
	uint16_t ts_req_id: 4;
	uint16_t host_reason: 4;
	uint64_t timestamp;
};
#pragma pack()

#endif /* ZEPHYR_DRIVERS_DSA_TAG_NETC_H_ */

/*
 * Copyright (c) 2021 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <net/net_if.h>
#include <net/ptp_time.h>

#define PTP_MESSAGE_TYPE_SYNC 0x0
#define PTP_MESSAGE_TYPE_DELAY_REQ 0x1
#define PTP_MESSAGE_TYPE_FOLLOW_UP 0x8
#define PTP_MESSAGE_TYPE_DELAY_RESP 0x9

struct ptp_header {
	uint8_t message_type : 4;
	uint8_t major_sdo_id : 4;
	uint8_t version_ptp : 4;
	uint8_t minor_version_ptp : 4;
	uint16_t message_length;
	uint8_t domain_number;
	uint8_t minor_sdo_id;
	uint16_t flag_field;
	uint64_t correction_field;
	uint32_t message_type_specific;
	uint8_t source_port_identity[10];
	uint16_t sequence_id;
	uint8_t control_field;
	uint8_t log_message_interval;
} __packed;

struct ptp_timestamp {
	uint16_t second_high;
	uint32_t second_low;
	uint32_t nanosecond;
} __packed;

static inline void ptp_ts_wire_to_net(struct net_ptp_time *net, struct ptp_timestamp *wire)
{
	net->_sec.high = ntohs(wire->second_high);
	net->_sec.low = ntohl(wire->second_low);
	net->nanosecond = ntohl(wire->nanosecond);
}

static inline void ptp_ts_net_to_wire(struct ptp_timestamp *wire, struct net_ptp_time *net)
{
	wire->second_high = htons(net->_sec.high);
	wire->second_low = htonl(net->_sec.low);
	wire->nanosecond = htonl(net->nanosecond);
}

static inline uint64_t ptp_ts_net_to_ns(struct net_ptp_time *net)
{
	return net->second * NSEC_PER_SEC + net->nanosecond;
}

static inline uint64_t ptp_ts_net_to_us(struct net_ptp_time *net)
{
	return (net->second * USEC_PER_SEC) + (net->nanosecond / NSEC_PER_USEC);
}

int ptp_ts_get(struct net_ptp_time *net);

struct ptp_sync_body {
	struct ptp_timestamp origin_timestamp;
} __packed;

struct ptp_follow_up_body {
	struct ptp_timestamp precise_origin_timestamp;
} __packed;

struct ptp_delay_req_body {
	struct ptp_timestamp origin_timestamp;
} __packed;

struct ptp_delay_resp_body {
	struct ptp_timestamp receive_timestamp;
	uint8_t requesting_port_identity[10];
} __packed;

/* Initializes PTP slave, returns non-zero on error. */
int ptp_slave_init(struct net_if *iface);

/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/net/net_if.h>
#include <zephyr/net/ptp_time.h>
#include <posix/time.h>

#define PTP_MESSAGE_TYPE_SYNC 0x0
#define PTP_MESSAGE_TYPE_DELAY_REQ 0x1
#define PTP_MESSAGE_TYPE_FOLLOW_UP 0x8
#define PTP_MESSAGE_TYPE_DELAY_RESP 0x9
#define PTP_MESSAGE_TYPE_ANNOUNCE 0xb

#define PTP_FLAG_TWO_STEP (1 << 1)
#define PTP_FLAG_PTP_TIMESCALE (1 << 11)

typedef uint8_t ptp_clock_identity[8];

struct ptp_port_identity {
	ptp_clock_identity clock_identity;
	uint16_t port_number;
} __packed;

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
	struct ptp_port_identity source_port_identity;
	uint16_t sequence_id;
	uint8_t control_field;
	uint8_t log_message_interval;
} __packed;

struct ptp_timestamp {
	uint16_t second_high;
	uint32_t second_low;
	uint32_t nanosecond;
} __packed;

struct ptp_clock_quality {
	uint8_t clock_class;
	uint8_t clock_accuracy;
	uint16_t offset_scaled_log_var;
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

static inline uint64_t ptp_ts_net_to_ms(struct net_ptp_time *net)
{
	return (net->second * MSEC_PER_SEC) + (net->nanosecond / NSEC_PER_MSEC);
}

struct ptp_announce_body {
	struct ptp_timestamp origin_timestamp;
	int16_t current_utc_offset;
	uint8_t reserved;
	uint8_t grandmaster_priority1;
	struct ptp_clock_quality grandmaster_clock_quality;
	uint8_t grandmaster_priority2;
	ptp_clock_identity grandmaster_identity;
	uint16_t steps_removed;
	uint8_t time_source;
} __packed;

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
	struct ptp_port_identity requesting_port_identity;
} __packed;

#ifdef CONFIG_NET_PTP_MASTER
int ptp_master_start(struct net_if *iface);
int ptp_master_stop(struct net_if *iface);
#endif /* CONFIG_NET_PTP_MASTER */

#ifdef CONFIG_NET_PTP_SLAVE
int ptp_slave_set_delay_req_offset_ms(struct net_if *iface, int value);
int ptp_slave_get_delay_req_offset_ms(struct net_if *iface, int* value);
#endif /* CONFIG_NET_PTP_SLAVE */

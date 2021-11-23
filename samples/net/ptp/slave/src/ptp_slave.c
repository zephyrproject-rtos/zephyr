/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(ptp_slave, LOG_LEVEL_DBG);

#include <drivers/ptp_clock.h>
#include <net/ethernet.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/ptp.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#include "ptp_slave.h"

static const struct net_eth_addr ptp_mcast_eth_addr = { { 0x01, 0x1b, 0x19, 0x00, 0x00, 0x00 } };

struct ptp_slave_state {
	struct net_if *iface;
	const struct device *clk;
	struct ptp_port_identity port_identity;
	struct net_context *context;
	struct net_if_timestamp_cb delay_req_sent_cb;
	struct net_ptp_time t1;
	uint16_t t1_seq;
	struct net_ptp_time t2;
	struct net_ptp_time t3;
	uint16_t t3_seq;
	volatile bool t3_valid;
	struct net_ptp_time t4;
	int64_t roundtrip_latency_ns;
	int64_t last_offset_ns;
	double kp;
	double kd;
	struct ptp_header fup_ptp_hdr;
	int delay_req_offset_ms;
};

static struct ptp_slave_state state;

static void send_delay_req(struct k_work *work);
static void handle_delay_req_sent(struct net_pkt *pkt);
static void adjust_clock(void);

K_WORK_DELAYABLE_DEFINE(send_delay_req_work, send_delay_req);

static void handle_ptp_sync(struct net_pkt *pkt, struct ptp_header *ptp_hdr)
{
	/* set t1 (remote send timestamp) sequence ID */
	state.t1_seq = ptp_hdr->sequence_id;

	/* set t2 (reception timestamp) */
	struct net_ptp_time *ts = net_pkt_timestamp(pkt);

	memcpy(&state.t2, ts, sizeof(state.t2));
}

static void handle_ptp_follow_up(struct net_pkt *pkt, struct ptp_header *ptp_hdr)
{
	if (ptp_hdr->sequence_id != state.t1_seq) {
		LOG_WRN("Mismatched FOLLOW_UP (%u) received for SYNC (%u)", ptp_hdr->sequence_id,
			state.t1_seq);
		return;
	}

	/* set t1 (remote send timestamp) */
	struct ptp_follow_up_body ptp_body;

	net_pkt_read(pkt, &ptp_body, sizeof(ptp_body));
	ptp_ts_wire_to_net(&state.t1, &ptp_body.precise_origin_timestamp);

	/* send delay request message */
	memcpy(&state.fup_ptp_hdr, ptp_hdr, sizeof(state.fup_ptp_hdr));
	k_work_schedule(&send_delay_req_work, K_MSEC(state.delay_req_offset_ms));
}

static void send_delay_req(struct k_work *work)
{
	/* prepare body */
	struct ptp_delay_req_body ptp_body;

	memset(&ptp_body, 0, sizeof(ptp_body));

	/* prepare header */
	struct ptp_header ptp_hdr;

	memset(&ptp_hdr, 0, sizeof(ptp_hdr));
	ptp_hdr.message_type = PTP_MESSAGE_TYPE_DELAY_REQ;
	ptp_hdr.major_sdo_id = state.fup_ptp_hdr.major_sdo_id;
	ptp_hdr.version_ptp = 2;
	ptp_hdr.minor_version_ptp = 0;
	ptp_hdr.message_length = sizeof(ptp_hdr) + sizeof(ptp_body);
	ptp_hdr.domain_number = state.fup_ptp_hdr.domain_number;
	ptp_hdr.minor_sdo_id = state.fup_ptp_hdr.minor_sdo_id;
	memcpy(&ptp_hdr.source_port_identity, &state.port_identity,
		sizeof(ptp_hdr.source_port_identity));
	ptp_hdr.sequence_id = ++state.t3_seq;
	ptp_hdr.log_message_interval = 0x7f;

	/* allocate packet with PTP settings */
	struct net_pkt *pkt = net_pkt_alloc_with_buffer(state.iface, ptp_hdr.message_length,
							AF_UNSPEC, 0, K_NO_WAIT);

	net_pkt_set_ptp(pkt, true);
	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	/* set src mac addr */
	struct net_linkaddr *iface_addr = net_if_get_link_addr(state.iface);

	net_pkt_lladdr_src(pkt)->addr = iface_addr->addr;
	net_pkt_lladdr_src(pkt)->len = iface_addr->len;

	/* set dst mac addr */
	net_pkt_lladdr_dst(pkt)->addr = (uint8_t *)&ptp_mcast_eth_addr;
	net_pkt_lladdr_dst(pkt)->len = sizeof(ptp_mcast_eth_addr);

	/* write packet PTP data */
	net_pkt_write(pkt, &ptp_hdr, sizeof(ptp_hdr));
	net_pkt_write(pkt, &ptp_body, sizeof(ptp_body));

	/* register to receive send callback */
	state.t3_valid = false;
	net_pkt_ref(pkt);
	net_if_register_timestamp_cb(&state.delay_req_sent_cb, pkt, state.iface,
		handle_delay_req_sent);

	/* enqueue for sending */
	net_if_queue_tx(state.iface, pkt);
}

static void handle_delay_req_sent(struct net_pkt *pkt)
{
	struct net_ptp_time *ts = net_pkt_timestamp(pkt);

	memcpy(&state.t3, ts, sizeof(state.t3));
	state.t3_valid = true;

	net_if_unregister_timestamp_cb(&state.delay_req_sent_cb);
	net_pkt_unref(pkt);
}

static void handle_ptp_delay_resp(struct net_pkt *pkt, struct ptp_header *ptp_hdr)
{
	struct ptp_delay_resp_body ptp_body;
	int ret;

	net_pkt_read(pkt, &ptp_body, sizeof(ptp_body));

	if (memcmp(&state.port_identity, &ptp_body.requesting_port_identity,
			sizeof(state.port_identity))) {
		/* not for us */
		return;
	}

	if (ptp_hdr->sequence_id != state.t3_seq) {
		LOG_WRN("Mismatched DELAY_RESP (%u) received for DELAY_REQ (%u)",
			ptp_hdr->sequence_id, state.t3_seq);
		return;
	}

	/* set t4 (remote receive timestamp) */
	ptp_ts_wire_to_net(&state.t4, &ptp_body.receive_timestamp);

	/*
	 * If t3 is not yet valid because of race between this callback and
	 * handle_delay_req_sent, wait for it.
	 */
	while (!state.t3_valid) {
		k_yield();
	}

	/* store packet roundtrip latency */
	struct net_ptp_time current_t;

	ret = ptp_clock_get(state.clk, &current_t);
	if (ret < 0) {
		LOG_WRN("ptp_clock_get() failed: %d", ret);
		return;
	}
	state.roundtrip_latency_ns = ptp_ts_net_to_ns(&current_t) - ptp_ts_net_to_ns(&state.t3);

	/* we got everything we need - adjust clock */
	adjust_clock();
}

static void adjust_clock(void)
{
	int32_t t1_t4_delay_ns = ptp_ts_net_to_ns(&state.t4) - ptp_ts_net_to_ns(&state.t1);
	int32_t t2_t3_delay_ns = ptp_ts_net_to_ns(&state.t3) - ptp_ts_net_to_ns(&state.t2);
	int32_t delay_ns = (t1_t4_delay_ns - t2_t3_delay_ns) / 2;
	int64_t t1_t4_avg_ns = (ptp_ts_net_to_ns(&state.t1) + ptp_ts_net_to_ns(&state.t4)) / 2;
	int64_t t2_t3_avg_ns = (ptp_ts_net_to_ns(&state.t2) + ptp_ts_net_to_ns(&state.t3)) / 2;
	int64_t offset_ns = t1_t4_avg_ns - t2_t3_avg_ns;
	int ret;

	LOG_DBG("t1: %lu.%09u, t2: %lu.%09u, t3: %lu.%09u, t4: %lu.%09u, "
		"delay: %d, offset: %c%lu.%09u",
		(unsigned long)state.t1.second, state.t1.nanosecond,
		(unsigned long)state.t2.second, state.t2.nanosecond,
		(unsigned long)state.t3.second, state.t3.nanosecond,
		(unsigned long)state.t4.second, state.t4.nanosecond,
		delay_ns, offset_ns >= 0 ? '+' : '-', labs(offset_ns / NSEC_PER_SEC),
		abs(offset_ns % NSEC_PER_SEC));

	if (llabs(offset_ns) > NSEC_PER_SEC / 2) {
		/*
		 * We'll assume that good estimate of current time is:
		 * t4 + roundtrip_latency_ns - delay_ns
		 */
		int64_t current_ns = ptp_ts_net_to_ns(&state.t4) + state.roundtrip_latency_ns
			- delay_ns;
		struct net_ptp_time current_t;

		current_t.second = current_ns / NSEC_PER_SEC;
		current_t.nanosecond = current_ns % NSEC_PER_SEC;

		/* time delta is too large - jump */
		ret = ptp_clock_set(state.clk, &current_t);
		if (ret < 0) {
			LOG_WRN("ptp_clock_set() failed: %d", ret);
			return;
		}
		LOG_INF("PTP clock set: %lu.%09u", (unsigned long)current_t.second,
			current_t.nanosecond);
		state.last_offset_ns = 0;
		return;
	}

	/* compensate the offset using a PD controller */
	double rate = 1.0 + state.kp * (double)offset_ns / NSEC_PER_SEC +
		state.kd * (double)(offset_ns - state.last_offset_ns) / NSEC_PER_SEC;
	ret = ptp_clock_rate_adjust(state.clk, rate);
	if (ret == -EINVAL) {
		/*
		 * Proposed rate is outside of available bounds, we will find the closest
		 * acceptable rate.  Will make sure to avoid an infinite loop by having
		 * 0.5 .. 2.0 guardrails for sanity.
		 */
		LOG_WRN("PTP clock rate %d.%03d not accepted, searching for acceptable rate "
			"for offset: %c%lu.%09u", (int)rate, abs(1000 * (rate - 1.0)),
			offset_ns >= 0 ? '+' : '-', labs(offset_ns / NSEC_PER_SEC),
			abs(offset_ns % NSEC_PER_SEC));

		double rate_adjustment = rate > 1.0 ? 0.99 : 1.01;

		while (rate > 0.5 && rate < 2.0) {
			rate *= rate_adjustment;
			ret = ptp_clock_rate_adjust(state.clk, rate);
			if (ret < 0 && ret != -EINVAL) {
				LOG_WRN("ptp_clock_rate_adjust() failed: %d", ret);
				return;
			}
			LOG_INF("Accepted PTP clock rate %d.%03d.", (int)rate,
				abs(1000 * (rate - 1.0)));
			break;
		}
		if (!(rate > 0.5 && rate < 2.0)) {
			LOG_ERR("Could not find acceptable PTP clock rate.");
			return;
		}
	} else if (ret < 0) {
		LOG_WRN("ptp_clock_rate_adjust() failed: %d", ret);
		return;
	}
	state.last_offset_ns = offset_ns;
}

static void pkt_received(struct net_context *context, struct net_pkt *pkt,
			 union net_ip_header *ip_hdr, union net_proto_header *proto_hdr,
			 int status, void *user_data)
{
	if (ntohs(NET_ETH_HDR(pkt)->type) != NET_ETH_PTYPE_PTP) {
		goto done;
	}

	/* skip ethernet header */
	net_pkt_skip(pkt, sizeof(struct net_eth_hdr));

	/* read PTP header */
	struct ptp_header ptp_hdr;

	net_pkt_read(pkt, &ptp_hdr, sizeof(ptp_hdr));

	if (ptp_hdr.version_ptp != 2 || ptp_hdr.minor_version_ptp != 0) {
		LOG_WRN("Unsupposed PTP v%d.%d packet ignored", ptp_hdr.version_ptp,
			ptp_hdr.minor_version_ptp);
		goto done;
	}

	switch (ptp_hdr.message_type) {
	case PTP_MESSAGE_TYPE_SYNC:
		handle_ptp_sync(pkt, &ptp_hdr);
		break;
	case PTP_MESSAGE_TYPE_FOLLOW_UP:
		handle_ptp_follow_up(pkt, &ptp_hdr);
		break;
	case PTP_MESSAGE_TYPE_DELAY_RESP:
		handle_ptp_delay_resp(pkt, &ptp_hdr);
		break;
	}

done:
	net_pkt_unref(pkt);
}

int ptp_slave_clk_get(struct net_ptp_time *net)
{
	return ptp_clock_get(state.clk, net);
}

int ptp_slave_init(struct net_if *iface, int delay_req_offset_ms)
{
	int ret;

	memset(&state, 0, sizeof(state));

	state.iface = iface;

	state.clk = net_eth_get_ptp_clock(iface);
	if (!state.clk) {
		return -ENODEV;
	}
	ret = device_is_ready(state.clk);
	if (ret < 0) {
		return ret;
	}

	struct net_linkaddr *iface_addr = net_if_get_link_addr(state.iface);

	memcpy(state.port_identity.clock_identity, iface_addr->addr, iface_addr->len);

	/* pid parameters determined experimentally */
	state.kp = 0.3;
	state.kd = 0.7;

	/* delay req offset */
	state.delay_req_offset_ms = delay_req_offset_ms;

	return 0;
}

int ptp_slave_start(void)
{
	int ret;
	struct sockaddr_ll dst;

	ret = net_context_get(AF_PACKET, SOCK_RAW, ETH_P_ALL, &state.context);
	if (ret < 0) {
		return ret;
	}

	dst.sll_ifindex = net_if_get_by_iface(state.iface);
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(NET_ETH_PTYPE_PTP);

	ret = net_context_bind(state.context, (const struct sockaddr *)&dst, sizeof(dst));
	if (ret < 0) {
		return ret;
	}

	ret = net_context_recv(state.context, pkt_received, K_NO_WAIT, NULL);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*
 * Copyright (c) 2022 Carbon Robotics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(ptp_master, CONFIG_NET_PTP_LOG_LEVEL);

#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ptp.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#define NET_PTP_MASTER_STACK_SIZE 1024

static const struct net_eth_addr ptp_mcast_eth_addr = { { 0x01, 0x1b, 0x19, 0x00, 0x00, 0x00 } };

struct ptp_master_state {
	struct net_if *iface;
	const struct device *clk;
	struct ptp_port_identity port_identity;
	struct net_context *context;
	struct net_pkt *sync_pkt;
	struct net_if_timestamp_cb sync_sent_cb;
	uint16_t announce_seq_id;
	uint16_t sync_seq_id;
	uint8_t domain_number;
	uint8_t log_announce_interval;
	uint8_t log_sync_interval;
	uint8_t log_min_delay_req_interval;
	bool running;
};

static struct ptp_master_state state;

K_KERNEL_STACK_DEFINE(ptp_announce_sender_thread_stack, NET_PTP_MASTER_STACK_SIZE);
static struct k_thread ptp_announce_sender_thread;

K_KERNEL_STACK_DEFINE(ptp_sync_sender_thread_stack, NET_PTP_MASTER_STACK_SIZE);
static struct k_thread ptp_sync_sender_thread;

static void handle_ptp_sync_sent(struct net_pkt *pkt);
static void send_ptp_follow_up(struct net_ptp_time *ts);

static void send_ptp_announce(void)
{
	/* prepare body */
	struct ptp_announce_body ptp_body;

	memset(&ptp_body, 0, sizeof(ptp_body));
	ptp_body.grandmaster_clock_quality.clock_class = 248;
	ptp_body.grandmaster_clock_quality.clock_accuracy = 0xfe;
	ptp_body.grandmaster_clock_quality.offset_scaled_log_var = 0xffff;
	memcpy(&ptp_body.grandmaster_identity, &state.port_identity.clock_identity,
		sizeof(ptp_body.grandmaster_identity));
	ptp_body.time_source = 0xa0;

	/* prepare header */
	struct ptp_header ptp_hdr;

	memset(&ptp_hdr, 0, sizeof(ptp_hdr));
	ptp_hdr.message_type = PTP_MESSAGE_TYPE_ANNOUNCE;
	ptp_hdr.version_ptp = 2;
	ptp_hdr.minor_version_ptp = 0;
	ptp_hdr.message_length = sizeof(ptp_hdr) + sizeof(ptp_body);
	ptp_hdr.domain_number = state.domain_number;
	ptp_hdr.flag_field = PTP_FLAG_PTP_TIMESCALE;
	memcpy(&ptp_hdr.source_port_identity, &state.port_identity,
		sizeof(ptp_hdr.source_port_identity));
	ptp_hdr.sequence_id = ++state.announce_seq_id;
	ptp_hdr.log_message_interval = state.log_announce_interval;

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

	/* enqueue for sending */
	net_if_queue_tx(state.iface, pkt);
}

static void send_ptp_sync(void)
{
	/* prepare body */
	struct ptp_sync_body ptp_body;

	memset(&ptp_body, 0, sizeof(ptp_body));

	/* prepare header */
	struct ptp_header ptp_hdr;

	memset(&ptp_hdr, 0, sizeof(ptp_hdr));
	ptp_hdr.message_type = PTP_MESSAGE_TYPE_SYNC;
	ptp_hdr.version_ptp = 2;
	ptp_hdr.minor_version_ptp = 0;
	ptp_hdr.message_length = sizeof(ptp_hdr) + sizeof(ptp_body);
	ptp_hdr.domain_number = state.domain_number;
	ptp_hdr.flag_field = PTP_FLAG_TWO_STEP;
	memcpy(&ptp_hdr.source_port_identity, &state.port_identity,
		sizeof(ptp_hdr.source_port_identity));
	ptp_hdr.sequence_id = ++state.sync_seq_id;
	ptp_hdr.log_message_interval = state.log_sync_interval;

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

	if (state.sync_pkt != NULL) {
		/* were not able to send last sync packet, unregister callback and unref packet */
		net_if_unregister_timestamp_cb(&state.sync_sent_cb);
		net_pkt_unref(state.sync_pkt);
		state.sync_pkt = NULL;
	}

	state.sync_pkt = pkt;
	net_pkt_ref(pkt);
	net_if_register_timestamp_cb(&state.sync_sent_cb, pkt, state.iface, handle_ptp_sync_sent);

	/* enqueue for sending */
	net_if_queue_tx(state.iface, pkt);
}

static void handle_ptp_sync_sent(struct net_pkt *pkt)
{
	struct net_ptp_time *ts = net_pkt_timestamp(pkt);

	send_ptp_follow_up(ts);

	net_if_unregister_timestamp_cb(&state.sync_sent_cb);
	net_pkt_unref(pkt);
	state.sync_pkt = NULL;
}

static void send_ptp_follow_up(struct net_ptp_time *ts)
{
	/* prepare body */
	struct ptp_follow_up_body ptp_body;

	memset(&ptp_body, 0, sizeof(ptp_body));
	ptp_ts_net_to_wire(&ptp_body.precise_origin_timestamp, ts);

	/* prepare header */
	struct ptp_header ptp_hdr;

	memset(&ptp_hdr, 0, sizeof(ptp_hdr));
	ptp_hdr.message_type = PTP_MESSAGE_TYPE_FOLLOW_UP;
	ptp_hdr.version_ptp = 2;
	ptp_hdr.minor_version_ptp = 0;
	ptp_hdr.message_length = sizeof(ptp_hdr) + sizeof(ptp_body);
	ptp_hdr.domain_number = state.domain_number;
	memcpy(&ptp_hdr.source_port_identity, &state.port_identity,
		sizeof(ptp_hdr.source_port_identity));
	ptp_hdr.sequence_id = state.sync_seq_id;
	ptp_hdr.log_message_interval = state.log_sync_interval;

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

	/* enqueue for sending */
	net_if_queue_tx(state.iface, pkt);
}

static void handle_ptp_delay_req(struct net_pkt *req_pkt, struct ptp_header *req_ptp_hdr)
{
	struct net_ptp_time *ts = net_pkt_timestamp(req_pkt);

	/* prepare body */
	struct ptp_delay_resp_body ptp_body;

	memset(&ptp_body, 0, sizeof(ptp_body));
	ptp_ts_net_to_wire(&ptp_body.receive_timestamp, ts);
	memcpy(&ptp_body.requesting_port_identity, &req_ptp_hdr->source_port_identity,
		sizeof(ptp_body.requesting_port_identity));

	/* prepare header */
	struct ptp_header ptp_hdr;

	memset(&ptp_hdr, 0, sizeof(ptp_hdr));
	ptp_hdr.message_type = PTP_MESSAGE_TYPE_DELAY_RESP;
	ptp_hdr.version_ptp = 2;
	ptp_hdr.minor_version_ptp = 0;
	ptp_hdr.message_length = sizeof(ptp_hdr) + sizeof(ptp_body);
	ptp_hdr.domain_number = state.domain_number;
	memcpy(&ptp_hdr.source_port_identity, &state.port_identity,
		sizeof(ptp_hdr.source_port_identity));
	ptp_hdr.sequence_id = req_ptp_hdr->sequence_id;
	ptp_hdr.log_message_interval = state.log_min_delay_req_interval;

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

	/* enqueue for sending */
	net_if_queue_tx(state.iface, pkt);
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
		LOG_WRN("Unsupported PTP v%d.%d packet ignored", ptp_hdr.version_ptp,
			ptp_hdr.minor_version_ptp);
		goto done;
	}

	switch (ptp_hdr.message_type) {
	case PTP_MESSAGE_TYPE_DELAY_REQ:
		handle_ptp_delay_req(pkt, &ptp_hdr);
		break;
	}

done:
	net_pkt_unref(pkt);
}

static void ptp_announce_sender(void)
{
	for (;;) {
		if (state.running) {
			send_ptp_announce();
		}
		k_msleep(1000 * (1 << state.log_announce_interval));
	}
}

static void ptp_sync_sender(void)
{
	for (;;) {
		if (state.running) {
			send_ptp_sync();
		}
		k_msleep(1000 * (1 << state.log_sync_interval));
	}
}

int ptp_master_start(struct net_if *iface)
{
	if (state.iface != iface) {
		return -EINVAL;
	}
	state.running = true;
	return 0;
}

int ptp_master_stop(struct net_if *iface)
{
	if (state.iface != iface) {
		return -EINVAL;
	}
	state.running = false;
	return 0;
}

void net_ptp_master_init(void)
{
	int ret;
	struct sockaddr_ll dst;

	state.iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (state.iface == NULL) {
		LOG_ERR("Network interface for PTP not found.");
		return;
	}

	state.clk = net_eth_get_ptp_clock(state.iface);
	if (!device_is_ready(state.clk)) {
		LOG_ERR("Network interface for PTP does not support PTP clock.");
		return;
	}

	struct net_linkaddr *iface_addr = net_if_get_link_addr(state.iface);

	memcpy(state.port_identity.clock_identity, iface_addr->addr, iface_addr->len);

	/* default linuxptp settings */
	state.domain_number = 0;
	state.log_announce_interval = 1;
	state.log_sync_interval = 0;
	state.log_min_delay_req_interval = 0;

	ret = net_context_get(AF_PACKET, SOCK_RAW, ETH_P_ALL, &state.context);
	if (ret < 0) {
		LOG_ERR("net_context_get() failed: %d", ret);
		return;
	}

	dst.sll_ifindex = net_if_get_by_iface(state.iface);
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(NET_ETH_PTYPE_PTP);

	ret = net_context_bind(state.context, (const struct sockaddr *)&dst, sizeof(dst));
	if (ret < 0) {
		LOG_ERR("net_context_bind() failed: %d", ret);
		return;
	}

	ret = net_context_recv(state.context, pkt_received, K_NO_WAIT, NULL);
	if (ret < 0) {
		LOG_ERR("net_context_recv() failed: %d", ret);
		return;
	}

	k_thread_create(&ptp_announce_sender_thread, ptp_announce_sender_thread_stack,
		K_KERNEL_STACK_SIZEOF(ptp_announce_sender_thread_stack),
		(k_thread_entry_t)ptp_announce_sender,
		NULL, NULL, NULL, K_PRIO_COOP(5), 0, K_NO_WAIT);
	k_thread_name_set(&ptp_announce_sender_thread, "announce_sender");

	k_thread_create(&ptp_sync_sender_thread, ptp_sync_sender_thread_stack,
		K_KERNEL_STACK_SIZEOF(ptp_sync_sender_thread_stack),
		(k_thread_entry_t)ptp_sync_sender,
		NULL, NULL, NULL, K_PRIO_COOP(5), 0, K_NO_WAIT);
	k_thread_name_set(&ptp_sync_sender_thread, "sync_sender");
}

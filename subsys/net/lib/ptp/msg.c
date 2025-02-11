/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_msg, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/udp.h>
#include <zephyr/net/ptp.h>

#include "clock.h"
#include "msg.h"
#include "port.h"
#include "tlv.h"
#include "transport.h"

static struct k_mem_slab msg_slab;

K_MEM_SLAB_DEFINE_STATIC(msg_slab, sizeof(struct ptp_msg), CONFIG_PTP_MSG_POLL_SIZE, 8);

static const char *msg_type_str(struct ptp_msg *msg)
{
	switch (ptp_msg_type(msg)) {
	case PTP_MSG_SYNC:
		return "Sync";
	case PTP_MSG_DELAY_REQ:
		return "Delay_Req";
	case PTP_MSG_PDELAY_REQ:
		return "Pdelay_Req";
	case PTP_MSG_PDELAY_RESP:
		return "Pdelay_Resp";
	case PTP_MSG_FOLLOW_UP:
		return "Follow_Up";
	case PTP_MSG_DELAY_RESP:
		return "Delay_Resp";
	case PTP_MSG_PDELAY_RESP_FOLLOW_UP:
		return "Pdelay_Resp_Follow_Up";
	case PTP_MSG_ANNOUNCE:
		return "Announce";
	case PTP_MSG_SIGNALING:
		return "Signaling";
	case PTP_MSG_MANAGEMENT:
		return "Management";
	default:
		return "Not recognized";
	}
}

static void msg_timestamp_post_recv(struct ptp_msg *msg, struct ptp_timestamp *ts)
{
	msg->timestamp.protocol._sec.high = ntohs(ts->seconds_high);
	msg->timestamp.protocol._sec.low = ntohl(ts->seconds_low);
	msg->timestamp.protocol.nanosecond = ntohl(ts->nanoseconds);
}

static void msg_timestamp_pre_send(struct ptp_timestamp *ts)
{
	ts->seconds_high = htons(ts->seconds_high);
	ts->seconds_low = htonl(ts->seconds_low);
	ts->nanoseconds = htonl(ts->nanoseconds);
}

static void msg_port_id_post_recv(struct ptp_port_id *port_id)
{
	port_id->port_number = ntohs(port_id->port_number);
}

static void msg_port_id_pre_send(struct ptp_port_id *port_id)
{
	port_id->port_number = htons(port_id->port_number);
}

static int msg_header_post_recv(struct ptp_header *header)
{
	if ((header->version & 0xF) != PTP_MAJOR_VERSION) {
		/* Incompatible protocol version */
		return -1;
	}

	header->msg_length = ntohs(header->msg_length);
	header->correction = ntohll(header->correction);
	header->sequence_id = ntohs(header->sequence_id);

	msg_port_id_post_recv(&header->src_port_id);

	return 0;
}

static void msg_header_pre_send(struct ptp_header *header)
{
	header->msg_length = htons(header->msg_length);
	header->correction = htonll(header->correction);
	header->sequence_id = htons(header->sequence_id);

	msg_port_id_pre_send(&header->src_port_id);
}

static uint8_t *msg_suffix(struct ptp_msg *msg)
{
	uint8_t *suffix = NULL;

	switch (ptp_msg_type(msg)) {
	case PTP_MSG_SYNC:
		suffix = msg->sync.suffix;
		break;
	case PTP_MSG_DELAY_REQ:
		suffix = msg->delay_req.suffix;
		break;
	case PTP_MSG_PDELAY_REQ:
		suffix = msg->pdelay_req.suffix;
		break;
	case PTP_MSG_PDELAY_RESP:
		suffix = msg->pdelay_resp.suffix;
		break;
	case PTP_MSG_FOLLOW_UP:
		suffix = msg->follow_up.suffix;
		break;
	case PTP_MSG_DELAY_RESP:
		suffix = msg->delay_resp.suffix;
		break;
	case PTP_MSG_PDELAY_RESP_FOLLOW_UP:
		suffix = msg->pdelay_resp_follow_up.suffix;
		break;
	case PTP_MSG_ANNOUNCE:
		suffix = msg->announce.suffix;
		break;
	case PTP_MSG_SIGNALING:
		suffix = msg->signaling.suffix;
		break;
	case PTP_MSG_MANAGEMENT:
		suffix = msg->management.suffix;
		break;
	}

	return suffix;
}

static int msg_tlv_post_recv(struct ptp_msg *msg, int length)
{
	int suffix_len = 0, ret = 0;
	struct ptp_tlv_container *tlv_container;
	uint8_t *suffix = msg_suffix(msg);

	if (!suffix) {
		LOG_DBG("No TLV attached to the message");
		return 0;
	}

	while (length >= sizeof(struct ptp_tlv)) {
		tlv_container = ptp_tlv_alloc();
		if (!tlv_container) {
			return -ENOMEM;
		}

		tlv_container->tlv = (struct ptp_tlv *)suffix;
		tlv_container->tlv->type = ntohs(tlv_container->tlv->type);
		tlv_container->tlv->length = ntohs(tlv_container->tlv->length);

		if (tlv_container->tlv->length % 2) {
			/* IEEE 1588-2019 Section 5.3.8 - length is an even number */
			LOG_ERR("Incorrect length of TLV");
			ptp_tlv_free(tlv_container);
			return -EBADMSG;
		}

		length -= sizeof(struct ptp_tlv);
		suffix += sizeof(struct ptp_tlv);
		suffix_len += sizeof(struct ptp_tlv);

		if (tlv_container->tlv->length > length) {
			LOG_ERR("Incorrect length of TLV");
			ptp_tlv_free(tlv_container);
			return -EBADMSG;
		}

		length -= tlv_container->tlv->length;
		suffix += tlv_container->tlv->length;
		suffix_len += tlv_container->tlv->length;

		ret = ptp_tlv_post_recv(tlv_container->tlv);
		if (ret) {
			ptp_tlv_free(tlv_container);
			return ret;
		}

		sys_slist_append(&msg->tlvs, &tlv_container->node);
	}

	return suffix_len;
}

static void msg_tlv_free(struct ptp_msg *msg)
{
	struct ptp_tlv_container *tlv_container;

	for (sys_snode_t *iter = sys_slist_get(&msg->tlvs);
	     iter;
	     iter = sys_slist_get(&msg->tlvs)) {
		tlv_container = CONTAINER_OF(iter, struct ptp_tlv_container, node);
		ptp_tlv_free(tlv_container);
	}
}

static void msg_tlv_pre_send(struct ptp_msg *msg)
{
	struct ptp_tlv_container *tlv_container;

	SYS_SLIST_FOR_EACH_CONTAINER(&msg->tlvs, tlv_container, node) {
		ptp_tlv_pre_send(tlv_container->tlv);
	}

	/* No need to track TLVs attached to the message. */
	msg_tlv_free(msg);
}

struct ptp_msg *ptp_msg_alloc(void)
{
	struct ptp_msg *msg = NULL;
	int ret = k_mem_slab_alloc(&msg_slab, (void **)&msg, K_FOREVER);

	if (ret) {
		LOG_ERR("Couldn't allocate memory for the message");
		return NULL;
	}

	memset(msg, 0, sizeof(*msg));
	sys_slist_init(&msg->tlvs);
	atomic_inc(&msg->ref);

	return msg;
}

void ptp_msg_unref(struct ptp_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);

	atomic_t val = atomic_dec(&msg->ref);

	if (val > 1) {
		return;
	}

	msg_tlv_free(msg);
	k_mem_slab_free(&msg_slab, (void *)msg);
}

void ptp_msg_ref(struct ptp_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);

	atomic_inc(&msg->ref);
}

enum ptp_msg_type ptp_msg_type(const struct ptp_msg *msg)
{
	return (enum ptp_msg_type)(msg->header.type_major_sdo_id & 0xF);
}

struct ptp_msg *ptp_msg_from_pkt(struct net_pkt *pkt)
{
	static const size_t eth_hdr_len = IS_ENABLED(CONFIG_NET_VLAN) ?
					  sizeof(struct net_eth_vlan_hdr) :
					  sizeof(struct net_eth_hdr);
	struct net_udp_hdr *hdr;
	struct ptp_msg *msg;
	int port, payload;

	if (pkt->buffer->len == eth_hdr_len) {
		/* Packet contain Ethernet header at the beginning. */
		struct net_buf *buf;

		/* remove packet temporarily. */
		buf = pkt->buffer;
		pkt->buffer = buf->frags;

		hdr = net_udp_get_hdr(pkt, NULL);

		/* insert back temporarily femoved frag. */
		net_pkt_frag_insert(pkt, buf);
	} else {
		hdr = net_udp_get_hdr(pkt, NULL);
	}

	if (!hdr) {
		LOG_ERR("Couldn't retrieve UDP header from the net packet");
		return NULL;
	}

	payload = ntohs(hdr->len) - NET_UDPH_LEN;
	port = ntohs(hdr->dst_port);

	if (port != PTP_SOCKET_PORT_EVENT && port != PTP_SOCKET_PORT_GENERAL) {
		LOG_ERR("Couldn't retrieve PTP message from the net packet");
		return NULL;
	}

	msg = (struct ptp_msg *)((uintptr_t)hdr + NET_UDPH_LEN);

	if (payload == ntohs(msg->header.msg_length)) {
		return msg;
	}

	return NULL;
}

void ptp_msg_pre_send(struct ptp_msg *msg)
{
	int64_t current;

	msg_header_pre_send(&msg->header);

	switch (ptp_msg_type(msg)) {
	case PTP_MSG_SYNC:
		break;
	case PTP_MSG_DELAY_REQ:
		current = k_uptime_get();

		msg->timestamp.host.second = (uint64_t)(current / MSEC_PER_SEC);
		msg->timestamp.host.nanosecond = (current % MSEC_PER_SEC) * NSEC_PER_MSEC;
		break;
	case PTP_MSG_PDELAY_REQ:
		break;
	case PTP_MSG_PDELAY_RESP:
		msg_timestamp_pre_send(&msg->pdelay_resp.req_receipt_timestamp);
		msg_port_id_pre_send(&msg->pdelay_resp.req_port_id);
		break;
	case PTP_MSG_FOLLOW_UP:
		break;
	case PTP_MSG_DELAY_RESP:
		msg_timestamp_pre_send(&msg->delay_resp.receive_timestamp);
		msg_port_id_pre_send(&msg->delay_resp.req_port_id);
		break;
	case PTP_MSG_PDELAY_RESP_FOLLOW_UP:
		msg_timestamp_pre_send(&msg->pdelay_resp_follow_up.resp_origin_timestamp);
		msg_port_id_pre_send(&msg->pdelay_resp_follow_up.req_port_id);
		break;
	case PTP_MSG_ANNOUNCE:
		msg->announce.current_utc_offset = htons(msg->announce.current_utc_offset);
		msg->announce.gm_clk_quality.offset_scaled_log_variance =
			htons(msg->announce.gm_clk_quality.offset_scaled_log_variance);
		msg->announce.steps_rm = htons(msg->announce.steps_rm);
		break;
	case PTP_MSG_SIGNALING:
		msg_port_id_pre_send(&msg->signaling.target_port_id);
		break;
	case PTP_MSG_MANAGEMENT:
		msg_port_id_pre_send(&msg->management.target_port_id);
		break;
	}

	msg_tlv_pre_send(msg);
}

int ptp_msg_post_recv(struct ptp_port *port, struct ptp_msg *msg, int cnt)
{
	static const int msg_size[] = {
		[PTP_MSG_SYNC]		        = sizeof(struct ptp_sync_msg),
		[PTP_MSG_DELAY_REQ]	        = sizeof(struct ptp_delay_req_msg),
		[PTP_MSG_PDELAY_REQ]	        = sizeof(struct ptp_pdelay_req_msg),
		[PTP_MSG_PDELAY_RESP]	        = sizeof(struct ptp_pdelay_resp_msg),
		[PTP_MSG_FOLLOW_UP]	        = sizeof(struct ptp_follow_up_msg),
		[PTP_MSG_DELAY_RESP]	        = sizeof(struct ptp_delay_resp_msg),
		[PTP_MSG_PDELAY_RESP_FOLLOW_UP] = sizeof(struct ptp_pdelay_resp_follow_up_msg),
		[PTP_MSG_ANNOUNCE]	        = sizeof(struct ptp_announce_msg),
		[PTP_MSG_SIGNALING]	        = sizeof(struct ptp_signaling_msg),
		[PTP_MSG_MANAGEMENT]	        = sizeof(struct ptp_management_msg),
	};
	enum ptp_msg_type type = ptp_msg_type(msg);
	int64_t current;
	int tlv_len;

	if (msg_size[type] > cnt) {
		LOG_ERR("Received message with incorrect length");
		return -EBADMSG;
	}

	if (msg_header_post_recv(&msg->header)) {
		LOG_ERR("Received message incomplient with supported PTP version");
		return -EBADMSG;
	}

	LOG_DBG("Port %d received %s message", port->port_ds.id.port_number, msg_type_str(msg));

	switch (type) {
	case PTP_MSG_SYNC:
		msg_timestamp_post_recv(msg, &msg->sync.origin_timestamp);
		break;
	case PTP_MSG_DELAY_REQ:
		break;
	case PTP_MSG_PDELAY_REQ:
		break;
	case PTP_MSG_PDELAY_RESP:
		msg_timestamp_post_recv(msg, &msg->pdelay_resp.req_receipt_timestamp);
		msg_port_id_post_recv(&msg->pdelay_resp.req_port_id);
		break;
	case PTP_MSG_FOLLOW_UP:
		msg_timestamp_post_recv(msg, &msg->follow_up.precise_origin_timestamp);
		break;
	case PTP_MSG_DELAY_RESP:
		msg_timestamp_post_recv(msg, &msg->delay_resp.receive_timestamp);
		msg_port_id_post_recv(&msg->delay_resp.req_port_id);
		break;
	case PTP_MSG_PDELAY_RESP_FOLLOW_UP:
		msg_timestamp_post_recv(msg, &msg->pdelay_resp_follow_up.resp_origin_timestamp);
		msg_port_id_post_recv(&msg->pdelay_resp_follow_up.req_port_id);
		break;
	case PTP_MSG_ANNOUNCE:
		current = k_uptime_get();

		msg->timestamp.host.second = (uint64_t)(current / MSEC_PER_SEC);
		msg->timestamp.host.nanosecond = (current % MSEC_PER_SEC) * NSEC_PER_MSEC;
		msg_timestamp_post_recv(msg, &msg->announce.origin_timestamp);
		msg->announce.current_utc_offset = ntohs(msg->announce.current_utc_offset);
		msg->announce.gm_clk_quality.offset_scaled_log_variance =
			ntohs(msg->announce.gm_clk_quality.offset_scaled_log_variance);
		msg->announce.steps_rm = ntohs(msg->announce.steps_rm);
		break;
	case PTP_MSG_SIGNALING:
		msg_port_id_post_recv(&msg->signaling.target_port_id);
		break;
	case PTP_MSG_MANAGEMENT:
		msg_port_id_post_recv(&msg->management.target_port_id);
		break;
	}

	tlv_len = msg_tlv_post_recv(msg, cnt - msg_size[type]);
	if (tlv_len < 0) {
		LOG_ERR("Failed processing TLVs");
		return -EBADMSG;
	}

	if (msg_size[type] + tlv_len != msg->header.msg_length) {
		LOG_ERR("Length and TLVs don't correspond with specified in the message");
		return -EMSGSIZE;
	}

	return 0;
}

struct ptp_tlv *ptp_msg_add_tlv(struct ptp_msg *msg, int length)
{
	struct ptp_tlv_container *tlv_container;
	uint8_t *suffix = msg_suffix(msg);

	if (!suffix) {
		return NULL;
	}

	tlv_container = (struct ptp_tlv_container *)sys_slist_peek_tail(&msg->tlvs);
	if (tlv_container) {
		suffix = (uint8_t *)tlv_container->tlv;
		suffix += sizeof(*tlv_container->tlv);
		suffix += tlv_container->tlv->length;
	}

	if ((intptr_t)(suffix + length) >= (intptr_t)&msg->ref) {
		LOG_ERR("Not enough space for TLV of %d length", length);
		return NULL;
	}

	tlv_container = ptp_tlv_alloc();
	if (tlv_container) {
		tlv_container->tlv = (struct ptp_tlv *)suffix;
		msg->header.msg_length += length;
	}

	return tlv_container ? tlv_container->tlv : NULL;
}

int ptp_msg_announce_cmp(const struct ptp_announce_msg *m1, const struct ptp_announce_msg *m2)
{
	int len = sizeof(m1->gm_priority1) + sizeof(m1->gm_clk_quality) +
		  sizeof(m1->gm_priority1) + sizeof(m1->gm_id) +
		  sizeof(m1->steps_rm);

	return memcmp(&m1->gm_priority1, &m2->gm_priority1, len);
}

bool ptp_msg_current_parent(const struct ptp_msg *msg)
{
	const struct ptp_parent_ds *pds = ptp_clock_parent_ds();

	return ptp_port_id_eq(&pds->port_id, &msg->header.src_port_id);
}

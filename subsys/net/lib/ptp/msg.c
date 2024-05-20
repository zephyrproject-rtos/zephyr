/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_msg, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ptp.h>

#include "clock.h"
#include "msg.h"
#include "port.h"

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

struct ptp_msg *ptp_msg_alloc(void)
{
	struct ptp_msg *msg = NULL;
	int ret = k_mem_slab_alloc(&msg_slab, (void **)&msg, K_FOREVER);

	if (ret) {
		LOG_ERR("Couldn't allocate memory for the message");
		return NULL;
	}

	memset(msg, 0, sizeof(*msg));
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

	return 0;
}

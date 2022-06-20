/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_gptp, CONFIG_NET_GPTP_LOG_LEVEL);

#include <zephyr/net/net_if.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"
#include "gptp_md.h"
#include "gptp_private.h"

#define NET_BUF_TIMEOUT K_MSEC(100)

static struct net_if_timestamp_cb sync_timestamp_cb;
static struct net_if_timestamp_cb pdelay_response_timestamp_cb;
static bool sync_cb_registered;
static bool ts_cb_registered;

static const struct net_eth_addr gptp_multicast_eth_addr = {
	{ 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e } };

#define NET_GPTP_INFO(msg, pkt)						\
	if (CONFIG_NET_GPTP_LOG_LEVEL >= LOG_LEVEL_DBG) {		\
		struct gptp_hdr *hdr = GPTP_HDR(pkt);			\
									\
		if (hdr->message_type == GPTP_ANNOUNCE_MESSAGE) {	\
			struct gptp_announce *ann = GPTP_ANNOUNCE(pkt);	\
			char output[sizeof("xx:xx:xx:xx:xx:xx:xx:xx")];	\
									\
			gptp_sprint_clock_id(				\
				ann->root_system_id.grand_master_id,	\
				output,					\
				sizeof(output));			\
									\
			NET_DBG("Sending %s seq %d pkt %p",		\
				msg,					\
				ntohs(hdr->sequence_id), pkt);		\
									\
			NET_DBG("  GM %d/%d/0x%x/%d/%s",\
				ann->root_system_id.grand_master_prio1, \
				ann->root_system_id.clk_quality.clock_class, \
				ann->root_system_id.clk_quality.clock_accuracy,\
				ann->root_system_id.grand_master_prio2,	\
				output);			\
		} else {						\
			NET_DBG("Sending %s seq %d pkt %p",		\
				msg,					\
				ntohs(hdr->sequence_id), pkt);		\
		}							\
	}

struct gptp_hdr *gptp_get_hdr(struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->frags;

	NET_ASSERT(buf);

	if (sizeof(struct gptp_hdr) <= buf->len) {
		return (struct gptp_hdr *)buf->data;
	}

	/* Check if there is a link layer buf in the front and skip it
	 * if needed.
	 */
	buf = buf->frags;
	if (!buf) {
		/* Do not return null here but let the caller failure
		 * checks to fail the packet.
		 */
		return (struct gptp_hdr *)pkt->frags->data;
	}

	return (struct gptp_hdr *)buf->data;
}

static void gptp_sync_timestamp_callback(struct net_pkt *pkt)
{
	int port = 0;
	struct gptp_sync_send_state *state;
	struct gptp_hdr *hdr;

	port = gptp_get_port_number(net_pkt_iface(pkt));
	if (port == -ENODEV) {
		NET_DBG("No port found for ptp buffer");
		return;
	}

	state = &GPTP_PORT_STATE(port)->sync_send;

	hdr = GPTP_HDR(pkt);

	/* If this buffer is a sync, flag it to the state machine. */
	if (hdr->message_type == GPTP_SYNC_MESSAGE) {
		state->md_sync_timestamp_avail = true;

		net_if_unregister_timestamp_cb(&sync_timestamp_cb);
		sync_cb_registered = false;

		/* The pkt was ref'ed in gptp_send_sync() */
		net_pkt_unref(pkt);
	}
}

static void gptp_pdelay_response_timestamp_callback(struct net_pkt *pkt)
{
	int port = 0;
	struct net_pkt *follow_up;
	struct gptp_hdr *hdr;

	port = gptp_get_port_number(net_pkt_iface(pkt));
	if (port == -ENODEV) {
		NET_DBG("No port found for ptp buffer");
		goto out;
	}

	hdr = GPTP_HDR(pkt);

	/* If this buffer is a path delay response, send the follow up. */
	if (hdr->message_type == GPTP_PATH_DELAY_RESP_MESSAGE) {
		follow_up = gptp_prepare_pdelay_follow_up(port, pkt);
		if (!follow_up) {
			/* Cannot handle the follow up, abort */
			NET_ERR("Could not get buffer");
			goto out;
		}

		net_if_unregister_timestamp_cb(&pdelay_response_timestamp_cb);
		ts_cb_registered = false;

		gptp_send_pdelay_follow_up(port, follow_up,
					   net_pkt_timestamp(pkt));

out:
		/* The pkt was ref'ed in gptp_handle_pdelay_req() */
		net_pkt_unref(pkt);
	}
}

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
static struct net_pkt *setup_gptp_frame_debug(struct net_if *iface,
					      size_t extra_header,
					      const char *caller,
					      int line)
#define setup_gptp_frame(iface, extra_header)				\
	setup_gptp_frame_debug(iface, extra_header, __func__, __LINE__)
#else
static struct net_pkt *setup_gptp_frame(struct net_if *iface,
					size_t extra_header)
#endif
{
	struct net_pkt *pkt;

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	pkt = net_pkt_alloc_with_buffer_debug(iface, sizeof(struct gptp_hdr) +
					      extra_header, AF_UNSPEC, 0,
					      NET_BUF_TIMEOUT, caller, line);
#else
	pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct gptp_hdr) +
					extra_header, AF_UNSPEC, 0,
					NET_BUF_TIMEOUT);
#endif
	if (!pkt) {
		return NULL;
	}

	net_buf_add(pkt->buffer, sizeof(struct gptp_hdr) + extra_header);
	net_pkt_set_ptp(pkt, true);

	net_pkt_lladdr_src(pkt)->addr = net_if_get_link_addr(iface)->addr;
	net_pkt_lladdr_src(pkt)->len = net_if_get_link_addr(iface)->len;

	net_pkt_lladdr_dst(pkt)->addr = (uint8_t *)&gptp_multicast_eth_addr;
	net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);

	return pkt;
}

struct net_pkt *gptp_prepare_sync(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_sync *sync;
	struct net_if *iface;
	struct net_pkt *pkt;
	struct gptp_hdr *hdr;

	NET_ASSERT((port >= GPTP_PORT_START) && (port <= GPTP_PORT_END));
	iface = GPTP_PORT_IFACE(port);
	NET_ASSERT(iface);

	pkt = setup_gptp_frame(iface, sizeof(struct gptp_sync));
	if (!pkt) {
		NET_DBG("Cannot get gPTP frame");
		return NULL;
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	port_ds = GPTP_PORT_DS(port);
	sync = GPTP_SYNC(pkt);
	hdr = GPTP_HDR(pkt);

	/*
	 * Header configuration.
	 *
	 * Some fields are set by gptp_md_sync_send_prepare().
	 */
	hdr->transport_specific = GPTP_TRANSPORT_802_1_AS;
	hdr->message_type = GPTP_SYNC_MESSAGE;
	hdr->ptp_version = GPTP_VERSION;
	hdr->sequence_id = htons(port_ds->sync_seq_id);
	hdr->domain_number = 0U;
	hdr->correction_field = 0;
	hdr->flags.octets[0] = GPTP_FLAG_TWO_STEP;
	hdr->flags.octets[1] = GPTP_FLAG_PTP_TIMESCALE;
	hdr->message_length = htons(sizeof(struct gptp_hdr) +
				    sizeof(struct gptp_sync));
	hdr->control = GPTP_SYNC_CONTROL_VALUE;

	/* Clear reserved fields. */
	hdr->reserved0 = 0U;
	hdr->reserved1 = 0U;
	hdr->reserved2 = 0U;

	/* PTP configuration. */
	(void)memset(&sync->reserved, 0, sizeof(sync->reserved));

	/* Update sequence number. */
	port_ds->sync_seq_id++;

	return pkt;
}

struct net_pkt *gptp_prepare_follow_up(int port, struct net_pkt *sync)
{
	struct gptp_hdr *hdr, *sync_hdr;
	struct net_if *iface;
	struct net_pkt *pkt;

	NET_ASSERT(sync);
	NET_ASSERT((port >= GPTP_PORT_START) && (port <= GPTP_PORT_END));
	iface = GPTP_PORT_IFACE(port);
	NET_ASSERT(iface);

	pkt = setup_gptp_frame(iface, sizeof(struct gptp_follow_up));
	if (!pkt) {
		NET_DBG("Cannot get gPTP frame");
		return NULL;
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_IC);

	hdr = GPTP_HDR(pkt);
	sync_hdr = GPTP_HDR(sync);

	/*
	 * Header configuration.
	 *
	 * Some fields are set by gptp_md_follow_up_prepare().
	 */
	hdr->transport_specific = GPTP_TRANSPORT_802_1_AS;
	hdr->message_type = GPTP_FOLLOWUP_MESSAGE;
	hdr->ptp_version = GPTP_VERSION;
	hdr->sequence_id = sync_hdr->sequence_id;
	hdr->domain_number = 0U;
	/* Store timestamp value in correction field. */
	hdr->correction_field = gptp_timestamp_to_nsec(&sync->timestamp);
	hdr->flags.octets[0] = 0U;
	hdr->flags.octets[1] = GPTP_FLAG_PTP_TIMESCALE;
	hdr->message_length = htons(sizeof(struct gptp_hdr) +
				    sizeof(struct gptp_follow_up));
	hdr->control = GPTP_FUP_CONTROL_VALUE;

	/* Clear reserved fields. */
	hdr->reserved0 = 0U;
	hdr->reserved1 = 0U;
	hdr->reserved2 = 0U;

	/* PTP configuration will be set by the MDSyncSend state machine. */

	return pkt;
}

struct net_pkt *gptp_prepare_pdelay_req(int port)
{
	struct gptp_pdelay_req *req;
	struct gptp_port_ds *port_ds;
	struct net_if *iface;
	struct net_pkt *pkt;
	struct gptp_hdr *hdr;

	NET_ASSERT((port >= GPTP_PORT_START) && (port <= GPTP_PORT_END));
	iface = GPTP_PORT_IFACE(port);
	NET_ASSERT(iface);

	pkt = setup_gptp_frame(iface, sizeof(struct gptp_pdelay_req));
	if (!pkt) {
		NET_DBG("Cannot get gPTP frame");
		return NULL;
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	port_ds = GPTP_PORT_DS(port);
	req = GPTP_PDELAY_REQ(pkt);
	hdr = GPTP_HDR(pkt);

	/* Header configuration. */
	hdr->transport_specific = GPTP_TRANSPORT_802_1_AS;
	hdr->message_type = GPTP_PATH_DELAY_REQ_MESSAGE;
	hdr->ptp_version = GPTP_VERSION;
	hdr->sequence_id = htons(port_ds->pdelay_req_seq_id);
	hdr->domain_number = 0U;
	hdr->correction_field = 0;
	hdr->flags.octets[0] = 0U;
	hdr->flags.octets[1] = GPTP_FLAG_PTP_TIMESCALE;

	hdr->message_length = htons(sizeof(struct gptp_hdr) +
				    sizeof(struct gptp_pdelay_req));
	hdr->port_id.port_number = htons(port_ds->port_id.port_number);
	hdr->control = GPTP_OTHER_CONTROL_VALUE;
	hdr->log_msg_interval = port_ds->cur_log_pdelay_req_itv;

	/* Clear reserved fields. */
	hdr->reserved0 = 0U;
	hdr->reserved1 = 0U;
	hdr->reserved2 = 0U;

	memcpy(hdr->port_id.clk_id,
	       port_ds->port_id.clk_id, GPTP_CLOCK_ID_LEN);

	/* PTP configuration. */
	(void)memset(&req->reserved1, 0, sizeof(req->reserved1));
	(void)memset(&req->reserved2, 0, sizeof(req->reserved2));

	/* Update sequence number. */
	port_ds->pdelay_req_seq_id++;

	return pkt;
}

struct net_pkt *gptp_prepare_pdelay_resp(int port,
					 struct net_pkt *req)
{
	struct net_if *iface = net_pkt_iface(req);
	struct gptp_pdelay_resp *pdelay_resp;
	struct gptp_hdr *hdr, *query;
	struct gptp_port_ds *port_ds;
	struct net_pkt *pkt;

	pkt = setup_gptp_frame(iface, sizeof(struct gptp_pdelay_resp));
	if (!pkt) {
		NET_DBG("Cannot get gPTP frame");
		return NULL;
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	port_ds = GPTP_PORT_DS(port);

	pdelay_resp = GPTP_PDELAY_RESP(pkt);
	hdr = GPTP_HDR(pkt);

	query = GPTP_HDR(req);

	/* Header configuration. */
	hdr->transport_specific = GPTP_TRANSPORT_802_1_AS;
	hdr->message_type = GPTP_PATH_DELAY_RESP_MESSAGE;
	hdr->ptp_version = GPTP_VERSION;
	hdr->sequence_id = query->sequence_id;
	hdr->domain_number = query->domain_number;
	hdr->correction_field = query->correction_field;
	hdr->flags.octets[0] = GPTP_FLAG_TWO_STEP;
	hdr->flags.octets[1] = GPTP_FLAG_PTP_TIMESCALE;

	hdr->message_length = htons(sizeof(struct gptp_hdr) +
				    sizeof(struct gptp_pdelay_resp));
	hdr->port_id.port_number = htons(port_ds->port_id.port_number);
	hdr->control = GPTP_OTHER_CONTROL_VALUE;
	hdr->log_msg_interval = GPTP_RESP_LOG_MSG_ITV;

	/* Clear reserved fields. */
	hdr->reserved0 = 0U;
	hdr->reserved1 = 0U;
	hdr->reserved2 = 0U;

	memcpy(hdr->port_id.clk_id, port_ds->port_id.clk_id,
	       GPTP_CLOCK_ID_LEN);

	/* PTP configuration. */
	pdelay_resp->req_receipt_ts_secs_high = 0U;
	pdelay_resp->req_receipt_ts_secs_low = 0U;
	pdelay_resp->req_receipt_ts_nsecs = 0U;

	memcpy(&pdelay_resp->requesting_port_id,
	       &query->port_id, sizeof(struct gptp_port_identity));

	return pkt;
}

struct net_pkt *gptp_prepare_pdelay_follow_up(int port,
					      struct net_pkt *resp)
{
	struct net_if *iface = net_pkt_iface(resp);
	struct gptp_pdelay_resp_follow_up *follow_up;
	struct gptp_pdelay_resp *pdelay_resp;
	struct gptp_hdr *hdr, *resp_hdr;
	struct gptp_port_ds *port_ds;
	struct net_pkt *pkt;

	pkt = setup_gptp_frame(iface,
			       sizeof(struct gptp_pdelay_resp_follow_up));
	if (!pkt) {
		NET_DBG("Cannot get gPTP frame");
		return NULL;
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_IC);

	port_ds = GPTP_PORT_DS(port);

	follow_up = GPTP_PDELAY_RESP_FOLLOWUP(pkt);
	hdr = GPTP_HDR(pkt);

	pdelay_resp = GPTP_PDELAY_RESP(resp);
	resp_hdr = GPTP_HDR(resp);

	/* Header configuration. */
	hdr->transport_specific = GPTP_TRANSPORT_802_1_AS;
	hdr->ptp_version = GPTP_VERSION;
	hdr->message_type = GPTP_PATH_DELAY_FOLLOWUP_MESSAGE;
	hdr->sequence_id = resp_hdr->sequence_id;
	hdr->domain_number = resp_hdr->domain_number;
	hdr->correction_field = 0;
	hdr->message_length = htons(sizeof(struct gptp_hdr) +
				    sizeof(struct gptp_pdelay_resp_follow_up));
	hdr->port_id.port_number = htons(port_ds->port_id.port_number);
	hdr->control = GPTP_OTHER_CONTROL_VALUE;
	hdr->log_msg_interval = GPTP_RESP_LOG_MSG_ITV;

	hdr->flags.octets[0] = 0U;
	hdr->flags.octets[1] = GPTP_FLAG_PTP_TIMESCALE;

	/* Clear reserved fields. */
	hdr->reserved0 = 0U;
	hdr->reserved1 = 0U;
	hdr->reserved2 = 0U;

	memcpy(hdr->port_id.clk_id, port_ds->port_id.clk_id,
	       GPTP_CLOCK_ID_LEN);

	/* PTP configuration. */
	follow_up->resp_orig_ts_secs_high = 0U;
	follow_up->resp_orig_ts_secs_low = 0U;
	follow_up->resp_orig_ts_nsecs = 0U;

	memcpy(&follow_up->requesting_port_id,
	       &pdelay_resp->requesting_port_id,
	       sizeof(struct gptp_port_identity));

	return pkt;
}

struct net_pkt *gptp_prepare_announce(int port)
{
	struct gptp_global_ds *global_ds;
	struct gptp_default_ds *default_ds;
	struct gptp_port_ds *port_ds;
	struct gptp_announce *ann;
	struct net_if *iface;
	struct net_pkt *pkt;
	struct gptp_hdr *hdr;

	NET_ASSERT((port >= GPTP_PORT_START) && (port <= GPTP_PORT_END));
	global_ds = GPTP_GLOBAL_DS();
	default_ds = GPTP_DEFAULT_DS();
	iface = GPTP_PORT_IFACE(port);
	NET_ASSERT(iface);

	pkt = setup_gptp_frame(iface, sizeof(struct gptp_announce) - 8 +
			       ntohs(global_ds->path_trace.len));
	if (!pkt) {
		NET_DBG("Cannot get gPTP frame");
		return NULL;
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_IC);

	hdr = GPTP_HDR(pkt);
	ann = GPTP_ANNOUNCE(pkt);
	port_ds = GPTP_PORT_DS(port);

	hdr->message_type = GPTP_ANNOUNCE_MESSAGE;
	hdr->transport_specific = GPTP_TRANSPORT_802_1_AS;
	hdr->ptp_version = GPTP_VERSION;

	hdr->domain_number = 0U;
	hdr->correction_field = 0;
	hdr->flags.octets[0] = 0U;

	/* Copy leap61, leap59, current UTC offset valid, time traceable and
	 * frequency traceable flags.
	 */
	hdr->flags.octets[1] =
		global_ds->global_flags.octets[1] | GPTP_FLAG_PTP_TIMESCALE;

	memcpy(hdr->port_id.clk_id, GPTP_DEFAULT_DS()->clk_id,
	       GPTP_CLOCK_ID_LEN);

	hdr->port_id.port_number = htons(port);
	hdr->control = GPTP_OTHER_CONTROL_VALUE;
	hdr->log_msg_interval = port_ds->cur_log_announce_itv;

	/* Clear reserved fields. */
	hdr->reserved0 = 0U;
	hdr->reserved1 = 0U;
	hdr->reserved2 = 0U;

	ann->cur_utc_offset = global_ds->current_utc_offset;
	ann->time_source = global_ds->time_source;

	switch (GPTP_PORT_BMCA_DATA(port)->info_is) {
	case GPTP_INFO_IS_MINE:
		ann->root_system_id.grand_master_prio1 = default_ds->priority1;
		ann->root_system_id.grand_master_prio2 = default_ds->priority2;

		memcpy(&ann->root_system_id.clk_quality,
		       &default_ds->clk_quality,
		       sizeof(struct gptp_clock_quality));

		memcpy(&ann->root_system_id.grand_master_id,
		       default_ds->clk_id,
		       GPTP_CLOCK_ID_LEN);
		break;
	case GPTP_INFO_IS_RECEIVED:
		memcpy(&ann->root_system_id,
		       &GPTP_PORT_BMCA_DATA(port)->
				master_priority.root_system_id,
		       sizeof(struct gptp_root_system_identity));
		break;
	default:
		goto fail;
	}

	ann->steps_removed = global_ds->master_steps_removed;
	hdr->sequence_id = htons(port_ds->announce_seq_id);
	port_ds->announce_seq_id++;

	ann->tlv.type = GPTP_ANNOUNCE_MSG_PATH_SEQ_TYPE;

	/* Clear reserved fields. */
	(void)memset(ann->reserved1, 0, sizeof(ann->reserved1));
	ann->reserved2 = 0U;

	hdr->message_length = htons(sizeof(struct gptp_hdr) +
				    sizeof(struct gptp_announce) - 8 +
				    ntohs(global_ds->path_trace.len));

	ann->tlv.len = global_ds->path_trace.len;

	net_pkt_cursor_init(pkt);

	/* setup_gptp_frame() already added all the length, so let's not
	 * add up more with net_pkt_skip/write_new(), let's just overwrite
	 */
	net_pkt_set_overwrite(pkt, true);

	if (net_pkt_skip(pkt, sizeof(struct gptp_hdr) +
			 sizeof(struct gptp_announce) - 8) ||
	    net_pkt_write(pkt, &global_ds->path_trace.path_sequence[0][0],
			  ntohs(global_ds->path_trace.len))) {
		goto fail;
	}

	net_pkt_set_overwrite(pkt, false);

	return pkt;

fail:
	net_pkt_unref(pkt);

	return NULL;
}

void gptp_handle_sync(int port, struct net_pkt *pkt)
{
	struct gptp_sync_rcv_state *state;
	struct gptp_hdr *hdr;
	uint64_t upstream_sync_itv;
	k_timeout_t duration;

	state = &GPTP_PORT_STATE(port)->sync_rcv;
	hdr = GPTP_HDR(state->rcvd_sync_ptr);

	upstream_sync_itv = NSEC_PER_SEC * GPTP_POW2(hdr->log_msg_interval);

	/* Convert ns to ms. */
	duration = K_MSEC((upstream_sync_itv / 1000000U));

	/* Start timeout timer. */
	k_timer_start(&state->follow_up_discard_timer, duration, K_NO_WAIT);
}

int gptp_handle_follow_up(int port, struct net_pkt *pkt)
{
	struct gptp_sync_rcv_state *state;
	struct gptp_hdr *sync_hdr, *hdr;

	state = &GPTP_PORT_STATE(port)->sync_rcv;

	sync_hdr = GPTP_HDR(state->rcvd_sync_ptr);
	hdr = GPTP_HDR(pkt);

	if (sync_hdr->sequence_id != hdr->sequence_id) {
		NET_WARN("%s sequence id %d %s %s %d",
			 "FOLLOWUP", ntohs(hdr->sequence_id),
			 "does not match",
			 "SYNC", ntohs(sync_hdr->sequence_id));
		return -EINVAL;
	}

	GPTP_STATS_INC(port, rx_fup_count);

	return 0;
}

void gptp_handle_pdelay_req(int port, struct net_pkt *pkt)
{
	struct net_pkt *reply;

	GPTP_STATS_INC(port, rx_pdelay_req_count);

	if (ts_cb_registered == true) {
		NET_WARN("Multiple pdelay requests");

		net_if_unregister_timestamp_cb(&pdelay_response_timestamp_cb);
		net_pkt_unref(pdelay_response_timestamp_cb.pkt);

		ts_cb_registered = false;
	}

	/* Prepare response and send */
	reply = gptp_prepare_pdelay_resp(port, pkt);
	if (!reply) {
		return;
	}

	net_if_register_timestamp_cb(&pdelay_response_timestamp_cb,
				     reply,
				     net_pkt_iface(pkt),
				     gptp_pdelay_response_timestamp_callback);

	/* TS thread will send this back to us so increment ref count so that
	 * the packet is not removed when sending it. This will be unref'ed by
	 * timestamp callback in gptp_pdelay_response_timestamp_callback()
	 */
	net_pkt_ref(reply);

	ts_cb_registered = true;

	gptp_send_pdelay_resp(port, reply, net_pkt_timestamp(pkt));
}

int gptp_handle_pdelay_resp(int port, struct net_pkt *pkt)
{
	struct gptp_pdelay_req_state *state;
	struct gptp_default_ds *default_ds;
	struct gptp_pdelay_resp *resp;
	struct gptp_hdr *hdr, *req_hdr;

	hdr = GPTP_HDR(pkt);
	resp = GPTP_PDELAY_RESP(pkt);
	state = &GPTP_PORT_STATE(port)->pdelay_req;
	default_ds = GPTP_DEFAULT_DS();

	if (!state->tx_pdelay_req_ptr) {
		goto reset;
	}

	req_hdr = GPTP_HDR(state->tx_pdelay_req_ptr);

	/* Check clock identity. */
	if (memcmp(default_ds->clk_id, resp->requesting_port_id.clk_id,
		   GPTP_CLOCK_ID_LEN)) {
		NET_WARN("Requesting Clock Identity %s", "does not match");
		goto reset;
	}
	if (memcmp(default_ds->clk_id, hdr->port_id.clk_id,
		   GPTP_CLOCK_ID_LEN) == 0) {
		NET_WARN("Source Clock Identity is local Clock Identity");
		goto reset;
	}

	/* Check port number. */
	if (resp->requesting_port_id.port_number != htons(port)) {
		NET_WARN("Requesting Port Number %s", "does not match");
		goto reset;
	}

	/* Check sequence id. */
	if (hdr->sequence_id != req_hdr->sequence_id) {
		NET_WARN("Sequence Id %d %s %d",
			 ntohs(hdr->sequence_id),
			 "does not match",
			 ntohs(req_hdr->sequence_id));
		goto reset;
	}

	GPTP_STATS_INC(port, rx_pdelay_resp_count);

	return 0;

reset:
	return -EINVAL;
}

int gptp_handle_pdelay_follow_up(int port, struct net_pkt *pkt)
{
	struct gptp_pdelay_resp_follow_up *follow_up;
	struct gptp_hdr *hdr, *req_hdr, *resp_hdr;
	struct gptp_pdelay_req_state *state;
	struct gptp_default_ds *default_ds;

	hdr = GPTP_HDR(pkt);
	follow_up = GPTP_PDELAY_RESP_FOLLOWUP(pkt);
	state = &GPTP_PORT_STATE(port)->pdelay_req;
	default_ds = GPTP_DEFAULT_DS();

	if (!state->tx_pdelay_req_ptr) {
		goto reset;
	}

	req_hdr = GPTP_HDR(state->tx_pdelay_req_ptr);

	if (!state->rcvd_pdelay_resp_ptr) {
		goto reset;
	}

	resp_hdr = GPTP_HDR(state->rcvd_pdelay_resp_ptr);

	/* Check clock identity. */
	if (memcmp(default_ds->clk_id, follow_up->requesting_port_id.clk_id,
		   GPTP_CLOCK_ID_LEN)) {
		NET_WARN("Requesting Clock Identity %s", "does not match");
		goto reset;
	}

	if (memcmp(default_ds->clk_id, hdr->port_id.clk_id,
		   GPTP_CLOCK_ID_LEN) == 0) {
		NET_WARN("Source Clock Identity is local Clock Identity");
		goto reset;
	}

	/* Check port number. */
	if (follow_up->requesting_port_id.port_number != htons(port)) {
		NET_WARN("Requesting Port Number %s", "does not match");
		goto reset;
	}

	/* Check sequence id. */
	if (hdr->sequence_id != req_hdr->sequence_id) {
		NET_WARN("Sequence Id %d %s %d",
			 ntohs(hdr->sequence_id),
			 "does not match",
			 ntohs(req_hdr->sequence_id));
		goto reset;
	}

	/* Check source port. */
	if (memcmp(&hdr->port_id, &resp_hdr->port_id,
		   sizeof(hdr->port_id)) != 0) {
		NET_WARN("pDelay response and follow up port IDs %s",
			"does not match");
		goto reset;
	}

	GPTP_STATS_INC(port, rx_fup_count);

	return 0;

reset:
	return -EINVAL;
}

void gptp_handle_signaling(int port, struct net_pkt *pkt)
{
	struct gptp_port_ds *port_ds;
	struct gptp_signaling *sig;

	sig = GPTP_SIGNALING(pkt);
	port_ds = GPTP_PORT_DS(port);

	/* If time-synchronization not enabled, drop packet. */
	if (!port_ds->ptt_port_enabled) {
		return;
	}

	/* pDelay interval. */
	gptp_update_pdelay_req_interval(port, sig->tlv.link_delay_itv);

	/* Sync interval. */
	gptp_update_sync_interval(port, sig->tlv.time_sync_itv);

	/* Announce interval. */
	gptp_update_announce_interval(port, sig->tlv.announce_itv);

	port_ds->compute_neighbor_rate_ratio =
		sig->tlv.compute_neighbor_rate_ratio;
	port_ds->compute_neighbor_prop_delay =
		sig->tlv.compute_neighbor_prop_delay;
}

void gptp_send_sync(int port, struct net_pkt *pkt)
{
	if (!sync_cb_registered) {
		net_if_register_timestamp_cb(&sync_timestamp_cb,
					     pkt,
					     net_pkt_iface(pkt),
					     gptp_sync_timestamp_callback);
		sync_cb_registered = true;
	}

	GPTP_STATS_INC(port, tx_sync_count);

	/* TS thread will send this back to us so increment ref count
	 * so that the packet is not removed when sending it.
	 * This will be unref'ed by timestamp callback in
	 * gptp_sync_timestamp_callback()
	 */
	net_pkt_ref(pkt);

	NET_GPTP_INFO("SYNC", pkt);

	net_if_queue_tx(net_pkt_iface(pkt), pkt);
}

void gptp_send_follow_up(int port, struct net_pkt *pkt)
{
	GPTP_STATS_INC(port, tx_fup_count);

	NET_GPTP_INFO("FOLLOWUP", pkt);

	net_if_queue_tx(net_pkt_iface(pkt), pkt);
}

void gptp_send_announce(int port, struct net_pkt *pkt)
{
	GPTP_STATS_INC(port, tx_announce_count);

	NET_GPTP_INFO("ANNOUNCE", pkt);

	net_if_queue_tx(net_pkt_iface(pkt), pkt);
}

void gptp_send_pdelay_req(int port)
{
	struct gptp_pdelay_req_state *state;
	struct net_pkt *pkt;

	NET_ASSERT((port >= GPTP_PORT_START) && (port <= GPTP_PORT_END));
	state = &GPTP_PORT_STATE(port)->pdelay_req;

	pkt = gptp_prepare_pdelay_req(port);
	if (pkt) {
		if (state->tx_pdelay_req_ptr) {
			NET_DBG("Unref pending %s %p", "PDELAY_REQ",
				state->tx_pdelay_req_ptr);

			net_pkt_unref(state->tx_pdelay_req_ptr);
		}

		/* Keep the buffer alive until pdelay_rate_ratio is computed. */
		state->tx_pdelay_req_ptr = net_pkt_ref(pkt);

		GPTP_STATS_INC(port, tx_pdelay_req_count);

		NET_GPTP_INFO("PDELAY_REQ", pkt);

		net_if_queue_tx(net_pkt_iface(pkt), pkt);
	} else {
		NET_ERR("Failed to prepare %s", "PDELAY_REQ");
	}
}

void gptp_send_pdelay_resp(int port, struct net_pkt *pkt,
			   struct net_ptp_time *treq)
{
	struct gptp_pdelay_resp *resp;
	struct gptp_hdr *hdr;

	hdr = GPTP_HDR(pkt);

	/* No Fractional nsec .*/
	hdr->correction_field = 0;

	resp = GPTP_PDELAY_RESP(pkt);
	resp->req_receipt_ts_secs_high = htons(treq->_sec.high);
	resp->req_receipt_ts_secs_low = htonl(treq->_sec.low);
	resp->req_receipt_ts_nsecs = htonl(treq->nanosecond);

	GPTP_STATS_INC(port, tx_pdelay_resp_count);

	NET_GPTP_INFO("PDELAY_RESP", pkt);

	net_if_queue_tx(net_pkt_iface(pkt), pkt);
}

void gptp_send_pdelay_follow_up(int port, struct net_pkt *pkt,
				struct net_ptp_time *tresp)
{
	struct gptp_pdelay_resp_follow_up *follow_up;
	struct gptp_hdr *hdr;

	hdr = GPTP_HDR(pkt);

	/* No Fractional nsec .*/
	hdr->correction_field = 0;

	follow_up = GPTP_PDELAY_RESP_FOLLOWUP(pkt);
	follow_up->resp_orig_ts_secs_high = htons(tresp->_sec.high);
	follow_up->resp_orig_ts_secs_low = htonl(tresp->_sec.low);
	follow_up->resp_orig_ts_nsecs = htonl(tresp->nanosecond);

	GPTP_STATS_INC(port, tx_pdelay_resp_fup_count);

	NET_GPTP_INFO("PDELAY_FOLLOWUP", pkt);

	net_if_queue_tx(net_pkt_iface(pkt), pkt);
}

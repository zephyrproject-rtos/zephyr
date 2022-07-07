/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_gptp, CONFIG_NET_GPTP_LOG_LEVEL);

#include "gptp_messages.h"
#include "gptp_md.h"
#include "gptp_data_set.h"
#include "gptp_private.h"

static void gptp_md_sync_prepare(struct net_pkt *pkt,
				 struct gptp_md_sync_info *sync_send,
				 int port_number)
{
	struct gptp_hdr *hdr;

	hdr = GPTP_HDR(pkt);

	memcpy(&hdr->port_id.clk_id, &sync_send->src_port_id.clk_id,
	       GPTP_CLOCK_ID_LEN);

	hdr->port_id.port_number = htons(port_number);

	hdr->log_msg_interval = sync_send->log_msg_interval;
}

static void gptp_md_follow_up_prepare(struct net_pkt *pkt,
				      struct gptp_md_sync_info *sync_send,
				      int port_number)
{
	struct gptp_hdr *hdr;
	struct gptp_follow_up *fup;

	hdr = GPTP_HDR(pkt);
	fup = GPTP_FOLLOW_UP(pkt);

	/*
	 * Compute correction field according to IEEE802.1AS 11.2.14.2.3.
	 *
	 * The correction_field already contains the timestamp of the sync
	 * message.
	 *
	 * TODO: if the value to be stored in correction_field is too big to
	 * be represented, the field should be set to all 1's except the most
	 * significant bit.
	 */
	hdr->correction_field -= sync_send->upstream_tx_time;
	hdr->correction_field *= sync_send->rate_ratio;
	hdr->correction_field += sync_send->follow_up_correction_field;
	hdr->correction_field <<= 16;
	hdr->correction_field = htonll(hdr->correction_field);

	memcpy(&hdr->port_id.clk_id, &sync_send->src_port_id.clk_id,
	       GPTP_CLOCK_ID_LEN);

	hdr->port_id.port_number = htons(port_number);

	hdr->log_msg_interval = sync_send->log_msg_interval;

	fup->prec_orig_ts_secs_high =
		htons(sync_send->precise_orig_ts._sec.high);
	fup->prec_orig_ts_secs_low =
		htonl(sync_send->precise_orig_ts._sec.low);
	fup->prec_orig_ts_nsecs =
		htonl(sync_send->precise_orig_ts.nanosecond);

	fup->tlv_hdr.type = htons(GPTP_TLV_ORGANIZATION_EXT);
	fup->tlv_hdr.len = htons(sizeof(struct gptp_follow_up_tlv));
	fup->tlv.org_id[0] = GPTP_FUP_TLV_ORG_ID_BYTE_0;
	fup->tlv.org_id[1] = GPTP_FUP_TLV_ORG_ID_BYTE_1;
	fup->tlv.org_id[2] = GPTP_FUP_TLV_ORG_ID_BYTE_2;
	fup->tlv.org_sub_type[0] = 0U;
	fup->tlv.org_sub_type[1] = 0U;
	fup->tlv.org_sub_type[2] = GPTP_FUP_TLV_ORG_SUB_TYPE;

	fup->tlv.cumulative_scaled_rate_offset =
		(sync_send->rate_ratio - 1.0) * GPTP_POW2_41;
	fup->tlv.cumulative_scaled_rate_offset =
		ntohl(fup->tlv.cumulative_scaled_rate_offset);
	fup->tlv.gm_time_base_indicator =
		ntohs(sync_send->gm_time_base_indicator);
	fup->tlv.last_gm_phase_change.high =
		ntohl(sync_send->last_gm_phase_change.high);
	fup->tlv.last_gm_phase_change.low =
		ntohll(sync_send->last_gm_phase_change.low);
	fup->tlv.scaled_last_gm_freq_change = sync_send->last_gm_freq_change;
	fup->tlv.scaled_last_gm_freq_change =
		ntohl(fup->tlv.scaled_last_gm_freq_change);
}

static int gptp_set_md_sync_receive(int port,
				    struct gptp_md_sync_info *sync_rcv)
{
	struct gptp_sync_rcv_state *state;
	struct gptp_port_ds *port_ds;
	struct gptp_hdr *sync_hdr, *fup_hdr;
	struct gptp_follow_up *fup;
	struct net_ptp_time *sync_ts;
	double prop_delay_rated;
	double delay_asymmetry_rated;

	state = &GPTP_PORT_STATE(port)->sync_rcv;
	if (!state->rcvd_sync_ptr || !state->rcvd_follow_up_ptr) {
		return -EINVAL;
	}

	port_ds = GPTP_PORT_DS(port);

	sync_hdr = GPTP_HDR(state->rcvd_sync_ptr);
	fup_hdr = GPTP_HDR(state->rcvd_follow_up_ptr);
	fup = GPTP_FOLLOW_UP(state->rcvd_follow_up_ptr);
	sync_ts = &state->rcvd_sync_ptr->timestamp;

	sync_rcv->follow_up_correction_field =
		ntohll(fup_hdr->correction_field);
	memcpy(&sync_rcv->src_port_id, &sync_hdr->port_id,
	       sizeof(struct gptp_port_identity));
	sync_rcv->log_msg_interval = fup_hdr->log_msg_interval;
	sync_rcv->precise_orig_ts._sec.high =
		ntohs(fup->prec_orig_ts_secs_high);
	sync_rcv->precise_orig_ts._sec.low = ntohl(fup->prec_orig_ts_secs_low);
	sync_rcv->precise_orig_ts.nanosecond = ntohl(fup->prec_orig_ts_nsecs);

	/* Compute time when sync was sent by the remote. */
	sync_rcv->upstream_tx_time = sync_ts->second;
	sync_rcv->upstream_tx_time *= NSEC_PER_SEC;
	sync_rcv->upstream_tx_time += sync_ts->nanosecond;

	prop_delay_rated = port_ds->neighbor_prop_delay;
	prop_delay_rated /= port_ds->neighbor_rate_ratio;

	sync_rcv->upstream_tx_time -= prop_delay_rated;

	delay_asymmetry_rated = port_ds->delay_asymmetry;
	delay_asymmetry_rated /= port_ds->neighbor_rate_ratio;

	sync_rcv->upstream_tx_time -= delay_asymmetry_rated;

	sync_rcv->rate_ratio = ntohl(fup->tlv.cumulative_scaled_rate_offset);
	sync_rcv->rate_ratio /= GPTP_POW2_41;
	sync_rcv->rate_ratio += 1;

	sync_rcv->gm_time_base_indicator =
		ntohs(fup->tlv.gm_time_base_indicator);
	sync_rcv->last_gm_phase_change.high =
		ntohl(fup->tlv.last_gm_phase_change.high);
	sync_rcv->last_gm_phase_change.low =
		ntohll(fup->tlv.last_gm_phase_change.low);
	sync_rcv->last_gm_freq_change =
		ntohl(fup->tlv.scaled_last_gm_freq_change);

	return 0;
}

static void gptp_md_pdelay_reset(int port)
{
	struct gptp_pdelay_req_state *state;
	struct gptp_port_ds *port_ds;

	NET_WARN("Reset Pdelay requests");

	state = &GPTP_PORT_STATE(port)->pdelay_req;
	port_ds = GPTP_PORT_DS(port);

	if (state->lost_responses < port_ds->allowed_lost_responses) {
		state->lost_responses += 1U;
	} else {
		port_ds->is_measuring_delay = false;
		port_ds->as_capable = false;
		state->init_pdelay_compute = true;
	}
}

static void gptp_md_pdelay_check_multiple_resp(int port)
{
	struct gptp_pdelay_req_state *state;
	struct gptp_port_ds *port_ds;
	int duration;

	state = &GPTP_PORT_STATE(port)->pdelay_req;
	port_ds = GPTP_PORT_DS(port);

	if ((state->rcvd_pdelay_resp > 1) ||
			(state->rcvd_pdelay_follow_up > 1)) {
		port_ds->as_capable = false;
		NET_WARN("Too many responses (%d / %d)",
			 state->rcvd_pdelay_resp,
			 state->rcvd_pdelay_follow_up);
		state->multiple_resp_count++;
	} else {
		state->multiple_resp_count = 0U;
	}

	if (state->multiple_resp_count >= 3U) {
		state->multiple_resp_count = 0U;
		k_timer_stop(&state->pdelay_timer);
		state->pdelay_timer_expired = false;

		/* Subtract time spent since last pDelay request. */
		duration = GPTP_MULTIPLE_PDELAY_RESP_WAIT -
			gptp_uscaled_ns_to_timer_ms(&port_ds->pdelay_req_itv);

		k_timer_start(&state->pdelay_timer, K_MSEC(duration),
			      K_NO_WAIT);
	} else {
		state->state = GPTP_PDELAY_REQ_SEND_REQ;
	}
}

static void gptp_md_compute_pdelay_rate_ratio(int port)
{
	uint64_t ingress_tstamp = 0U;
	uint64_t resp_evt_tstamp = 0U;
	struct gptp_pdelay_resp_follow_up *fup;
	struct gptp_pdelay_req_state *state;
	struct gptp_port_ds *port_ds;
	struct net_pkt *pkt;
	struct gptp_hdr *hdr;
	double neighbor_rate_ratio;

	state = &GPTP_PORT_STATE(port)->pdelay_req;
	port_ds = GPTP_PORT_DS(port);

	/* Get ingress timestamp. */
	pkt = state->rcvd_pdelay_resp_ptr;
	if (pkt) {
		ingress_tstamp =
			gptp_timestamp_to_nsec(net_pkt_timestamp(pkt));
	}

	/* Get peer corrected timestamp. */
	pkt = state->rcvd_pdelay_follow_up_ptr;
	if (pkt) {
		hdr = GPTP_HDR(pkt);
		fup = GPTP_PDELAY_RESP_FOLLOWUP(pkt);

		resp_evt_tstamp = ntohs(fup->resp_orig_ts_secs_high);
		resp_evt_tstamp <<= 32;
		resp_evt_tstamp |= ntohl(fup->resp_orig_ts_secs_low);
		resp_evt_tstamp *= NSEC_PER_SEC;
		resp_evt_tstamp += ntohl(fup->resp_orig_ts_nsecs);
		resp_evt_tstamp += (ntohll(hdr->correction_field) >> 16);
	}

	if (state->init_pdelay_compute) {
		state->init_pdelay_compute = false;

		state->ini_resp_ingress_tstamp = ingress_tstamp;
		state->ini_resp_evt_tstamp = resp_evt_tstamp;

		neighbor_rate_ratio = 1.0;

		state->neighbor_rate_ratio_valid = false;
	} else {
		if (resp_evt_tstamp == state->ini_resp_evt_tstamp) {
			state->neighbor_rate_ratio_valid = false;
			neighbor_rate_ratio = 1.0;
		} else {
			neighbor_rate_ratio =
				(resp_evt_tstamp - state->ini_resp_evt_tstamp);
			neighbor_rate_ratio /=
				(ingress_tstamp -
				 state->ini_resp_ingress_tstamp);

			/* Measure the ratio with the previously sent response.
			 */
			state->ini_resp_ingress_tstamp = ingress_tstamp;
			state->ini_resp_evt_tstamp = resp_evt_tstamp;
			state->neighbor_rate_ratio_valid = true;
		}
	}

	port_ds->neighbor_rate_ratio = neighbor_rate_ratio;
	port_ds->neighbor_rate_ratio_valid = state->neighbor_rate_ratio_valid;
}

static void gptp_md_compute_prop_time(int port)
{
	uint64_t t1_ns = 0U, t2_ns = 0U, t3_ns = 0U, t4_ns = 0U;
	struct gptp_pdelay_resp_follow_up *fup;
	struct gptp_pdelay_req_state *state;
	struct gptp_pdelay_resp *resp;
	struct gptp_port_ds *port_ds;
	struct gptp_hdr *hdr;
	struct net_pkt *pkt;
	double prop_time, turn_around;

	state = &GPTP_PORT_STATE(port)->pdelay_req;
	port_ds = GPTP_PORT_DS(port);

	/* Get egress timestamp. */
	pkt = state->tx_pdelay_req_ptr;
	if (pkt) {
		t1_ns = gptp_timestamp_to_nsec(net_pkt_timestamp(pkt));
	}

	/* Get ingress timestamp. */
	pkt = state->rcvd_pdelay_resp_ptr;
	if (pkt) {
		t4_ns = gptp_timestamp_to_nsec(net_pkt_timestamp(pkt));
	}

	/* Get peer corrected timestamps. */
	pkt = state->rcvd_pdelay_resp_ptr;
	if (pkt) {
		hdr = GPTP_HDR(pkt);
		resp = GPTP_PDELAY_RESP(pkt);

		t2_ns = ((uint64_t)ntohs(resp->req_receipt_ts_secs_high)) << 32;
		t2_ns |= ntohl(resp->req_receipt_ts_secs_low);
		t2_ns *= NSEC_PER_SEC;
		t2_ns += ntohl(resp->req_receipt_ts_nsecs);
		t2_ns += (ntohll(hdr->correction_field) >> 16);
	}

	pkt = state->rcvd_pdelay_follow_up_ptr;
	if (pkt) {
		hdr = GPTP_HDR(pkt);
		fup = GPTP_PDELAY_RESP_FOLLOWUP(pkt);

		t3_ns = ((uint64_t)ntohs(fup->resp_orig_ts_secs_high)) << 32;
		t3_ns |= ntohl(fup->resp_orig_ts_secs_low);
		t3_ns *= NSEC_PER_SEC;
		t3_ns += ntohl(fup->resp_orig_ts_nsecs);
		t3_ns += (ntohll(hdr->correction_field) >> 16);
	}

	prop_time = t4_ns - t1_ns;

	turn_around = t3_ns - t2_ns;

	/* Adjusting the turn-around time for peer to local clock rate
	 * difference. The check is implemented the same way as how Avnu/gptp
	 * daemon is doing it. This comment is also found in their source
	 * for the magic values "TODO: Are these .998 and 1.002 specifically
	 * defined in the standard?"
	 */
	if (port_ds->neighbor_rate_ratio > .998 &&
	    port_ds->neighbor_rate_ratio < 1.002) {
		turn_around *= port_ds->neighbor_rate_ratio;
	}

	prop_time -= turn_around;
	prop_time /= 2;

	port_ds->neighbor_prop_delay = prop_time;
}

static void gptp_md_pdelay_compute(int port)
{
	struct gptp_pdelay_req_state *state;
	struct gptp_port_ds *port_ds;
	struct gptp_hdr *hdr;
	struct net_pkt *pkt;
	bool local_clock;

	state = &GPTP_PORT_STATE(port)->pdelay_req;
	port_ds = GPTP_PORT_DS(port);

	if (!state->tx_pdelay_req_ptr || !state->rcvd_pdelay_resp_ptr ||
	    !state->rcvd_pdelay_follow_up_ptr) {
		NET_ERR("Compute path delay called without buffer ready");
		port_ds->as_capable = false;
		goto out;
	}

	if (port_ds->compute_neighbor_rate_ratio) {
		gptp_md_compute_pdelay_rate_ratio(port);
	}

	if (port_ds->compute_neighbor_prop_delay) {
		gptp_md_compute_prop_time(port);

		NET_DBG("Neighbor prop delay %d",
			(int32_t)port_ds->neighbor_prop_delay);
	}

	state->lost_responses = 0U;
	port_ds->is_measuring_delay = true;

	pkt = state->rcvd_pdelay_follow_up_ptr;
	hdr = GPTP_HDR(pkt);

	local_clock = !memcmp(gptp_domain.default_ds.clk_id,
			      hdr->port_id.clk_id,
			      GPTP_CLOCK_ID_LEN);
	if (local_clock) {
		NET_WARN("Discard path delay response from local clock.");
		goto out;
	}

	if (!state->neighbor_rate_ratio_valid) {
		goto out;
	}

	/*
	 * Currently, if the computed delay is negative, this means
	 * that it is negligible enough compared to other factors.
	 */
	if ((port_ds->neighbor_prop_delay <=
	     port_ds->neighbor_prop_delay_thresh)) {
		port_ds->as_capable = true;
	} else {
		port_ds->as_capable = false;

		NET_WARN("Not AS capable: %u ns > %u ns",
			 (uint32_t)port_ds->neighbor_prop_delay,
			 (uint32_t)port_ds->neighbor_prop_delay_thresh);

		GPTP_STATS_INC(port, neighbor_prop_delay_exceeded);
	}

out:
	/* Release buffers. */
	if (state->tx_pdelay_req_ptr) {
		net_pkt_unref(state->tx_pdelay_req_ptr);
		state->tx_pdelay_req_ptr = NULL;
	}

	if (state->rcvd_pdelay_resp_ptr) {
		net_pkt_unref(state->rcvd_pdelay_resp_ptr);
		state->rcvd_pdelay_resp_ptr = NULL;
	}

	if (state->rcvd_pdelay_follow_up_ptr) {
		net_pkt_unref(state->rcvd_pdelay_follow_up_ptr);
		state->rcvd_pdelay_follow_up_ptr = NULL;
	}
}

static void gptp_md_pdelay_req_timeout(struct k_timer *timer)
{
	struct gptp_pdelay_req_state *state;
	int port;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->pdelay_req;
		if (timer == &state->pdelay_timer) {
			state->pdelay_timer_expired = true;

			if (state->rcvd_pdelay_resp == 0U) {
				GPTP_STATS_INC(port,
					pdelay_allowed_lost_resp_exceed_count);
			}
		}
	}
}

static void gptp_md_start_pdelay_req(int port)
{
	struct gptp_pdelay_req_state *state;
	struct gptp_port_ds *port_ds;

	port_ds = GPTP_PORT_DS(port);
	state = &GPTP_PORT_STATE(port)->pdelay_req;

	port_ds->neighbor_rate_ratio = 1.0;
	port_ds->is_measuring_delay = false;
	port_ds->as_capable = false;
	state->lost_responses = 0U;
	state->rcvd_pdelay_resp = 0U;
	state->rcvd_pdelay_follow_up = 0U;
	state->multiple_resp_count = 0U;
}

static void gptp_md_follow_up_receipt_timeout(struct k_timer *timer)
{
	struct gptp_sync_rcv_state *state;
	int port;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->sync_rcv;
		if (timer == &state->follow_up_discard_timer) {
			NET_WARN("No %s received after %s message",
				 "FOLLOWUP", "SYNC");
			state->follow_up_timeout_expired = true;
		}
	}
}

static void gptp_md_init_pdelay_req_state_machine(int port)
{
	struct gptp_pdelay_req_state *state;

	state = &GPTP_PORT_STATE(port)->pdelay_req;

	k_timer_init(&state->pdelay_timer, gptp_md_pdelay_req_timeout, NULL);

	state->state = GPTP_PDELAY_REQ_NOT_ENABLED;

	state->neighbor_rate_ratio_valid = false;
	state->init_pdelay_compute = true;
	state->rcvd_pdelay_resp = 0U;
	state->rcvd_pdelay_follow_up = 0U;
	state->pdelay_timer_expired = false;

	state->rcvd_pdelay_resp_ptr = NULL;
	state->rcvd_pdelay_follow_up_ptr = NULL;
	state->tx_pdelay_req_ptr = NULL;

	state->ini_resp_evt_tstamp = 0U;
	state->ini_resp_ingress_tstamp = 0U;
	state->lost_responses = 0U;
}

static void gptp_md_init_pdelay_resp_state_machine(int port)
{
	struct gptp_pdelay_resp_state *state;

	state = &GPTP_PORT_STATE(port)->pdelay_resp;

	state->state = GPTP_PDELAY_RESP_NOT_ENABLED;
}

static void gptp_md_init_sync_rcv_state_machine(int port)
{
	struct gptp_sync_rcv_state *state;

	state = &GPTP_PORT_STATE(port)->sync_rcv;

	k_timer_init(&state->follow_up_discard_timer,
		     gptp_md_follow_up_receipt_timeout, NULL);

	state->rcvd_sync = false;
	state->rcvd_follow_up = false;
	state->rcvd_sync_ptr = NULL;
	state->rcvd_follow_up_ptr = NULL;

	state->follow_up_timeout_expired = false;
	state->follow_up_receipt_timeout = 0U;

	state->state = GPTP_SYNC_RCV_DISCARD;
}

static void gptp_md_init_sync_send_state_machine(int port)
{
	struct gptp_sync_send_state *state;

	state = &GPTP_PORT_STATE(port)->sync_send;

	state->rcvd_md_sync = false;
	state->md_sync_timestamp_avail = false;
	state->sync_send_ptr = NULL;
	state->sync_ptr = NULL;

	state->state = GPTP_SYNC_SEND_INITIALIZING;
}

void gptp_md_init_state_machine(void)
{
	int port;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		gptp_md_init_pdelay_req_state_machine(port);
		gptp_md_init_pdelay_resp_state_machine(port);
		gptp_md_init_sync_rcv_state_machine(port);
		gptp_md_init_sync_send_state_machine(port);
	}
}

static void gptp_md_pdelay_req_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_pdelay_req_state *state;
	struct net_pkt *pkt;

	state = &GPTP_PORT_STATE(port)->pdelay_req;
	port_ds = GPTP_PORT_DS(port);

	/* Unset AS-Capable if multiple responses to a pDelay request have been
	 * received.
	 */
	if (state->rcvd_pdelay_resp > 1 || state->rcvd_pdelay_follow_up > 1) {
		port_ds->as_capable = false;
	}

	if (!port_ds->ptt_port_enabled) {
		/* Make sure the timer is stopped. */
		k_timer_stop(&state->pdelay_timer);
		state->state = GPTP_PDELAY_REQ_NOT_ENABLED;
	}

	switch (state->state) {
	case GPTP_PDELAY_REQ_NOT_ENABLED:
		if (port_ds->ptt_port_enabled) {
			/* (Re)Init interval (as defined in
			 * LinkDelaySyncIntervalSetting state machine).
			 */
			port_ds->cur_log_pdelay_req_itv =
				port_ds->ini_log_pdelay_req_itv;

			gptp_set_time_itv(&port_ds->pdelay_req_itv, 1,
					  port_ds->cur_log_pdelay_req_itv);

			port_ds->compute_neighbor_rate_ratio = true;
			port_ds->compute_neighbor_prop_delay = true;

			state->pdelay_timer_expired = true;
			state->state = GPTP_PDELAY_REQ_INITIAL_SEND_REQ;
		}

		break;

	case GPTP_PDELAY_REQ_RESET:
		gptp_md_pdelay_reset(port);
		/* Send a request on the next timer expiry. */
		state->state = GPTP_PDELAY_REQ_WAIT_ITV_TIMER;
		break;

	case GPTP_PDELAY_REQ_INITIAL_SEND_REQ:
		gptp_md_start_pdelay_req(port);
		__fallthrough;

	case GPTP_PDELAY_REQ_SEND_REQ:
		if (state->tx_pdelay_req_ptr) {
			net_pkt_unref(state->tx_pdelay_req_ptr);
			state->tx_pdelay_req_ptr = NULL;
		}

		if (state->rcvd_pdelay_resp_ptr) {
			net_pkt_unref(state->rcvd_pdelay_resp_ptr);
			state->rcvd_pdelay_resp_ptr = NULL;
		}

		if (state->rcvd_pdelay_follow_up_ptr) {
			net_pkt_unref(state->rcvd_pdelay_follow_up_ptr);
			state->rcvd_pdelay_follow_up_ptr = NULL;
		}

		gptp_send_pdelay_req(port);

		k_timer_stop(&state->pdelay_timer);
		state->pdelay_timer_expired = false;
		k_timer_start(&state->pdelay_timer,
			      K_MSEC(gptp_uscaled_ns_to_timer_ms(
					     &port_ds->pdelay_req_itv)),
			      K_NO_WAIT);
		/*
		 * Transition directly to GPTP_PDELAY_REQ_WAIT_RESP.
		 * Check for the TX timestamp will be done during
		 * the computation of the path delay.
		 */
		state->state = GPTP_PDELAY_REQ_WAIT_RESP;
		break;

	case GPTP_PDELAY_REQ_WAIT_RESP:
		if (state->pdelay_timer_expired) {
			state->state = GPTP_PDELAY_REQ_RESET;
		} else if (state->rcvd_pdelay_resp != 0U) {
			pkt = state->rcvd_pdelay_resp_ptr;
			if (!gptp_handle_pdelay_resp(port, pkt)) {
				state->state = GPTP_PDELAY_REQ_WAIT_FOLLOW_UP;
			} else {
				state->state = GPTP_PDELAY_REQ_RESET;
			}
		}

		break;

	case GPTP_PDELAY_REQ_WAIT_FOLLOW_UP:
		if (state->pdelay_timer_expired) {
			state->state = GPTP_PDELAY_REQ_RESET;
		} else if (state->rcvd_pdelay_follow_up != 0U) {
			pkt = state->rcvd_pdelay_follow_up_ptr;
			if (!gptp_handle_pdelay_follow_up(port, pkt)) {
				gptp_md_pdelay_compute(port);
				state->state = GPTP_PDELAY_REQ_WAIT_ITV_TIMER;
			} else {
				state->state = GPTP_PDELAY_REQ_RESET;
			}
		}

		break;

	case GPTP_PDELAY_REQ_WAIT_ITV_TIMER:
		if (state->pdelay_timer_expired) {
			gptp_md_pdelay_check_multiple_resp(port);

			state->rcvd_pdelay_resp = 0U;
			state->rcvd_pdelay_follow_up = 0U;
		}

		break;
	}
}

static void gptp_md_pdelay_resp_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_pdelay_resp_state *state;

	state = &GPTP_PORT_STATE(port)->pdelay_resp;
	port_ds = GPTP_PORT_DS(port);

	if (!port_ds->ptt_port_enabled) {
		state->state = GPTP_PDELAY_RESP_NOT_ENABLED;
	}

	switch (state->state) {
	case GPTP_PDELAY_RESP_NOT_ENABLED:
		if (port_ds->ptt_port_enabled) {
			state->state = GPTP_PDELAY_RESP_INITIAL_WAIT_REQ;
		}

		break;

	case GPTP_PDELAY_RESP_INITIAL_WAIT_REQ:
	case GPTP_PDELAY_RESP_WAIT_REQ:
		/* Handled in gptp_handle_msg for latency considerations. */
		break;

	case GPTP_PDELAY_RESP_WAIT_TSTAMP:
		/* Handled in gptp_follow_up_callback. */
		break;
	}

}

static void gptp_md_sync_receive_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_sync_rcv_state *state;
	struct gptp_pss_rcv_state *pss_state;

	state = &GPTP_PORT_STATE(port)->sync_rcv;
	pss_state = &GPTP_PORT_STATE(port)->pss_rcv;
	port_ds = GPTP_PORT_DS(port);

	if ((!port_ds->ptt_port_enabled) || !port_ds->as_capable) {
		/* Make sure the timer is stopped. */
		k_timer_stop(&state->follow_up_discard_timer);

		/* Discard all received messages. */
		if (state->rcvd_sync_ptr) {
			net_pkt_unref(state->rcvd_sync_ptr);
			state->rcvd_sync_ptr = NULL;
		}

		if (state->rcvd_follow_up_ptr) {
			net_pkt_unref(state->rcvd_follow_up_ptr);
			state->rcvd_follow_up_ptr = NULL;
		}

		state->rcvd_sync = false;
		state->rcvd_follow_up = false;
		state->state = GPTP_SYNC_RCV_DISCARD;
		return;
	}

	switch (state->state) {
	case GPTP_SYNC_RCV_DISCARD:
	case GPTP_SYNC_RCV_WAIT_SYNC:
		if (state->rcvd_sync) {
			gptp_handle_sync(port, state->rcvd_sync_ptr);
			state->rcvd_sync = false;
			state->state = GPTP_SYNC_RCV_WAIT_FOLLOW_UP;
		} else if (state->rcvd_follow_up) {
			/* Delete late/early message. */
			if (state->rcvd_follow_up_ptr) {
				net_pkt_unref(state->rcvd_follow_up_ptr);
				state->rcvd_follow_up_ptr = NULL;
			}

			state->rcvd_follow_up = false;
		}

		break;

	case GPTP_SYNC_RCV_WAIT_FOLLOW_UP:
		/* Never received a follow up for a sync message. */
		if (state->follow_up_timeout_expired) {
			k_timer_stop(&state->follow_up_discard_timer);
			state->follow_up_timeout_expired = false;
			state->state = GPTP_SYNC_RCV_DISCARD;
			if (state->rcvd_sync_ptr) {
				net_pkt_unref(state->rcvd_sync_ptr);
				state->rcvd_sync_ptr = NULL;
			}

			state->rcvd_sync = false;
		} else if (state->rcvd_sync) {
			/* Handle received extra sync. */
			gptp_handle_sync(port, state->rcvd_sync_ptr);
			state->rcvd_sync = false;
		} else if (state->rcvd_follow_up) {
			if (!gptp_handle_follow_up(
				    port, state->rcvd_follow_up_ptr)) {

				/*
				 * Fill the structure to be sent to
				 * PortSyncSyncReceive.
				 */
				gptp_set_md_sync_receive(port,
							 &pss_state->sync_rcv);

				pss_state->rcvd_md_sync = true;

				state->state = GPTP_SYNC_RCV_WAIT_SYNC;

				/* Buffers can be released now. */
				if (state->rcvd_sync_ptr) {
					net_pkt_unref(state->rcvd_sync_ptr);
					state->rcvd_sync_ptr = NULL;
				}

				k_timer_stop(&state->follow_up_discard_timer);
				state->follow_up_timeout_expired = false;
			}
		}

		if (state->rcvd_follow_up_ptr) {
			net_pkt_unref(state->rcvd_follow_up_ptr);
			state->rcvd_follow_up_ptr = NULL;
		}

		state->rcvd_follow_up = false;
		break;
	}
}

static void gptp_md_sync_send_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_sync_send_state *state;
	struct net_pkt *pkt;

	state = &GPTP_PORT_STATE(port)->sync_send;
	port_ds = GPTP_PORT_DS(port);

	if ((!port_ds->ptt_port_enabled) || !port_ds->as_capable) {
		state->rcvd_md_sync = false;
		state->state = GPTP_SYNC_SEND_INITIALIZING;

		/* Sync sequence id is initialized in the port_ds init function
		 */
		return;
	}

	switch (state->state) {
	case GPTP_SYNC_SEND_INITIALIZING:
		state->state = GPTP_SYNC_SEND_SEND_SYNC;
		break;

	case GPTP_SYNC_SEND_SEND_SYNC:
		if (state->rcvd_md_sync) {
			pkt = gptp_prepare_sync(port);
			if (pkt) {
				/* Reference message to track timestamp info */
				state->sync_ptr = net_pkt_ref(pkt);
				gptp_md_sync_prepare(pkt,
						     state->sync_send_ptr,
						     port);
				gptp_send_sync(port, pkt);
			}

			state->rcvd_md_sync = false;
			state->state = GPTP_SYNC_SEND_SEND_FUP;
		}

		break;

	case GPTP_SYNC_SEND_SEND_FUP:
		if (state->md_sync_timestamp_avail) {
			state->md_sync_timestamp_avail = false;

			if (!state->sync_ptr) {
				NET_ERR("Sync message not available");
				break;
			}

			pkt = gptp_prepare_follow_up(port, state->sync_ptr);
			if (pkt) {
				gptp_md_follow_up_prepare(pkt,
							 state->sync_send_ptr,
							 port);
				gptp_send_follow_up(port, pkt);
			}

			net_pkt_unref(state->sync_ptr);
			state->sync_ptr = NULL;

			state->state = GPTP_SYNC_SEND_SEND_SYNC;
		}

		break;
	}
}

void gptp_md_state_machines(int port)
{
	gptp_md_pdelay_req_state_machine(port);
	gptp_md_pdelay_resp_state_machine(port);
	gptp_md_sync_receive_state_machine(port);
	gptp_md_sync_send_state_machine(port);
}

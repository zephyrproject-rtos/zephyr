/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_GPTP)
#define SYS_LOG_DOMAIN "net/gptp"
#define NET_LOG_ENABLED 1
#endif

#include <ptp_clock.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"
#include "gptp_state.h"
#include "gptp_private.h"

#if defined(CONFIG_NET_DEBUG_GPTP) &&			\
	(CONFIG_SYS_LOG_NET_LEVEL > SYS_LOG_LEVEL_INFO)
static const char * const state2str(enum gptp_port_state state)
{
	switch (state) {
	case GPTP_PORT_INITIALIZING:
		return "INITIALIZING";
	case GPTP_PORT_FAULTY:
		return "FAULTY";
	case GPTP_PORT_DISABLED:
		return "DISABLED";
	case GPTP_PORT_LISTENING:
		return "LISTENING";
	case GPTP_PORT_PRE_MASTER:
		return "PRE_MASTER";
	case GPTP_PORT_MASTER:
		return "MASTER";
	case GPTP_PORT_PASSIVE:
		return "PASSIVE";
	case GPTP_PORT_UNCALIBRATED:
		return "UNCALIBRATED";
	case GPTP_PORT_SLAVE:
		return "SLAVE";
	}

	return "<unknown>";
}
#endif

void gptp_change_port_state(int port, enum gptp_port_state state)
{
	struct gptp_global_ds *global_ds = GPTP_GLOBAL_DS();

	if (global_ds->selected_role[port] == state) {
		return;
	}

	NET_DBG("[%d] state %s -> %s", port,
		state2str(global_ds->selected_role[port]),
		state2str(state));

	global_ds->selected_role[port] = state;
};

static void gptp_mi_half_sync_itv_timeout(struct k_timer *timer)
{
	struct gptp_pss_send_state *state;
	int port;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->pss_send;
		if (&state->half_sync_itv_timer == timer) {
			if (!state->half_sync_itv_timer_expired) {
				state->half_sync_itv_timer_expired = true;
			} else {
				/* We do not need the timer anymore. */
				k_timer_stop(timer);

				state->sync_itv_timer_expired = true;
			}
		}
	}
}

static void gptp_mi_rcv_sync_receipt_timeout(struct k_timer *timer)
{
	struct gptp_pss_rcv_state *state;
	int port;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->pss_rcv;
		if (&state->rcv_sync_receipt_timeout_timer == timer) {
			state->rcv_sync_receipt_timeout_timer_expired = true;
		}

		GPTP_STATS_INC(port, sync_receipt_timeout_count);
	}
}

static void gptp_mi_send_sync_receipt_timeout(struct k_timer *timer)
{
	struct gptp_pss_send_state *state;
	int port;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->pss_send;
		if (&state->send_sync_receipt_timeout_timer == timer) {
			state->send_sync_receipt_timeout_timer_expired = true;
		}

		GPTP_STATS_INC(port, sync_receipt_timeout_count);
	}
}

static void gptp_mi_init_port_sync_sync_rcv_sm(int port)
{
	struct gptp_pss_rcv_state *pss_rcv;

	pss_rcv = &GPTP_PORT_STATE(port)->pss_rcv;
	(void)memset(pss_rcv, 0, sizeof(struct gptp_pss_rcv_state));

	k_timer_init(&pss_rcv->rcv_sync_receipt_timeout_timer,
		     gptp_mi_rcv_sync_receipt_timeout, NULL);

	pss_rcv->state = GPTP_PSS_RCV_DISCARD;
}

static void gptp_mi_init_port_sync_sync_send_sm(int port)
{
	struct gptp_pss_send_state *pss_send;

	pss_send = &GPTP_PORT_STATE(port)->pss_send;
	(void)memset(pss_send, 0, sizeof(struct gptp_pss_send_state));

	k_timer_init(&pss_send->half_sync_itv_timer,
		     gptp_mi_half_sync_itv_timeout, NULL);
	k_timer_init(&pss_send->send_sync_receipt_timeout_timer,
		     gptp_mi_send_sync_receipt_timeout, NULL);

	pss_send->state = GPTP_PSS_SEND_TRANSMIT_INIT;
}

static void gptp_mi_init_site_sync_sync_sm(void)
{
	struct gptp_site_sync_sync_state *site_ss;

	site_ss = &GPTP_STATE()->site_ss;
	(void)memset(site_ss, 0, sizeof(struct gptp_site_sync_sync_state));
	site_ss->state = GPTP_SSS_INITIALIZING;
}

static void gptp_mi_init_clock_slave_sync_sm(void)
{
	struct gptp_clk_slave_sync_state *clk_ss;

	clk_ss = &GPTP_STATE()->clk_slave_sync;
	(void)memset(clk_ss, 0, sizeof(struct gptp_clk_slave_sync_state));
	clk_ss->state = GPTP_CLK_SLAVE_SYNC_INITIALIZING;
}

static void gptp_mi_init_port_announce_rcv_sm(int port)
{
	struct gptp_port_announce_receive_state *pa_rcv;

	pa_rcv = &GPTP_PORT_STATE(port)->pa_rcv;
	(void)memset(pa_rcv, 0,
		     sizeof(struct gptp_port_announce_receive_state));
	pa_rcv->state = GPTP_PA_RCV_DISCARD;
}

static void gptp_mi_init_clock_master_sync_rcv_sm(void)
{
	struct gptp_clk_master_sync_state *cms_rcv;

	cms_rcv = &GPTP_STATE()->clk_master_sync_receive;
	(void)memset(cms_rcv, 0, sizeof(struct gptp_clk_master_sync_state));
	cms_rcv->state = GPTP_CMS_RCV_INITIALIZING;
}

static void announce_timer_handler(struct k_timer *timer)
{
	int port;
	struct gptp_port_announce_information_state *state;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->pa_info;
		if (&state->ann_rcpt_expiry_timer == timer) {
			state->ann_expired = true;
			GPTP_STATS_INC(port, announce_receipt_timeout_count);
			break;
		}
	}
}

static void gptp_mi_init_port_announce_info_sm(int port)
{
	struct gptp_port_announce_information_state *state;

	state = &GPTP_PORT_STATE(port)->pa_info;

	k_timer_init(&state->ann_rcpt_expiry_timer,
		     announce_timer_handler, NULL);

	state->ann_expired = false;
	state->state = GPTP_PA_INFO_DISABLED;
}

static void gptp_mi_init_bmca_data(int port)
{
	struct gptp_port_bmca_data *bmca_data;

	bmca_data = GPTP_PORT_BMCA_DATA(port);

	(void)memset(bmca_data, 0, sizeof(struct gptp_port_bmca_data));

	gptp_set_time_itv(&bmca_data->announce_interval, 1,
			  CONFIG_NET_GPTP_INIT_LOG_ANNOUNCE_ITV);

	(void)memset(&bmca_data->port_priority, 0xFF,
		     sizeof(struct gptp_priority_vector));
	(void)memset(&bmca_data->master_priority, 0xFF,
		     sizeof(struct gptp_priority_vector));
}

static void announce_periodic_timer_handler(struct k_timer *timer)
{
	int port;
	struct gptp_port_announce_transmit_state *state;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		state = &GPTP_PORT_STATE(port)->pa_transmit;
		if (&state->ann_send_periodic_timer == timer) {
			state->ann_trigger = true;
			break;
		}
	}
}

static void gptp_mi_init_port_announce_transmit_sm(int port)
{
	struct gptp_port_announce_transmit_state *state;

	state = &GPTP_PORT_STATE(port)->pa_transmit;

	k_timer_init(&state->ann_send_periodic_timer,
		     announce_periodic_timer_handler, NULL);

	state->ann_trigger = false;
	state->state = GPTP_PA_TRANSMIT_INIT;
}

static void gptp_mi_init_port_role_selection_sm(void)
{
	GPTP_STATE()->pr_sel.state = GPTP_PR_SELECTION_INIT_BRIDGE;
}

void gptp_mi_init_state_machine(void)
{
	int port;

	for (port = GPTP_PORT_START;
	     port < (GPTP_PORT_START + CONFIG_NET_GPTP_NUM_PORTS); port++) {
		gptp_mi_init_port_sync_sync_rcv_sm(port);
		gptp_mi_init_port_sync_sync_send_sm(port);
		gptp_mi_init_port_announce_rcv_sm(port);
		gptp_mi_init_port_announce_info_sm(port);
		gptp_mi_init_port_announce_transmit_sm(port);
		gptp_mi_init_bmca_data(port);
	}

	gptp_mi_init_site_sync_sync_sm();
	gptp_mi_init_clock_slave_sync_sm();
	gptp_mi_init_port_role_selection_sm();
	gptp_mi_init_clock_master_sync_rcv_sm();
}

u64_t gptp_get_current_time_nanosecond(int port)
{
	struct device *clk;

	clk = net_eth_get_ptp_clock(GPTP_PORT_IFACE(port));
	if (clk) {
		struct net_ptp_time tm = {};

		ptp_clock_get(clk, &tm);

		if (tm.second == 0 && tm.nanosecond == 0) {
			goto use_uptime;
		}

		return gptp_timestamp_to_nsec(&tm);
	}

use_uptime:
	/* A workaround if clock cannot be found. Note that accuracy is
	 * only in milliseconds.
	 */
	return k_uptime_get() * 1000000;
}

static void gptp_mi_pss_rcv_compute(int port)
{
	struct gptp_pss_rcv_state *state;
	struct gptp_mi_port_sync_sync *pss;
	struct gptp_md_sync_info *sync_rcv;
	struct gptp_port_ds *port_ds;

	state = &GPTP_PORT_STATE(port)->pss_rcv;
	pss = &state->pss;
	sync_rcv = &state->sync_rcv;
	port_ds = GPTP_PORT_DS(port);

	state->rate_ratio = sync_rcv->rate_ratio;
	state->rate_ratio += (port_ds->neighbor_rate_ratio - 1.0);

	port_ds->sync_receipt_timeout_time_itv = port_ds->sync_receipt_timeout;
	port_ds->sync_receipt_timeout_time_itv *= NSEC_PER_SEC;
	port_ds->sync_receipt_timeout_time_itv *=
		GPTP_POW2(sync_rcv->log_msg_interval);

	pss->local_port_number = port;

	memcpy(&pss->sync_info, sync_rcv, sizeof(struct gptp_md_sync_info));

	pss->sync_receipt_timeout_time = gptp_get_current_time_nanosecond(port);
	pss->sync_receipt_timeout_time +=
		port_ds->sync_receipt_timeout_time_itv;

	pss->sync_info.rate_ratio = state->rate_ratio;
}

static void start_rcv_sync_timer(struct gptp_port_ds *port_ds,
				 struct gptp_pss_rcv_state *state)
{
	s32_t duration;

	duration = port_ds->sync_receipt_timeout_time_itv;

	k_timer_start(&state->rcv_sync_receipt_timeout_timer, duration, 0);
}

static void gptp_mi_pss_rcv_state_machine(int port)
{
	struct gptp_pss_rcv_state *state;
	struct gptp_site_sync_sync_state *site_ss_state;
	struct gptp_port_ds *port_ds;

	state = &GPTP_PORT_STATE(port)->pss_rcv;
	site_ss_state = &GPTP_STATE()->site_ss;
	port_ds = GPTP_PORT_DS(port);

	if ((!port_ds->ptt_port_enabled) || !port_ds->as_capable) {
		state->rcvd_md_sync = false;
		state->state = GPTP_PSS_RCV_DISCARD;
		return;
	}

	switch (state->state) {
	case GPTP_PSS_RCV_DISCARD:
		k_timer_stop(&state->rcv_sync_receipt_timeout_timer);
		state->rcv_sync_receipt_timeout_timer_expired = false;

		/* Fallthrough. */
	case GPTP_PSS_RCV_RECEIVED_SYNC:
		if (state->rcvd_md_sync) {
			state->rcvd_md_sync = false;
			gptp_mi_pss_rcv_compute(port);

			state->state = GPTP_PSS_RCV_RECEIVED_SYNC;

			site_ss_state->pss_rcv_ptr = &state->pss;
			site_ss_state->rcvd_pss = true;

			k_timer_stop(&state->rcv_sync_receipt_timeout_timer);
			state->rcv_sync_receipt_timeout_timer_expired = false;

			if (GPTP_GLOBAL_DS()->gm_present) {
				start_rcv_sync_timer(port_ds, state);
			}
		}

		break;
	}
}

static void gptp_mi_pss_store_last_pss(int port)
{
	struct gptp_pss_send_state *state;
	struct gptp_mi_port_sync_sync *pss_ptr;
	struct gptp_md_sync_info *sync_info;
	struct gptp_port_ds *port_ds;

	state = &GPTP_PORT_STATE(port)->pss_send;
	port_ds = GPTP_PORT_DS(port);
	pss_ptr = state->pss_sync_ptr;
	sync_info = &pss_ptr->sync_info;

	state->last_rcvd_port_num = pss_ptr->local_port_number;

	memcpy(&state->last_precise_orig_ts, &sync_info->precise_orig_ts,
	       sizeof(struct net_ptp_time));
	memcpy(&state->last_gm_phase_change, &sync_info->last_gm_phase_change,
	       sizeof(struct gptp_scaled_ns));

	state->last_follow_up_correction_field =
		sync_info->follow_up_correction_field;
	state->last_rate_ratio = sync_info->rate_ratio;
	state->last_upstream_tx_time = sync_info->upstream_tx_time;
	state->last_gm_time_base_indicator = sync_info->gm_time_base_indicator;
	state->last_gm_freq_change = sync_info->last_gm_freq_change;
}

static void gptp_mi_pss_send_md_sync_send(int port)
{
	struct gptp_pss_send_state *state;
	struct gptp_mi_port_sync_sync *pss_ptr;
	struct gptp_port_ds *port_ds;
	struct gptp_sync_send_state *sync_send;

	state = &GPTP_PORT_STATE(port)->pss_send;
	port_ds = GPTP_PORT_DS(port);
	pss_ptr = state->pss_sync_ptr;
	sync_send = &GPTP_PORT_STATE(port)->sync_send;

	memcpy(&state->sync_send, &pss_ptr->sync_info,
	       sizeof(struct gptp_md_sync_info));

	sync_send->sync_send_ptr = &state->sync_send;
	sync_send->rcvd_md_sync = true;
}

static void gptp_mi_pss_send_state_machine(int port)
{
	struct gptp_pss_send_state *state;
	struct gptp_port_ds *port_ds;
	struct gptp_global_ds *global_ds;
	s32_t duration;

	global_ds = GPTP_GLOBAL_DS();
	state = &GPTP_PORT_STATE(port)->pss_send;
	port_ds = GPTP_PORT_DS(port);

	/* Reset interval as defined in LinkDelaySyncIntervalSetting state
	 * machine.
	 */
	if (port_ds->ptt_port_enabled && !port_ds->prev_ptt_port_enabled) {
		gptp_update_sync_interval(port, GPTP_ITV_SET_TO_INIT);
	}

	if (state->rcvd_pss_sync && ((!port_ds->ptt_port_enabled) ||
				     !port_ds->as_capable)) {
		state->rcvd_pss_sync = false;
		state->state = GPTP_PSS_SEND_TRANSMIT_INIT;

		return;
	}

	switch (state->state) {
	case GPTP_PSS_SEND_TRANSMIT_INIT:
	case GPTP_PSS_SEND_SYNC_RECEIPT_TIMEOUT:
		if (state->rcvd_pss_sync &&
		    (state->pss_sync_ptr->local_port_number != port) &&
		    (global_ds->selected_role[port] == GPTP_PORT_MASTER)) {
			state->state = GPTP_PSS_SEND_SEND_MD_SYNC;
		} else {
			break;
		}

		/* Fallthrough. */
	case GPTP_PSS_SEND_SEND_MD_SYNC:
		if (state->rcvd_pss_sync) {
			gptp_mi_pss_store_last_pss(port);
			state->rcvd_pss_sync = false;
		}

		/* Make sure no previous timer is still running. */
		k_timer_stop(&state->half_sync_itv_timer);
		k_timer_stop(&state->send_sync_receipt_timeout_timer);

		state->half_sync_itv_timer_expired = false;
		state->sync_itv_timer_expired = false;
		state->send_sync_receipt_timeout_timer_expired = false;

		/* Convert ns to ms. */
		duration = gptp_uscaled_ns_to_timer_ms(
						&port_ds->half_sync_itv);

		/* Start 0.5 * syncInterval timeout timer. */
		k_timer_start(&state->half_sync_itv_timer,
			      duration, duration);

		gptp_mi_pss_send_md_sync_send(port);

		/* Fallthrough. */
	case GPTP_PSS_SEND_SET_SYNC_RECEIPT_TIMEOUT:
		/* Test conditions have been slightly rearranged compared to
		 * their definitions in the standard in order not to test
		 * AsCapable and pttPortEnabled when not needed (they are
		 * already tested with rcvdPSSync for the reset of this state
		 * machine).
		 */
		if ((global_ds->selected_role[port] == GPTP_PORT_MASTER) &&
		    ((state->rcvd_pss_sync &&
		      state->half_sync_itv_timer_expired &&
		      state->pss_sync_ptr->local_port_number != port) ||
		     (state->sync_itv_timer_expired &&
		      (state->last_rcvd_port_num != port) &&
		      port_ds->as_capable && port_ds->ptt_port_enabled))) {

			state->state = GPTP_PSS_SEND_SEND_MD_SYNC;

		} else if ((state->state == GPTP_PSS_SEND_SEND_MD_SYNC) ||
			   (state->rcvd_pss_sync &&
			    !state->sync_itv_timer_expired &&
			    (global_ds->selected_role[port] ==
							GPTP_PORT_MASTER) &&
			    state->pss_sync_ptr->local_port_number != port)) {

			/* Change state as it may have transitioned from
			 * SEND_MD_SYNC.
			 */
			state->state = GPTP_PSS_SEND_SET_SYNC_RECEIPT_TIMEOUT;

			/* Stop and (re)start receipt timeout timer. */
			k_timer_stop(&state->send_sync_receipt_timeout_timer);
			state->send_sync_receipt_timeout_timer_expired = false;

			duration = (state->last_sync_receipt_timeout_time -
				    gptp_get_current_time_nanosecond(port)) /
				(NSEC_PER_USEC * USEC_PER_MSEC);

			k_timer_start(&state->send_sync_receipt_timeout_timer,
				      duration, 0);

		} else if (state->send_sync_receipt_timeout_timer_expired) {
			state->state = GPTP_PSS_SEND_SYNC_RECEIPT_TIMEOUT;
		}

		break;
	}
}

static void gptp_mi_site_ss_prepare_pss_send(void)
{
	struct gptp_site_sync_sync_state *state;

	state = &GPTP_STATE()->site_ss;

	memcpy(&state->pss_send, state->pss_rcv_ptr,
	       sizeof(struct gptp_mi_port_sync_sync));
}

static void gptp_mi_site_ss_send_to_pss(void)
{
	struct gptp_site_sync_sync_state *state;
	struct gptp_pss_send_state *pss_send;
	int port;

	state = &GPTP_STATE()->site_ss;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		pss_send = &GPTP_PORT_STATE(port)->pss_send;
		pss_send->pss_sync_ptr = &state->pss_send;
		pss_send->rcvd_pss_sync = true;
	}
}

static void gptp_mi_site_sync_sync_state_machine(void)
{
	bool gm_present;
	u16_t local_port_number;
	struct gptp_site_sync_sync_state *state;
	struct gptp_clk_slave_sync_state *clk_ss;

	state = &GPTP_STATE()->site_ss;
	clk_ss = &GPTP_STATE()->clk_slave_sync;
	gm_present = GPTP_GLOBAL_DS()->gm_present;

	if (!state->pss_rcv_ptr) {
		/* We do not have connection to GM yet */
		return;
	}

	local_port_number = state->pss_rcv_ptr->local_port_number;

	switch (state->state) {
	case GPTP_SSS_INITIALIZING:
		state->rcvd_pss = false;
		state->state = GPTP_SSS_RECEIVING_SYNC;
		break;

	case GPTP_SSS_RECEIVING_SYNC:
		if (state->rcvd_pss) {
			state->rcvd_pss = false;
			if (gptp_is_slave_port(local_port_number) &&
					gm_present) {
				gptp_mi_site_ss_prepare_pss_send();

				/*
				 * Send Port Sync Sync to all
				 * PortSyncSyncSend State Machines.
				 */
				gptp_mi_site_ss_send_to_pss();

				/*
				 * Send PortSyncSync to
				 * ClockSlaveSync State Machine.
				 */
				clk_ss->pss_rcv_ptr = &state->pss_send;
				clk_ss->rcvd_pss = true;
			}
		}

		break;
	}
}

static void gptp_mi_clk_slave_sync_compute(void)
{
	struct gptp_clk_slave_sync_state *state;
	struct gptp_global_ds *global_ds;
	struct gptp_md_sync_info *pss;
	struct gptp_port_ds *port_ds;
	u64_t sync_receipt_time;

	state = &GPTP_STATE()->clk_slave_sync;
	global_ds = GPTP_GLOBAL_DS();
	port_ds = GPTP_PORT_DS(state->pss_rcv_ptr->local_port_number);

	pss = &state->pss_rcv_ptr->sync_info;

	sync_receipt_time = pss->rate_ratio;
	sync_receipt_time /= port_ds->neighbor_rate_ratio;
	sync_receipt_time *= port_ds->neighbor_prop_delay;
	sync_receipt_time += pss->follow_up_correction_field;
	sync_receipt_time += port_ds->delay_asymmetry;

	global_ds->sync_receipt_time.second = sync_receipt_time / NSEC_PER_SEC;
	global_ds->sync_receipt_time.nanosecond =
		sync_receipt_time % NSEC_PER_SEC;
	global_ds->sync_receipt_time.second += pss->precise_orig_ts.second;
	global_ds->sync_receipt_time.nanosecond +=
		pss->precise_orig_ts.nanosecond;

	global_ds->sync_receipt_local_time = port_ds->delay_asymmetry;
	global_ds->sync_receipt_local_time /= pss->rate_ratio;
	global_ds->sync_receipt_local_time +=
		(port_ds->neighbor_prop_delay / port_ds->neighbor_rate_ratio);
	global_ds->sync_receipt_local_time += pss->upstream_tx_time;

	global_ds->gm_time_base_indicator = pss->gm_time_base_indicator;
	global_ds->last_gm_phase_change.high = pss->last_gm_phase_change.high;
	global_ds->last_gm_phase_change.low = pss->last_gm_phase_change.low;
	global_ds->last_gm_freq_change = pss->last_gm_freq_change;
}

#if defined(CONFIG_NET_GPTP_USE_DEFAULT_CLOCK_UPDATE)
static void gptp_update_local_port_clock(void)
{
	struct gptp_clk_slave_sync_state *state;
	struct gptp_global_ds *global_ds;
	struct gptp_port_ds *port_ds;
	int port;
	s64_t nanosecond_diff;
	s64_t second_diff;
	struct device *clk;
	struct net_ptp_time tm;
	int key;

	state = &GPTP_STATE()->clk_slave_sync;
	global_ds = GPTP_GLOBAL_DS();
	port = state->pss_rcv_ptr->local_port_number;
	NET_ASSERT((port >= GPTP_PORT_START) && (port <= GPTP_PORT_END));

	port_ds = GPTP_PORT_DS(port);

	/* Check if the last neighbor rate ratio can still be used */
	if (!port_ds->neighbor_rate_ratio_valid) {
		return;
	}

	port_ds->neighbor_rate_ratio_valid = false;

	second_diff = global_ds->sync_receipt_time.second -
		(global_ds->sync_receipt_local_time / NSEC_PER_SEC);
	nanosecond_diff = global_ds->sync_receipt_time.nanosecond -
		(global_ds->sync_receipt_local_time % NSEC_PER_SEC);

	clk = net_eth_get_ptp_clock(GPTP_PORT_IFACE(port));
	if (!clk) {
		return;
	}

	if (second_diff > 0 && nanosecond_diff < 0) {
		second_diff--;
		nanosecond_diff = NSEC_PER_SEC + nanosecond_diff;
	}

	if (second_diff < 0 && nanosecond_diff > 0) {
		second_diff++;
		nanosecond_diff = -NSEC_PER_SEC + nanosecond_diff;
	}

	ptp_clock_rate_adjust(clk, port_ds->neighbor_rate_ratio);

	/* If time difference is too high, set the clock value.
	 * Otherwise, adjust it.
	 */
	if (second_diff || (second_diff == 0 &&
			    (nanosecond_diff < -5000 ||
			     nanosecond_diff > 5000))) {
		bool underflow = false;

		key = irq_lock();
		ptp_clock_get(clk, &tm);

		tm.second += second_diff;

		if (nanosecond_diff < 0 &&
		    tm.nanosecond < -nanosecond_diff) {
			underflow = true;
		}

		tm.nanosecond += nanosecond_diff;

		if (underflow) {
			tm.second--;
			tm.nanosecond += NSEC_PER_SEC;
		} else if (tm.nanosecond >= NSEC_PER_SEC) {
			tm.second++;
			tm.nanosecond -= NSEC_PER_SEC;
		}

		ptp_clock_set(clk, &tm);
		irq_unlock(key);
	} else {
		if (nanosecond_diff < -200) {
			nanosecond_diff = -200;
		} else if (nanosecond_diff > 200) {
			nanosecond_diff = 200;
		}

		ptp_clock_adjust(clk, nanosecond_diff);
	}
}
#endif /* CONFIG_NET_GPTP_USE_DEFAULT_CLOCK_UPDATE */

static void gptp_mi_clk_slave_sync_state_machine(void)
{
	struct gptp_clk_slave_sync_state *state;

	state = &GPTP_STATE()->clk_slave_sync;

	switch (state->state) {
	case GPTP_CLK_SLAVE_SYNC_INITIALIZING:
		state->rcvd_pss = false;
		state->state = GPTP_CLK_SLAVE_SYNC_SEND_SYNC_IND;
		break;

	case GPTP_CLK_SLAVE_SYNC_SEND_SYNC_IND:
		if (state->rcvd_pss) {
			state->rcvd_pss = false;
			gptp_mi_clk_slave_sync_compute();

#if defined(CONFIG_NET_GPTP_USE_DEFAULT_CLOCK_UPDATE)
			/* Instead of updating SlaveClock, update LocalClock */
			gptp_update_local_port_clock();
#endif
			gptp_call_phase_dis_cb();
		}

		break;
	}
}

static void gptp_mi_clk_master_sync_rcv_state_machine(void)
{
	struct gptp_clk_master_sync_state *state;

	state = &GPTP_STATE()->clk_master_sync_receive;
	switch (state->state) {
	case GPTP_CMS_RCV_INITIALIZING:
		state->state = GPTP_CMS_RCV_WAITING;
		break;

	case GPTP_CMS_RCV_WAITING:
		if (state->rcvd_clock_source_req ||
		    state->rcvd_local_clock_tick) {
			state->state = GPTP_CMS_RCV_SOURCE_TIME;
		}

		break;

	case GPTP_CMS_RCV_SOURCE_TIME:
		/* TODO:
		 *     updateMasterTime();
		 *     localTime = currentTime;
		 */
		if (state->rcvd_clock_source_req) {
			/* TODO:
			 *    computeGMRateRatio();
			 *    Update:
			 *    clockSourceTimeBaseIndicatorOld;
			 *    clockSourceTimeBaseIndicator
			 *    clockSourceLastGmPhaseChange
			 *    clockSourceLastGmFreqChange
			 */
		}

		state->rcvd_clock_source_req = false;
		state->rcvd_local_clock_tick = false;
		state->state = GPTP_CMS_RCV_WAITING;
		break;

	default:
		NET_ERR("Unrecognised state %d", state->state);
		break;
	}
}

static void copy_path_trace(struct gptp_announce *announce)
{
	int len = ntohs(announce->tlv.len);
	struct gptp_path_trace *sys_path_trace;

	if (len > GPTP_MAX_PATHTRACE_SIZE) {
		NET_ERR("Too long path trace (%d vs %d)",
			GPTP_MAX_PATHTRACE_SIZE, len);
		return;
	}

	sys_path_trace = &GPTP_GLOBAL_DS()->path_trace;

	sys_path_trace->len = htons(len + GPTP_CLOCK_ID_LEN);

	memcpy(sys_path_trace->path_sequence, announce->tlv.path_sequence,
	       len);

	/* Append local clockIdentity. */
	memcpy((u8_t *)sys_path_trace->path_sequence + len,
	       GPTP_DEFAULT_DS()->clk_id, GPTP_CLOCK_ID_LEN);
}

static bool gptp_mi_qualify_announce(int port, struct net_pkt *announce_msg)
{
	struct gptp_announce *announce;
	struct gptp_hdr *hdr;
	int i;
	u16_t len;

	hdr = GPTP_HDR(announce_msg);
	announce = GPTP_ANNOUNCE(announce_msg);

	if (memcmp(hdr->port_id.clk_id, GPTP_DEFAULT_DS()->clk_id,
		   GPTP_CLOCK_ID_LEN) == 0) {
		return false;
	}

	len = ntohs(announce->steps_removed);
	if (len >= 255) {
		return false;
	}

	for (i = 0; i < len + 1; i++) {
		if (memcmp(announce->tlv.path_sequence[i],
			   GPTP_DEFAULT_DS()->clk_id,
			   GPTP_CLOCK_ID_LEN) == 0) {
			return false;
		}
	}

	if (GPTP_GLOBAL_DS()->selected_role[port] == GPTP_PORT_SLAVE) {
		copy_path_trace(announce);
	}

	return true;
}

static void gptp_mi_port_announce_receive_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_port_announce_receive_state *state;
	struct gptp_port_bmca_data *bmca_data;

	state = &GPTP_PORT_STATE(port)->pa_rcv;
	port_ds = GPTP_PORT_DS(port);
	bmca_data = GPTP_PORT_BMCA_DATA(port);

	if ((!port_ds->ptt_port_enabled) || (!port_ds->as_capable)) {
		state->state = GPTP_PA_RCV_DISCARD;
	}

	switch (state->state) {
	case GPTP_PA_RCV_DISCARD:
		state->rcvd_announce = false;
		bmca_data->rcvd_msg = false;
		if (bmca_data->rcvd_announce_ptr != NULL) {
			net_pkt_unref(bmca_data->rcvd_announce_ptr);
			bmca_data->rcvd_announce_ptr = NULL;
		}

		state->state = GPTP_PA_RCV_RECEIVE;
		break;

	case GPTP_PA_RCV_RECEIVE:
		/* "portEnabled" is not checked: the interface is always up. */
		if (state->rcvd_announce &&
		    port_ds->ptt_port_enabled &&
		    port_ds->as_capable	&&
		    !bmca_data->rcvd_msg) {
			state->rcvd_announce = false;

			bmca_data->rcvd_msg = gptp_mi_qualify_announce(
				port, bmca_data->rcvd_announce_ptr);
			if (!bmca_data->rcvd_msg) {
				net_pkt_unref(bmca_data->rcvd_announce_ptr);
				bmca_data->rcvd_announce_ptr = NULL;
			}
		}

		break;
	}
}

/*
 * Compare a vector to an announce message vector.
 * All must be in big endian (network) order.
 */
static enum gptp_received_info compare_priority_vectors(
		struct gptp_priority_vector *vector,
		struct net_pkt *pkt, int port)
{
	struct gptp_hdr *hdr;
	struct gptp_announce *announce;
	struct gptp_port_bmca_data *bmca_data;
	int rsi_cmp, spi_cmp, port_cmp;

	bmca_data = GPTP_PORT_BMCA_DATA(port);
	hdr = GPTP_HDR(pkt);
	announce = GPTP_ANNOUNCE(pkt);

	/* Compare rootSystemIdentity and stepsRemoved. */
	rsi_cmp = memcmp(&announce->root_system_id,
			 &vector->root_system_id,
			 sizeof(struct gptp_root_system_identity) +
			 sizeof(u16_t));
	if (rsi_cmp < 0) {
		/* Better rootSystemIdentity. */
		return GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO;
	}

	/* Compare sourcePortIdentity. */
	spi_cmp = memcmp(&hdr->port_id, &vector->src_port_id,
			 sizeof(struct gptp_port_identity));

	port_cmp = (int)port - ntohs(vector->port_number);

	if (spi_cmp == 0) {
		if (rsi_cmp == 0) {
			if (port_cmp == 0) {
				/* Same priority vector. */
				return GPTP_RCVD_INFO_REPEATED_MASTER_INFO;
			} else if (port_cmp < 0) {
				/* Priority vector with better reception port
				 * number.
				 */
				return GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO;
			}
		} else {
			/* Same master port but different Grand Master. */
			return GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO;
		}
	} else if ((spi_cmp < 0) && (rsi_cmp == 0)) {
		/* Same Grand Master but better masterPort. */
		return GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO;
	}

	return GPTP_RCVD_INFO_INFERIOR_MASTER_INFO;
}

static enum gptp_received_info rcv_info(int port)
{
	/* TODO
	 * How can we define that a message does not convey the port
	 * role Master port ?
	 * It is needed to define that to be able to send
	 * GPTP_RCVD_INFO_OTHER_INFO.
	 */
	struct gptp_port_bmca_data *bmca_data;
	struct gptp_announce *announce;

	bmca_data = GPTP_PORT_BMCA_DATA(port);
	announce = GPTP_ANNOUNCE(bmca_data->rcvd_announce_ptr);

	bmca_data->message_steps_removed = announce->steps_removed;

	return compare_priority_vectors(&bmca_data->port_priority,
					bmca_data->rcvd_announce_ptr,
					port);
}

static void record_other_announce_info(int port)
{
	struct gptp_hdr *hdr;
	struct gptp_announce *announce;
	struct gptp_port_bmca_data *bmca_data;

	bmca_data = GPTP_PORT_BMCA_DATA(port);
	hdr = GPTP_HDR(bmca_data->rcvd_announce_ptr);
	announce = GPTP_ANNOUNCE(bmca_data->rcvd_announce_ptr);

	/* Copy leap61, leap59, current UTC offset valid, time traceable and
	 * frequency traceable flags.
	 */
	bmca_data->ann_flags.octets[1] = hdr->flags.octets[1];

	bmca_data->ann_current_utc_offset = ntohs(announce->cur_utc_offset);
	bmca_data->ann_time_source = announce->time_source;
}

static void copy_priority_vector(struct gptp_priority_vector *vector,
				 struct net_pkt *pkt, int port)
{
	struct gptp_hdr *hdr;
	struct gptp_announce *announce;

	hdr = GPTP_HDR(pkt);
	announce = GPTP_ANNOUNCE(pkt);

	memcpy(&vector->root_system_id, &announce->root_system_id,
	       sizeof(struct gptp_root_system_identity) + sizeof(u16_t));

	memcpy(&vector->src_port_id, &hdr->port_id,
	       sizeof(struct gptp_port_identity));

	vector->port_number = htons(port);
}

static void gptp_mi_port_announce_information_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_global_ds *global_ds;
	struct gptp_port_announce_information_state *state;
	struct gptp_announce *announce;
	struct gptp_hdr *hdr;
	struct gptp_port_bmca_data *bmca_data;
	struct gptp_pss_rcv_state *pss_rcv;

	bmca_data = GPTP_PORT_BMCA_DATA(port);
	state = &GPTP_PORT_STATE(port)->pa_info;
	port_ds = GPTP_PORT_DS(port);
	global_ds = GPTP_GLOBAL_DS();

	if ((!port_ds->ptt_port_enabled || !port_ds->as_capable) &&
	    (bmca_data->info_is != GPTP_INFO_IS_DISABLED)) {
		state->state = GPTP_PA_INFO_DISABLED;
	}

	switch (state->state) {
	case GPTP_PA_INFO_DISABLED:
		bmca_data->rcvd_msg = false;
		bmca_data->info_is = GPTP_INFO_IS_DISABLED;
		SET_RESELECT(global_ds, port);
		CLEAR_SELECTED(global_ds, port);
		state->state = GPTP_PA_INFO_POST_DISABLED;
		k_timer_stop(&state->ann_rcpt_expiry_timer);
		state->ann_expired = true;
		/* Fallthrough. */

	case GPTP_PA_INFO_POST_DISABLED:
		if (port_ds->ptt_port_enabled && port_ds->as_capable) {
			state->state = GPTP_PA_INFO_AGED;
		} else if (bmca_data->rcvd_msg) {
			state->state = GPTP_PA_INFO_DISABLED;
		}

		break;

	case GPTP_PA_INFO_AGED:
		bmca_data->info_is = GPTP_INFO_IS_AGED;
		CLEAR_SELECTED(global_ds, port);
		SET_RESELECT(global_ds, port);
		/* Transition will be actually tested in UPDATE state. */
		state->state = GPTP_PA_INFO_UPDATE;
		break;

	case GPTP_PA_INFO_UPDATE:
		if (IS_SELECTED(global_ds, port) && bmca_data->updt_info) {
			memcpy(&bmca_data->port_priority,
			       &bmca_data->master_priority,
			       sizeof(struct gptp_priority_vector));

			bmca_data->port_steps_removed =
				global_ds->master_steps_removed;
			bmca_data->updt_info = false;
			bmca_data->info_is = GPTP_INFO_IS_MINE;
			bmca_data->new_info = true;
			state->state = GPTP_PA_INFO_CURRENT;
		}

		break;

	case GPTP_PA_INFO_CURRENT:
		pss_rcv = &GPTP_PORT_STATE(port)->pss_rcv;
		if (IS_SELECTED(global_ds, port) && bmca_data->updt_info) {
			state->state = GPTP_PA_INFO_UPDATE;
		} else if (bmca_data->rcvd_msg && !bmca_data->updt_info) {
			state->state = GPTP_PA_INFO_RECEIVE;
		} else if ((bmca_data->info_is == GPTP_INFO_IS_RECEIVED) &&
			   !bmca_data->updt_info &&
			   !bmca_data->rcvd_msg &&
			   (state->ann_expired ||
			    (global_ds->gm_present &&
			   pss_rcv->rcv_sync_receipt_timeout_timer_expired))) {
			state->state = GPTP_PA_INFO_AGED;
		}

		break;

	case GPTP_PA_INFO_RECEIVE:
		switch (rcv_info(port)) {
		case GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO:
			state->state = GPTP_PA_INFO_SUPERIOR_MASTER_PORT;
			break;
		case GPTP_RCVD_INFO_REPEATED_MASTER_INFO:
			state->state = GPTP_PA_INFO_REPEATED_MASTER_PORT;
			break;
		case GPTP_RCVD_INFO_INFERIOR_MASTER_INFO:
			/* Fallthrough. */
		case GPTP_RCVD_INFO_OTHER_INFO:
			state->state =
				GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT;
			break;
		}

		break;

	case GPTP_PA_INFO_SUPERIOR_MASTER_PORT:
		/* We copy directly the content of the message to the port
		 * priority vector without using an intermediate
		 * messagePrioriry structure.
		 */

		if (!bmca_data->rcvd_announce_ptr) {
			/* Shouldn't be reached. Checked for safety reason. */
			bmca_data->rcvd_msg = false;
			state->state = GPTP_PA_INFO_CURRENT;
			break;
		}

		copy_priority_vector(&bmca_data->port_priority,
				     bmca_data->rcvd_announce_ptr, port);

		announce = GPTP_ANNOUNCE(bmca_data->rcvd_announce_ptr);
		bmca_data->port_steps_removed = ntohs(announce->steps_removed);
		record_other_announce_info(port);
		hdr = GPTP_HDR(bmca_data->rcvd_announce_ptr);
		gptp_set_time_itv(&bmca_data->ann_rcpt_timeout_time_interval,
				  port_ds->announce_receipt_timeout,
				  hdr->log_msg_interval);
		bmca_data->info_is = GPTP_INFO_IS_RECEIVED;
		CLEAR_SELECTED(global_ds, port);
		SET_RESELECT(global_ds, port);
		/* Fallthrough. */

	case GPTP_PA_INFO_REPEATED_MASTER_PORT:
		k_timer_stop(&state->ann_rcpt_expiry_timer);
		state->ann_expired = false;
		k_timer_start(&state->ann_rcpt_expiry_timer,
			      gptp_uscaled_ns_to_timer_ms(
				   &bmca_data->ann_rcpt_timeout_time_interval),
			      0);
		/* Fallthrough. */

	case GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT:
		if (bmca_data->rcvd_announce_ptr != NULL) {
			net_pkt_unref(bmca_data->rcvd_announce_ptr);
			bmca_data->rcvd_announce_ptr = NULL;
		}

		bmca_data->rcvd_msg = false;
		state->state = GPTP_PA_INFO_CURRENT;
		break;
	}
}

static void gptp_updt_role_disabled_tree(void)
{
	struct gptp_global_ds *global_ds;
	int port;

	global_ds = GPTP_GLOBAL_DS();

	/* Set all elements of the selectedRole array to DisabledPort. */
	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		gptp_change_port_state(port, GPTP_PORT_DISABLED);
	}

	/* Set lastGmPriority to all ones. */
	(void)memset(&global_ds->last_gm_priority, 0xFF,
		     sizeof(struct gptp_priority_vector));

	/* Set pathTrace array to contain the single element thisClock. */
	global_ds->path_trace.len = htons(GPTP_CLOCK_ID_LEN);
	memcpy(global_ds->path_trace.path_sequence, GPTP_DEFAULT_DS()->clk_id,
	       GPTP_CLOCK_ID_LEN);
}

static void gptp_clear_reselect_tree(void)
{
	/* Set all the elements of the reselect array to FALSE. */
	GPTP_GLOBAL_DS()->reselect_array = 0;
}

static int compute_best_vector(void)
{
	struct gptp_priority_vector *gm_prio;
	struct gptp_default_ds *default_ds;
	struct gptp_global_ds *global_ds;
	struct gptp_priority_vector *best_vector, *challenger;
	int best_port, port, tmp;
	struct gptp_pss_rcv_state *pss_rcv;
	struct gptp_port_announce_information_state *pa_info_state;

	default_ds = GPTP_DEFAULT_DS();
	global_ds = GPTP_GLOBAL_DS();
	best_port = 0;
	gm_prio = &global_ds->gm_priority;

	/* Write systemPriority into grandmaster. */
	(void)memset(gm_prio, 0, sizeof(struct gptp_priority_vector));
	gm_prio->root_system_id.grand_master_prio1 = default_ds->priority1;
	gm_prio->root_system_id.grand_master_prio2 = default_ds->priority2;
	gm_prio->root_system_id.clk_quality.clock_class =
		default_ds->clk_quality.clock_class;
	gm_prio->root_system_id.clk_quality.clock_accuracy =
		default_ds->clk_quality.clock_accuracy;
	gm_prio->root_system_id.clk_quality.offset_scaled_log_var =
		htons(default_ds->clk_quality.offset_scaled_log_var);

	memcpy(gm_prio->src_port_id.clk_id, default_ds->clk_id,
	       GPTP_CLOCK_ID_LEN);
	memcpy(gm_prio->root_system_id.grand_master_id, default_ds->clk_id,
	       GPTP_CLOCK_ID_LEN);

	best_vector = gm_prio;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		challenger = &GPTP_PORT_BMCA_DATA(port)->port_priority;
		pa_info_state = &GPTP_PORT_STATE(port)->pa_info;
		pss_rcv = &GPTP_PORT_STATE(port)->pss_rcv;

		if (pa_info_state->ann_expired ||
		    (global_ds->gm_present &&
		     pss_rcv->rcv_sync_receipt_timeout_timer_expired)) {
			continue;
		}

		if (memcmp(challenger->src_port_id.clk_id, default_ds->clk_id,
			   GPTP_CLOCK_ID_LEN) == 0) {
			/* Discard this challenger. */
			continue;
		}

		if (best_port == 0) {
			tmp = memcmp(&challenger->root_system_id,
				     &best_vector->root_system_id,
				     sizeof(struct gptp_root_system_identity));
			if (tmp < 0) {
				best_vector = challenger;
				best_port = port;
			} else if (tmp > 0) {
				continue;
			}

			tmp = (int)challenger->steps_removed -
				((int)ntohs(best_vector->steps_removed) + 1);
			if (tmp < 0) {
				best_vector = challenger;
				best_port = port;
			} else if (tmp > 0) {
				continue;
			}

			tmp = memcmp(&challenger->src_port_id,
				     &best_vector->src_port_id,
				     sizeof(struct gptp_port_identity));
			if (tmp < 0) {
				best_vector = challenger;
				best_port = port;
			} else if (tmp > 0) {
				continue;
			}

			if (ntohs(challenger->port_number) <
			    ntohs(best_vector->port_number)) {
				best_vector = challenger;
				best_port = port;
			}

		} else {
			/* We can compare portPriority vectors without
			 * calculating pathPriority vectors.
			 */
			if (memcmp(challenger, best_vector,
				   sizeof(struct gptp_priority_vector)) < 0) {
				best_vector = challenger;
				best_port = port;
			}
		}
	}

	if (best_port != 0) {
		memcpy(&global_ds->gm_priority.root_system_id,
		       &best_vector->root_system_id,
		       sizeof(struct gptp_root_system_identity));

		global_ds->gm_priority.steps_removed =
			htons(ntohs(best_vector->steps_removed) + 1);

		memcpy(&global_ds->gm_priority.src_port_id,
		       &best_vector->src_port_id,
		       sizeof(struct gptp_port_identity));

		global_ds->gm_priority.port_number = best_vector->port_number;
	}

	return best_port;
}

static void update_bmca(int port,
			int best_port,
			struct gptp_global_ds *global_ds,
			struct gptp_default_ds *default_ds,
			struct gptp_priority_vector *gm_prio)
{
	struct gptp_port_bmca_data *bmca_data = GPTP_PORT_BMCA_DATA(port);

	/* Update masterPriorityVector for the port. */
	if (best_port == 0) {
		memcpy(&bmca_data->master_priority, gm_prio,
		       sizeof(struct gptp_priority_vector));

		bmca_data->master_priority.port_number = htons(port);
		bmca_data->master_priority.src_port_id.port_number =
			htons(port);
	} else {
		memcpy(&bmca_data->master_priority.root_system_id,
		       &gm_prio->root_system_id,
		       sizeof(struct gptp_root_system_identity));
		memcpy(bmca_data->master_priority.src_port_id.clk_id,
		       default_ds->clk_id, GPTP_CLOCK_ID_LEN);
		bmca_data->master_priority.port_number = htons(port);
		bmca_data->master_priority.src_port_id.port_number =
			htons(port);
	}

	switch (bmca_data->info_is) {
	case GPTP_INFO_IS_DISABLED:
		gptp_change_port_state(port, GPTP_PORT_DISABLED);
		break;

	case GPTP_INFO_IS_AGED:
		bmca_data->updt_info = true;
		gptp_change_port_state(port, GPTP_PORT_MASTER);
		break;

	case GPTP_INFO_IS_MINE:
		gptp_change_port_state(port, GPTP_PORT_MASTER);

		if ((memcmp(&bmca_data->port_priority,
			    &bmca_data->master_priority,
			    sizeof(struct gptp_priority_vector)) != 0) ||
		    (bmca_data->port_steps_removed !=
		     global_ds->master_steps_removed)) {
			bmca_data->updt_info = true;
		}

		break;

	case GPTP_INFO_IS_RECEIVED:
		if (best_port == port) {
			/* gmPriorityVector is now derived from
			 * portPriorityVector.
			 */
			gptp_change_port_state(port, GPTP_PORT_SLAVE);
			bmca_data->updt_info = false;
		} else if (memcmp(&bmca_data->port_priority,
				  &bmca_data->master_priority,
				  sizeof(struct gptp_priority_vector)) <= 0) {
			/* The masterPriorityVector is not better than
			 * the portPriorityVector.
			 */
			gptp_change_port_state(port, GPTP_PORT_PASSIVE);

			if (memcmp(bmca_data->port_priority.src_port_id.clk_id,
				   default_ds->clk_id,
				   GPTP_CLOCK_ID_LEN)) {
				/* The sourcePortIdentity component of
				 * the portPriorityVector does not
				 * reflect another port on the
				 * time-aware system.
				 */
				bmca_data->updt_info = true;
			} else {
				bmca_data->updt_info = false;
			}
		} else {
			gptp_change_port_state(port, GPTP_PORT_MASTER);
			bmca_data->updt_info = true;
		}

		break;
	}
}

static void gptp_updt_roles_tree(void)
{
	struct gptp_global_ds *global_ds;
	struct gptp_default_ds *default_ds;
	struct gptp_priority_vector *gm_prio, *last_gm_prio;
	struct gptp_port_bmca_data *bmca_data;
	int port, best_port;

	global_ds = GPTP_GLOBAL_DS();
	default_ds = GPTP_DEFAULT_DS();

	gm_prio = &global_ds->gm_priority;
	last_gm_prio = &global_ds->last_gm_priority;

	/* Save gmPriority. */
	memcpy(last_gm_prio, gm_prio, sizeof(struct gptp_priority_vector));

	best_port = compute_best_vector();

	/* If the best vector was the systemPriorityVector. */
	if (best_port == 0) {
		/* Copy leap61, leap59, current UTC offset valid,
		 * time traceable and frequency traceable flags.
		 */
		global_ds->global_flags.octets[1] =
			global_ds->sys_flags.octets[1];
		global_ds->current_utc_offset =
			global_ds->sys_current_utc_offset;
		global_ds->time_source = global_ds->sys_time_source;
		global_ds->master_steps_removed = 0;
	} else {
		bmca_data = GPTP_PORT_BMCA_DATA(best_port);

		/* Copy leap61, leap59, current UTC offset valid,
		 * time traceable and frequency traceable flags.
		 */
		global_ds->global_flags.octets[1] =
			bmca_data->ann_flags.octets[1];
		global_ds->current_utc_offset =
			global_ds->sys_current_utc_offset;
		global_ds->time_source = bmca_data->ann_time_source;
		global_ds->master_steps_removed =
			htons(ntohs(bmca_data->message_steps_removed) + 1);
	}

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		update_bmca(port, best_port, global_ds, default_ds, gm_prio);
	}

	/* Update gmPresent. */
	global_ds->gm_present =
		(gm_prio->root_system_id.grand_master_prio1 == 255) ?
		false : true;

	/* Assign the port role for port 0. */
	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		if (global_ds->selected_role[port] == GPTP_PORT_SLAVE) {
			gptp_change_port_state(0, GPTP_PORT_PASSIVE);
			break;
		}
	}

	if (port == GPTP_PORT_END) {
		gptp_change_port_state(0, GPTP_PORT_SLAVE);
	}

	/* If current system is the Grand Master, set pathTrace array. */
	if (memcmp(default_ds->clk_id, gm_prio->root_system_id.grand_master_id,
		   GPTP_CLOCK_ID_LEN) == 0) {
		global_ds->path_trace.len = htons(GPTP_CLOCK_ID_LEN);
		memcpy(global_ds->path_trace.path_sequence,
		       default_ds->clk_id, GPTP_CLOCK_ID_LEN);
	}
}

static void gptp_set_selected_tree(void)
{
	/* Set all the elements of the selected array to TRUE. */
	GPTP_GLOBAL_DS()->selected_array = ~0;
}

static void gptp_mi_port_role_selection_state_machine(void)
{
	struct gptp_port_role_selection_state *state;

	state = &GPTP_STATE()->pr_sel;

	switch (state->state) {
	case GPTP_PR_SELECTION_INIT_BRIDGE:
		gptp_updt_role_disabled_tree();
		state->state = GPTP_PR_SELECTION_ROLE_SELECTION;

		/* Be sure to enter the "if" statement immediately after. */
		GPTP_GLOBAL_DS()->reselect_array = ~0;
		/* Fallthrough. */

	case GPTP_PR_SELECTION_ROLE_SELECTION:
		if (GPTP_GLOBAL_DS()->reselect_array != 0) {
			gptp_clear_reselect_tree();
			gptp_updt_roles_tree();
			gptp_set_selected_tree();
		}

		break;
	}
}

static void tx_announce(int port)
{
	struct net_pkt *pkt;

	pkt = gptp_prepare_announce(port);
	if (pkt) {
		gptp_send_announce(port, pkt);
	}
}

static void gptp_mi_port_announce_transmit_state_machine(int port)
{
	struct gptp_port_ds *port_ds;
	struct gptp_global_ds *global_ds;
	struct gptp_port_announce_transmit_state *state;
	struct gptp_port_bmca_data *bmca_data;

	port_ds = GPTP_PORT_DS(port);
	global_ds = GPTP_GLOBAL_DS();
	bmca_data = GPTP_PORT_BMCA_DATA(port);
	state = &GPTP_PORT_STATE(port)->pa_transmit;

	/* Reset interval as defined in AnnounceIntervalSetting
	 * state machine.
	 */
	if (port_ds->ptt_port_enabled && !port_ds->prev_ptt_port_enabled) {
		gptp_update_announce_interval(port, GPTP_ITV_SET_TO_INIT);
	}

	switch (state->state) {
	case GPTP_PA_TRANSMIT_INIT:
		bmca_data->new_info = true;
		/* Fallthrough. */

	case GPTP_PA_TRANSMIT_IDLE:
		k_timer_stop(&state->ann_send_periodic_timer);
		state->ann_trigger = false;
		k_timer_start(&state->ann_send_periodic_timer,
			      gptp_uscaled_ns_to_timer_ms(
				      &bmca_data->announce_interval),
			      0);

		state->state = GPTP_PA_TRANSMIT_POST_IDLE;
		/* Fallthrough. */

	case GPTP_PA_TRANSMIT_POST_IDLE:
		if (IS_SELECTED(global_ds, port) &&
		    !bmca_data->updt_info &&
		    state->ann_trigger) {

			state->state = GPTP_PA_TRANSMIT_PERIODIC;

		} else if (IS_SELECTED(global_ds, port) &&
			   !bmca_data->updt_info &&
			   !state->ann_trigger &&
			   (global_ds->selected_role[port] ==
			    GPTP_PORT_MASTER) &&
			   bmca_data->new_info) {

			bmca_data->new_info = false;
			tx_announce(port);
			state->state = GPTP_PA_TRANSMIT_IDLE;
		}

		break;

	case GPTP_PA_TRANSMIT_PERIODIC:
		if (global_ds->selected_role[port] == GPTP_PORT_MASTER) {
			bmca_data->new_info = true;
		}
		state->state = GPTP_PA_TRANSMIT_IDLE;
		break;
	}
}


void gptp_mi_port_sync_state_machines(int port)
{
	gptp_mi_pss_rcv_state_machine(port);
	gptp_mi_pss_send_state_machine(port);
}

void gptp_mi_port_bmca_state_machines(int port)
{
	gptp_mi_port_announce_receive_state_machine(port);
	gptp_mi_port_announce_information_state_machine(port);
	gptp_mi_port_announce_transmit_state_machine(port);
}

void gptp_mi_state_machines(void)
{
	gptp_mi_site_sync_sync_state_machine();
	gptp_mi_clk_slave_sync_state_machine();
	gptp_mi_port_role_selection_state_machine();
	gptp_mi_clk_master_sync_rcv_state_machine();
}

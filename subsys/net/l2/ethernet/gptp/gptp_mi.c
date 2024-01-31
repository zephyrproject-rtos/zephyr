/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_gptp, CONFIG_NET_GPTP_LOG_LEVEL);

#include <zephyr/drivers/ptp_clock.h>

#include "gptp_messages.h"
#include "gptp_data_set.h"
#include "gptp_state.h"
#include "gptp_private.h"

#if CONFIG_NET_GPTP_LOG_LEVEL >= LOG_LEVEL_DBG
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

static const char * const pa_info_state2str(enum gptp_pa_info_states state)
{
	switch (state) {
	case GPTP_PA_INFO_DISABLED:
		return "DISABLED";
	case GPTP_PA_INFO_POST_DISABLED:
		return "POST_DISABLED";
	case GPTP_PA_INFO_AGED:
		return "AGED";
	case GPTP_PA_INFO_UPDATE:
		return "UPDATE";
	case GPTP_PA_INFO_CURRENT:
		return "CURRENT";
	case GPTP_PA_INFO_RECEIVE:
		return "RECEIVE";
	case GPTP_PA_INFO_SUPERIOR_MASTER_PORT:
		return "SUPERIOR_MASTER_PORT";
	case GPTP_PA_INFO_REPEATED_MASTER_PORT:
		return "REPEATED_MASTER_PORT";
	case GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT:
		return "INFERIOR_MASTER_OR_OTHER_PORT";
	}

	return "<unknown>";
}
#endif

#if CONFIG_NET_GPTP_LOG_LEVEL >= LOG_LEVEL_DBG
void gptp_change_port_state_debug(int port, enum gptp_port_state state,
				  const char *caller,
				  int line)
#else
void gptp_change_port_state(int port, enum gptp_port_state state)
#endif
{
	struct gptp_global_ds *global_ds = GPTP_GLOBAL_DS();

	if (global_ds->selected_role[port] == state) {
		return;
	}

#if CONFIG_NET_GPTP_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("[%d] state %s -> %s (%s():%d)", port,
		state2str(global_ds->selected_role[port]),
		state2str(state), caller, line);
#endif

	global_ds->selected_role[port] = state;
};

#if CONFIG_NET_GPTP_LOG_LEVEL >= LOG_LEVEL_DBG
void gptp_change_pa_info_state_debug(
	int port,
	struct gptp_port_announce_information_state *pa_info_state,
	enum gptp_pa_info_states state,
	const char *caller,
	int line)
#else
void gptp_change_pa_info_state(
	int port,
	struct gptp_port_announce_information_state *pa_info_state,
	enum gptp_pa_info_states state)
#endif
{
	if (pa_info_state->state == state) {
		return;
	}

#if CONFIG_NET_GPTP_LOG_LEVEL >= LOG_LEVEL_DBG
	NET_DBG("[%d] PA info state %s -> %s (%s():%d)", port,
		pa_info_state2str(pa_info_state->state),
		pa_info_state2str(state), caller, line);
#endif

	pa_info_state->state = state;
}

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
	struct gptp_clk_master_sync_rcv_state *cms_rcv;

	cms_rcv = &GPTP_STATE()->clk_master_sync_receive;
	(void)memset(cms_rcv, 0, sizeof(struct gptp_clk_master_sync_rcv_state));
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
	gptp_change_pa_info_state(port, state, GPTP_PA_INFO_DISABLED);
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

uint64_t gptp_get_current_time_nanosecond(int port)
{
	const struct device *clk;

	clk = net_eth_get_ptp_clock(GPTP_PORT_IFACE(port));
	if (clk) {
		struct net_ptp_time tm = {};

		ptp_clock_get(clk, &tm);

		if (tm.second == 0U && tm.nanosecond == 0U) {
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

uint64_t gptp_get_current_master_time_nanosecond(void)
{
	int port;
	enum gptp_port_state *port_role;

	port_role = GPTP_GLOBAL_DS()->selected_role;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		if (port_role[port] == GPTP_PORT_MASTER) {
			return gptp_get_current_time_nanosecond(port);
		}
	}

	/* No master */
	return 0;
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
	k_timeout_t duration;

	duration = K_MSEC(port_ds->sync_receipt_timeout_time_itv /
			  (NSEC_PER_USEC * USEC_PER_MSEC));

	k_timer_start(&state->rcv_sync_receipt_timeout_timer, duration,
		      K_NO_WAIT);
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

		__fallthrough;
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

	state = &GPTP_PORT_STATE(port)->pss_send;
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
	struct gptp_sync_send_state *sync_send;

	state = &GPTP_PORT_STATE(port)->pss_send;
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
	k_timeout_t duration;

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

		__fallthrough;
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
		duration = K_MSEC(gptp_uscaled_ns_to_timer_ms(
					  &port_ds->half_sync_itv));

		/* Start 0.5 * syncInterval timeout timer. */
		k_timer_start(&state->half_sync_itv_timer, duration,
			      K_NO_WAIT);

		/* sourcePortIdentity is set to the portIdentity of this
		 * PTP Port (see ch. 10.2.12.2.1 and ch 8.5.2).
		 */
		memcpy(&state->pss_sync_ptr->sync_info.src_port_id.clk_id,
			GPTP_DEFAULT_DS()->clk_id,
			GPTP_CLOCK_ID_LEN);
		state->pss_sync_ptr->sync_info.src_port_id.port_number = port;


		gptp_mi_pss_send_md_sync_send(port);

		__fallthrough;
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

			duration =
				K_MSEC(port_ds->sync_receipt_timeout_time_itv /
				       (NSEC_PER_USEC * USEC_PER_MSEC));

			k_timer_start(&state->send_sync_receipt_timeout_timer,
				      duration, K_NO_WAIT);

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
	uint16_t local_port_number;
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
	struct gptp_clk_master_sync_offset_state *offset_state;
	struct gptp_global_ds *global_ds;
	struct gptp_md_sync_info *pss;
	struct gptp_port_ds *port_ds;
	uint64_t sync_receipt_time;

	state = &GPTP_STATE()->clk_slave_sync;
	offset_state = &GPTP_STATE()->clk_master_sync_offset;
	global_ds = GPTP_GLOBAL_DS();
	port_ds = GPTP_PORT_DS(state->pss_rcv_ptr->local_port_number);

	pss = &state->pss_rcv_ptr->sync_info;

	sync_receipt_time = port_ds->neighbor_prop_delay;
	sync_receipt_time *= pss->rate_ratio;
	sync_receipt_time /= port_ds->neighbor_rate_ratio;
	sync_receipt_time += pss->follow_up_correction_field;
	sync_receipt_time += port_ds->delay_asymmetry;

	global_ds->sync_receipt_time.second = sync_receipt_time / NSEC_PER_SEC;
	global_ds->sync_receipt_time.fract_nsecond =
		(sync_receipt_time % NSEC_PER_SEC) * GPTP_POW2_16;
	global_ds->sync_receipt_time.second += pss->precise_orig_ts.second;
	global_ds->sync_receipt_time.fract_nsecond +=
		pss->precise_orig_ts.nanosecond * GPTP_POW2_16;

	global_ds->sync_receipt_local_time = port_ds->delay_asymmetry;
	global_ds->sync_receipt_local_time /= pss->rate_ratio;
	global_ds->sync_receipt_local_time +=
		(port_ds->neighbor_prop_delay / port_ds->neighbor_rate_ratio);
	global_ds->sync_receipt_local_time += pss->upstream_tx_time;

	global_ds->gm_time_base_indicator = pss->gm_time_base_indicator;
	global_ds->last_gm_phase_change.high = pss->last_gm_phase_change.high;
	global_ds->last_gm_phase_change.low = pss->last_gm_phase_change.low;
	global_ds->last_gm_freq_change = pss->last_gm_freq_change;

	offset_state->rcvd_sync_receipt_time = true;
}

#if defined(CONFIG_NET_GPTP_USE_DEFAULT_CLOCK_UPDATE)
static void gptp_update_local_port_clock(void)
{
	struct gptp_clk_slave_sync_state *state;
	struct gptp_global_ds *global_ds;
	struct gptp_port_ds *port_ds;
	int port;
	int64_t nanosecond_diff;
	int64_t second_diff;
	const struct device *clk;
	struct net_ptp_time tm;
	unsigned int key;

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
	nanosecond_diff =
		(global_ds->sync_receipt_time.fract_nsecond / GPTP_POW2_16) -
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
		nanosecond_diff = -(int64_t)NSEC_PER_SEC + nanosecond_diff;
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

		if (second_diff < 0 && tm.second < -second_diff) {
			NET_DBG("Do not set local clock because %lu < %ld",
				(unsigned long int)tm.second,
				(long int)-second_diff);
			goto skip_clock_set;
		}

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

		/* This prints too much data normally but can be enabled to see
		 * what time we are setting to the local clock.
		 */
		if (0) {
			NET_INFO("Set local clock %lu.%lu",
				 (unsigned long int)tm.second,
				 (unsigned long int)tm.nanosecond);
		}

		ptp_clock_set(clk, &tm);

	skip_clock_set:
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

static void gptp_mi_clk_master_sync_offset_state_machine(void)
{
	struct gptp_clk_master_sync_offset_state *state;
	struct gptp_global_ds *global_ds;

	state = &GPTP_STATE()->clk_master_sync_offset;
	global_ds = GPTP_GLOBAL_DS();

	switch (state->state) {
	case GPTP_CMS_OFFSET_INITIALIZING:
		state->rcvd_sync_receipt_time = false;
		state->state = GPTP_CMS_OFFSET_INDICATION;
		break;
	case GPTP_CMS_OFFSET_INDICATION:
		if (!state->rcvd_sync_receipt_time) {
			break;
		}

		state->rcvd_sync_receipt_time = false;

		if (global_ds->selected_role[0] == GPTP_PORT_PASSIVE) {
			/* TODO Calculate real values for proper BC support */
			memset(&global_ds->clk_src_phase_offset, 0x0,
			       sizeof(struct gptp_scaled_ns));
			global_ds->clk_src_freq_offset = 0;
		} else if (global_ds->clk_src_time_base_indicator_prev
			   != global_ds->clk_src_time_base_indicator) {
			memcpy(&global_ds->clk_src_phase_offset,
			       &global_ds->last_gm_phase_change,
			       sizeof(struct gptp_scaled_ns));

			global_ds->clk_src_freq_offset =
				global_ds->last_gm_freq_change;
		}

		break;
	default:
		NET_ERR("Unrecognised state %d", state->state);
		break;
	}
}

#if defined(CONFIG_NET_GPTP_GM_CAPABLE)
static inline void gptp_mi_setup_sync_send_time(void)
{
	struct gptp_clk_master_sync_snd_state *state;
	struct gptp_global_ds *global_ds;
	uint64_t time_helper;

	state = &GPTP_STATE()->clk_master_sync_send;
	global_ds = GPTP_GLOBAL_DS();

	time_helper = state->sync_send_time.low;

	state->sync_send_time.low +=
		global_ds->clk_master_sync_itv;

	/* Check for overflow */
	if (state->sync_send_time.low < time_helper) {
		state->sync_send_time.high += 1U;
		state->sync_send_time.low =
			UINT64_MAX - state->sync_send_time.low;
	}
}

static void gptp_mi_set_ps_sync_cmss(void)
{
	struct gptp_clk_master_sync_snd_state *state;
	struct gptp_global_ds *global_ds;
	struct gptp_md_sync_info *sync_info;
	uint64_t current_time;

	global_ds = GPTP_GLOBAL_DS();
	state = &GPTP_STATE()->clk_master_sync_send;

	sync_info = &state->pss_snd.sync_info;

	state->pss_snd.local_port_number = 0U;

	current_time = gptp_get_current_master_time_nanosecond();

	sync_info->precise_orig_ts.second = current_time / NSEC_PER_SEC;
	sync_info->precise_orig_ts.nanosecond = current_time % NSEC_PER_SEC;

	/* TODO calculate correction field properly, rate_ratio is also set to
	 * zero instead of being copied from global_ds as it affects the final
	 * value of FUP correction field.
	 */
	sync_info->follow_up_correction_field = 0;
	sync_info->rate_ratio = 0;

	memcpy(&sync_info->src_port_id.clk_id,
	       GPTP_DEFAULT_DS()->clk_id,
	       GPTP_CLOCK_ID_LEN);

	sync_info->src_port_id.port_number = 0U;
	sync_info->log_msg_interval = CONFIG_NET_GPTP_INIT_LOG_SYNC_ITV;
	sync_info->upstream_tx_time = global_ds->local_time.low;

	state->pss_snd.sync_receipt_timeout_time = UINT64_MAX;

	sync_info->gm_time_base_indicator =
		global_ds->clk_src_time_base_indicator;

	memcpy(&sync_info->last_gm_phase_change,
	       &global_ds->clk_src_phase_offset,
	       sizeof(struct gptp_scaled_ns));

	sync_info->last_gm_freq_change = global_ds->clk_src_freq_offset;
}

static inline void gptp_mi_tx_ps_sync_cmss(void)
{
	struct gptp_clk_master_sync_snd_state *state;
	struct gptp_pss_send_state *pss_send;
	int port;

	state = &GPTP_STATE()->clk_master_sync_send;

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		pss_send = &GPTP_PORT_STATE(port)->pss_send;
		pss_send->pss_sync_ptr = &state->pss_snd;

		pss_send->rcvd_pss_sync = true;
	}
}

static void gptp_mi_clk_master_sync_snd_state_machine(void)
{
	struct gptp_clk_master_sync_snd_state *state;
	uint64_t current_time;

	state = &GPTP_STATE()->clk_master_sync_send;

	switch (state->state) {
	case GPTP_CMS_SND_INITIALIZING:
		gptp_mi_setup_sync_send_time();

		state->state = GPTP_CMS_SND_INDICATION;
		break;

	case GPTP_CMS_SND_INDICATION:
		current_time = gptp_get_current_master_time_nanosecond();

		if (current_time >= state->sync_send_time.low) {
			gptp_mi_set_ps_sync_cmss();
			gptp_mi_tx_ps_sync_cmss();

			gptp_mi_setup_sync_send_time();
		}

		break;

	default:
		NET_ERR("Unrecognised state %d", state->state);
		break;
	}
}
#endif

static void gptp_compute_gm_rate_ratio(void)
{
	static struct net_ptp_extended_time src_time_0;
	static struct gptp_uscaled_ns local_time_0;
	struct net_ptp_extended_time src_time_n;
	struct gptp_uscaled_ns local_time_n;
	struct net_ptp_extended_time src_time_t;
	struct gptp_uscaled_ns local_time_t;
	struct gptp_clk_master_sync_rcv_state *state;
	struct gptp_global_ds *global_ds;
	double new_gm_rate;

	state = &GPTP_STATE()->clk_master_sync_receive;
	global_ds = GPTP_GLOBAL_DS();

	/* Get current local and source time */
	memcpy(&src_time_n, &state->rcvd_clk_src_req.src_time,
	       sizeof(struct net_ptp_extended_time));

	memcpy(&local_time_n, &global_ds->local_time,
	       sizeof(struct gptp_uscaled_ns));

	if ((src_time_0.second == 0U && src_time_0.fract_nsecond == 0U)
	    || (local_time_0.high == 0U && local_time_0.low == 0U)) {
		memcpy(&src_time_0, &src_time_n,
		       sizeof(struct net_ptp_extended_time));

		memcpy(&local_time_0, &local_time_n,
		       sizeof(struct gptp_uscaled_ns));

		global_ds->gm_rate_ratio = 1.0;

		return;
	}

	/* Take care of the sign of the result */
	new_gm_rate = 1.0;

	if ((src_time_n.second < src_time_0.second)
	    || (src_time_n.second == src_time_0.second
		&& src_time_n.fract_nsecond < src_time_0.fract_nsecond)) {
		/* Change result sign and swap src_time_n and src_time_0 */
		memcpy(&src_time_t, &src_time_n,
		       sizeof(struct net_ptp_extended_time));
		memcpy(&src_time_n, &src_time_0,
		       sizeof(struct net_ptp_extended_time));
		memcpy(&src_time_0, &src_time_t,
		       sizeof(struct net_ptp_extended_time));

		new_gm_rate *= -1;
	}

	if ((local_time_n.high < local_time_0.high)
	    || (local_time_n.high == local_time_0.high
		&& local_time_n.low < local_time_0.low)) {
		/* Change result sign and swap local_time_n and local_time_0 */
		memcpy(&local_time_t, &local_time_n,
		       sizeof(struct gptp_uscaled_ns));
		memcpy(&local_time_n, &local_time_0,
		       sizeof(struct gptp_uscaled_ns));
		memcpy(&local_time_0, &local_time_t,
		       sizeof(struct gptp_uscaled_ns));

		new_gm_rate *= -1;
	}

	/* At this point src_time_n >= src_time_0 */
	src_time_n.second -= src_time_0.second;

	if (src_time_n.fract_nsecond >= src_time_0.fract_nsecond) {
		src_time_n.fract_nsecond -= src_time_0.fract_nsecond;
	} else {
		src_time_n.second -= 1U;
		src_time_n.fract_nsecond = (NSEC_PER_SEC * GPTP_POW2_16)
			- src_time_0.fract_nsecond;
	}

	/* At this point local_time_n >= local_time_0 */
	local_time_n.high -= local_time_0.high;

	if (local_time_n.low >= local_time_0.low) {
		local_time_n.low -= local_time_0.low;
	} else {
		local_time_n.high -= 1U;
		local_time_n.low = UINT64_MAX - local_time_0.low;
	}

	/* Calculate it in nanoseconds, new_gm_rate is either 1 or -1 here */
	new_gm_rate *= ((src_time_n.second * NSEC_PER_SEC)
		+ (src_time_n.fract_nsecond / GPTP_POW2_16));

	new_gm_rate /= local_time_n.low;

	global_ds->gm_rate_ratio = new_gm_rate;
}

static void gptp_mi_clk_master_sync_rcv_state_machine(void)
{
	struct gptp_clk_master_sync_rcv_state *s;
	struct gptp_global_ds *global_ds;

#ifdef CONFIG_NET_GPTP_PROBE_CLOCK_SOURCE_ON_DEMAND
	struct gptp_clk_src_time_invoke_params invoke_args = {};
	uint64_t cur = gptp_get_current_master_time_nanosecond();

	invoke_args.src_time.second = cur / NSEC_PER_SEC;
	cur -= (invoke_args.src_time.second * NSEC_PER_SEC);

	invoke_args.src_time.fract_nsecond = cur * GPTP_POW2_16;

	memset(&invoke_args.last_gm_phase_change, 0x0,
	       sizeof(struct gptp_scaled_ns));
	invoke_args.last_gm_freq_change = 0;

	gptp_clk_src_time_invoke(&invoke_args);
#endif

	global_ds = GPTP_GLOBAL_DS();

	s = &GPTP_STATE()->clk_master_sync_receive;
	switch (s->state) {
	case GPTP_CMS_RCV_INITIALIZING:
		s->state = GPTP_CMS_RCV_WAITING;
		break;

	case GPTP_CMS_RCV_WAITING:
		if (s->rcvd_clock_source_req || s->rcvd_local_clock_tick) {
			s->state = GPTP_CMS_RCV_SOURCE_TIME;
		}

		break;

	case GPTP_CMS_RCV_SOURCE_TIME:
		global_ds->local_time.high = 0U;
		global_ds->local_time.low =
			gptp_get_current_master_time_nanosecond();

		if (s->rcvd_clock_source_req) {
			gptp_compute_gm_rate_ratio();

			global_ds->clk_src_time_base_indicator_prev =
				global_ds->clk_src_time_base_indicator;

			global_ds->clk_src_time_base_indicator =
				s->rcvd_clk_src_req.time_base_indicator;

			memcpy(&global_ds->clk_src_last_gm_phase_change,
			       &s->rcvd_clk_src_req.last_gm_phase_change,
			       sizeof(struct gptp_scaled_ns));

			global_ds->clk_src_last_gm_freq_change =
				s->rcvd_clk_src_req.last_gm_freq_change;
		}

		s->rcvd_clock_source_req = false;
		s->rcvd_local_clock_tick = false;
		s->state = GPTP_CMS_RCV_WAITING;
		break;

	default:
		NET_ERR("Unrecognised state %d", s->state);
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
	memcpy((uint8_t *)sys_path_trace->path_sequence + len,
	       GPTP_DEFAULT_DS()->clk_id, GPTP_CLOCK_ID_LEN);
}

static bool gptp_mi_qualify_announce(int port, struct net_pkt *announce_msg)
{
	struct gptp_announce *announce;
	struct gptp_hdr *hdr;
	int i;
	uint16_t len;

	hdr = GPTP_HDR(announce_msg);
	announce = GPTP_ANNOUNCE(announce_msg);

	if (memcmp(hdr->port_id.clk_id, GPTP_DEFAULT_DS()->clk_id,
		   GPTP_CLOCK_ID_LEN) == 0) {
		return false;
	}

	len = ntohs(announce->steps_removed);
	if (len >= 255U) {
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
	int rsi_cmp, spi_cmp, port_cmp;

	hdr = GPTP_HDR(pkt);
	announce = GPTP_ANNOUNCE(pkt);

	/* Compare rootSystemIdentity and stepsRemoved. */
	rsi_cmp = memcmp(&announce->root_system_id,
			 &vector->root_system_id,
			 sizeof(struct gptp_root_system_identity) +
			 sizeof(uint16_t));
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
	       sizeof(struct gptp_root_system_identity) + sizeof(uint16_t));

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
		gptp_change_pa_info_state(port, state, GPTP_PA_INFO_DISABLED);
	}

	switch (state->state) {
	case GPTP_PA_INFO_DISABLED:
		bmca_data->rcvd_msg = false;
		bmca_data->info_is = GPTP_INFO_IS_DISABLED;
		SET_RESELECT(global_ds, port);
		CLEAR_SELECTED(global_ds, port);
		gptp_change_pa_info_state(port, state,
					  GPTP_PA_INFO_POST_DISABLED);
		k_timer_stop(&state->ann_rcpt_expiry_timer);
		state->ann_expired = true;
		__fallthrough;

	case GPTP_PA_INFO_POST_DISABLED:
		if (port_ds->ptt_port_enabled && port_ds->as_capable) {
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_AGED);
		} else if (bmca_data->rcvd_msg) {
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_DISABLED);
		}

		break;

	case GPTP_PA_INFO_AGED:
		bmca_data->info_is = GPTP_INFO_IS_AGED;
		CLEAR_SELECTED(global_ds, port);
		SET_RESELECT(global_ds, port);
		/* Transition will be actually tested in UPDATE state. */
		gptp_change_pa_info_state(port, state, GPTP_PA_INFO_UPDATE);
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
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_CURRENT);
		}

		break;

	case GPTP_PA_INFO_CURRENT:
		pss_rcv = &GPTP_PORT_STATE(port)->pss_rcv;
		if (IS_SELECTED(global_ds, port) && bmca_data->updt_info) {
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_UPDATE);
		} else if (bmca_data->rcvd_msg && !bmca_data->updt_info) {
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_RECEIVE);
		} else if ((bmca_data->info_is == GPTP_INFO_IS_RECEIVED) &&
			   !bmca_data->updt_info &&
			   !bmca_data->rcvd_msg &&
			   (state->ann_expired ||
			    (global_ds->gm_present &&
			   pss_rcv->rcv_sync_receipt_timeout_timer_expired))) {
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_AGED);
		}

		break;

	case GPTP_PA_INFO_RECEIVE:
		switch (rcv_info(port)) {
		case GPTP_RCVD_INFO_SUPERIOR_MASTER_INFO:
			gptp_change_pa_info_state(port, state,
				GPTP_PA_INFO_SUPERIOR_MASTER_PORT);
			break;
		case GPTP_RCVD_INFO_REPEATED_MASTER_INFO:
			gptp_change_pa_info_state(port, state,
				GPTP_PA_INFO_REPEATED_MASTER_PORT);
			break;
		case GPTP_RCVD_INFO_INFERIOR_MASTER_INFO:
			__fallthrough;
		case GPTP_RCVD_INFO_OTHER_INFO:
			gptp_change_pa_info_state(port, state,
				GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT);
			break;
		}

		break;

	case GPTP_PA_INFO_SUPERIOR_MASTER_PORT:
		/* We copy directly the content of the message to the port
		 * priority vector without using an intermediate
		 * messagePriority structure.
		 */

		if (!bmca_data->rcvd_announce_ptr) {
			/* Shouldn't be reached. Checked for safety reason. */
			bmca_data->rcvd_msg = false;
			gptp_change_pa_info_state(port, state,
						  GPTP_PA_INFO_CURRENT);
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
		__fallthrough;

	case GPTP_PA_INFO_REPEATED_MASTER_PORT:
		k_timer_stop(&state->ann_rcpt_expiry_timer);
		state->ann_expired = false;
		k_timer_start(&state->ann_rcpt_expiry_timer,
			      K_MSEC(gptp_uscaled_ns_to_timer_ms(
				  &bmca_data->ann_rcpt_timeout_time_interval)),
			      K_NO_WAIT);
		__fallthrough;

	case GPTP_PA_INFO_INFERIOR_MASTER_OR_OTHER_PORT:
		if (bmca_data->rcvd_announce_ptr != NULL) {
			net_pkt_unref(bmca_data->rcvd_announce_ptr);
			bmca_data->rcvd_announce_ptr = NULL;
		}

		bmca_data->rcvd_msg = false;
		gptp_change_pa_info_state(port, state, GPTP_PA_INFO_CURRENT);
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
		if (&global_ds->gm_priority.root_system_id !=
		    &best_vector->root_system_id) {
			memcpy(&global_ds->gm_priority.root_system_id,
			       &best_vector->root_system_id,
			       sizeof(struct gptp_root_system_identity));
		}

		global_ds->gm_priority.steps_removed =
			ntohs(best_vector->steps_removed) + 1;

		if (&global_ds->gm_priority.src_port_id !=
		    &best_vector->src_port_id) {
			memcpy(&global_ds->gm_priority.src_port_id,
			       &best_vector->src_port_id,
			       sizeof(struct gptp_port_identity));
		}

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
		global_ds->master_steps_removed = 0U;
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
		(gm_prio->root_system_id.grand_master_prio1 == 255U) ?
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
		__fallthrough;

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
		__fallthrough;

	case GPTP_PA_TRANSMIT_IDLE:
		k_timer_stop(&state->ann_send_periodic_timer);
		state->ann_trigger = false;
		k_timer_start(&state->ann_send_periodic_timer,
			      K_MSEC(gptp_uscaled_ns_to_timer_ms(
					     &bmca_data->announce_interval)),
			      K_NO_WAIT);

		state->state = GPTP_PA_TRANSMIT_POST_IDLE;
		__fallthrough;

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
	gptp_mi_clk_master_sync_offset_state_machine();
#if defined(CONFIG_NET_GPTP_GM_CAPABLE)
	gptp_mi_clk_master_sync_snd_state_machine();
#endif
	gptp_mi_clk_master_sync_rcv_state_machine();
}

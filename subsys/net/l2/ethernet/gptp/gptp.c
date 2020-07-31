/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_gptp, CONFIG_NET_GPTP_LOG_LEVEL);

#include <net/net_pkt.h>
#include <ptp_clock.h>
#include <net/ethernet_mgmt.h>
#include <random/rand32.h>

#include <net/gptp.h>

#include "gptp_messages.h"
#include "gptp_mi.h"
#include "gptp_data_set.h"

#include "gptp_private.h"

#define NET_GPTP_STACK_SIZE 2048

#if CONFIG_NET_GPTP_NUM_PORTS > 32
/*
 * Boolean arrays sizes have been hardcoded.
 * It has been arbitrary chosen that a system can not
 * have more than 32 ports.
 */
#error Maximum number of ports exceeded. (Max is 32).
#endif

K_KERNEL_STACK_DEFINE(gptp_stack, NET_GPTP_STACK_SIZE);
K_FIFO_DEFINE(gptp_rx_queue);

static k_tid_t tid;
static struct k_thread gptp_thread_data;
struct gptp_domain gptp_domain;

int gptp_get_port_number(struct net_if *iface)
{
	int port = net_eth_get_ptp_port(iface) + 1;

	if (port >= GPTP_PORT_START && port < GPTP_PORT_END) {
		return port;
	}

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		if (GPTP_PORT_IFACE(port) == iface) {
			return port;
		}
	}

	return -ENODEV;
}

bool gptp_is_slave_port(int port)
{
	return (GPTP_GLOBAL_DS()->selected_role[port] == GPTP_PORT_SLAVE);
}

/*
 * Use the given port to generate the clock identity
 * for the device.
 * The clock identity is unique for one time-aware system.
 */
static void gptp_compute_clock_identity(int port)
{
	struct net_if *iface = GPTP_PORT_IFACE(port);
	struct gptp_default_ds *default_ds;

	default_ds = GPTP_DEFAULT_DS();

	if (iface) {
		default_ds->clk_id[0] = net_if_get_link_addr(iface)->addr[0];
		default_ds->clk_id[1] = net_if_get_link_addr(iface)->addr[1];
		default_ds->clk_id[2] = net_if_get_link_addr(iface)->addr[2];
		default_ds->clk_id[3] = 0xFF;
		default_ds->clk_id[4] = 0xFE;
		default_ds->clk_id[5] = net_if_get_link_addr(iface)->addr[3];
		default_ds->clk_id[6] = net_if_get_link_addr(iface)->addr[4];
		default_ds->clk_id[7] = net_if_get_link_addr(iface)->addr[5];
	}
}

/* Note that we do not use log_strdup() here when printing msg as currently the
 * msg variable is always a const string that is not allocated from the stack.
 * If this changes at some point, then add log_strdup(msg) here.
 */
#define PRINT_INFO(msg, hdr, pkt)				\
	NET_DBG("Received %s seq %d pkt %p", msg,		\
		ntohs(hdr->sequence_id), pkt)			\


static bool gptp_handle_critical_msg(struct net_if *iface, struct net_pkt *pkt)
{
	struct gptp_hdr *hdr = GPTP_HDR(pkt);
	bool handled = false;
	int port;

	switch (hdr->message_type) {
	case GPTP_PATH_DELAY_REQ_MESSAGE:
		if (GPTP_CHECK_LEN(pkt, GPTP_PDELAY_REQ_LEN)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "PDELAY_REQ",
				 GPTP_PDELAY_REQ_LEN,
				 GPTP_PACKET_LEN(pkt));
			break;
		}

		PRINT_INFO("PDELAY_REQ", hdr, pkt);

		port = gptp_get_port_number(iface);
		if (port == -ENODEV) {
			NET_DBG("No port found for gPTP buffer");
			return handled;
		}

		if (GPTP_PORT_STATE(port)->pdelay_resp.state !=
						GPTP_PDELAY_RESP_NOT_ENABLED) {
			gptp_handle_pdelay_req(port, pkt);
		}

		handled = true;
		break;
	default:
		/* Not a critical message, this will be handled later. */
		break;
	}

	return handled;
}

static void gptp_handle_msg(struct net_pkt *pkt)
{
	struct gptp_hdr *hdr = GPTP_HDR(pkt);
	struct gptp_pdelay_req_state *pdelay_req_state;
	struct gptp_sync_rcv_state *sync_rcv_state;
	struct gptp_port_announce_receive_state *pa_rcv_state;
	struct gptp_port_bmca_data *bmca_data;
	int port;

	port = gptp_get_port_number(net_pkt_iface(pkt));
	if (port == -ENODEV) {
		NET_DBG("No port found for ptp buffer");
		return;
	}

	pdelay_req_state = &GPTP_PORT_STATE(port)->pdelay_req;
	sync_rcv_state = &GPTP_PORT_STATE(port)->sync_rcv;

	switch (hdr->message_type) {
	case GPTP_SYNC_MESSAGE:
		if (GPTP_CHECK_LEN(pkt, GPTP_SYNC_LEN)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "SYNC",
				 GPTP_SYNC_LEN,
				 GPTP_PACKET_LEN(pkt));
			GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
			break;
		}

		PRINT_INFO("SYNC", hdr, pkt);

		sync_rcv_state->rcvd_sync = true;

		/* If we already have one, drop the previous one. */
		if (sync_rcv_state->rcvd_sync_ptr) {
			net_pkt_unref(sync_rcv_state->rcvd_sync_ptr);
		}

		/* Keep the buffer alive until follow_up is received. */
		net_pkt_ref(pkt);
		sync_rcv_state->rcvd_sync_ptr = pkt;

		GPTP_STATS_INC(port, rx_sync_count);
		break;

	case GPTP_DELAY_REQ_MESSAGE:
		NET_DBG("Delay Request not handled.");
		break;

	case GPTP_PATH_DELAY_REQ_MESSAGE:
		/*
		 * Path Delay Responses to Path Delay Requests need
		 * very low latency. These need to handled in priority
		 * when received as they cannot afford to be delayed
		 * by context switches.
		 */
		NET_WARN("Path Delay Request received as normal messages!");
		GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
		break;

	case GPTP_PATH_DELAY_RESP_MESSAGE:
		if (GPTP_CHECK_LEN(pkt, GPTP_PDELAY_RESP_LEN)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "PDELAY_RESP",
				 GPTP_PDELAY_RESP_LEN,
				 GPTP_PACKET_LEN(pkt));
			GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
			break;
		}

		PRINT_INFO("PDELAY_RESP", hdr, pkt);

		pdelay_req_state->rcvd_pdelay_resp++;

		/* If we already have one, drop the received one. */
		if (pdelay_req_state->rcvd_pdelay_resp_ptr) {
			break;
		}

		/* Keep the buffer alive until pdelay_rate_ratio is computed. */
		net_pkt_ref(pkt);
		pdelay_req_state->rcvd_pdelay_resp_ptr = pkt;
		break;

	case GPTP_FOLLOWUP_MESSAGE:
		if (GPTP_CHECK_LEN(pkt, GPTP_FOLLOW_UP_LEN)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "FOLLOWUP",
				 GPTP_FOLLOW_UP_LEN,
				 GPTP_PACKET_LEN(pkt));
			GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
			break;
		}

		PRINT_INFO("FOLLOWUP", hdr, pkt);

		sync_rcv_state->rcvd_follow_up = true;

		/* If we already have one, drop the previous one. */
		if (sync_rcv_state->rcvd_follow_up_ptr) {
			net_pkt_unref(sync_rcv_state->rcvd_follow_up_ptr);
		}

		/* Keep the pkt alive until info is extracted. */
		sync_rcv_state->rcvd_follow_up_ptr = net_pkt_ref(pkt);
		NET_DBG("Keeping %s seq %d pkt %p", "FOLLOWUP",
			ntohs(hdr->sequence_id), pkt);
		break;

	case GPTP_PATH_DELAY_FOLLOWUP_MESSAGE:
		if (GPTP_CHECK_LEN(pkt, GPTP_PDELAY_RESP_FUP_LEN)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "PDELAY_FOLLOWUP",
				 GPTP_PDELAY_RESP_FUP_LEN,
				 GPTP_PACKET_LEN(pkt));
			GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
			break;
		}

		PRINT_INFO("PDELAY_FOLLOWUP", hdr, pkt);

		pdelay_req_state->rcvd_pdelay_follow_up++;

		/* If we already have one, drop the received one. */
		if (pdelay_req_state->rcvd_pdelay_follow_up_ptr) {
			break;
		}

		/* Keep the buffer alive until pdelay_rate_ratio is computed. */
		net_pkt_ref(pkt);
		pdelay_req_state->rcvd_pdelay_follow_up_ptr = pkt;

		GPTP_STATS_INC(port, rx_pdelay_resp_fup_count);
		break;

	case GPTP_ANNOUNCE_MESSAGE:
		if (GPTP_ANNOUNCE_CHECK_LEN(pkt)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "ANNOUNCE",
				 GPTP_ANNOUNCE_LEN(pkt),
				 GPTP_PACKET_LEN(pkt));
			GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
			break;
		}

		PRINT_INFO("ANNOUNCE", hdr, pkt);

		pa_rcv_state = &GPTP_PORT_STATE(port)->pa_rcv;
		bmca_data = GPTP_PORT_BMCA_DATA(port);

		if (pa_rcv_state->rcvd_announce == false &&
				bmca_data->rcvd_announce_ptr == NULL) {
			pa_rcv_state->rcvd_announce = true;
			bmca_data->rcvd_announce_ptr = pkt;
			net_pkt_ref(pkt);
		}

		GPTP_STATS_INC(port, rx_announce_count);
		break;

	case GPTP_SIGNALING_MESSAGE:
		if (GPTP_CHECK_LEN(pkt, GPTP_SIGNALING_LEN)) {
			NET_WARN("Invalid length for %s packet "
				 "should have %zd bytes but has %zd bytes",
				 "SIGNALING",
				 GPTP_SIGNALING_LEN,
				 GPTP_PACKET_LEN(pkt));
			GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
			break;
		}

		PRINT_INFO("SIGNALING", hdr, pkt);

		gptp_handle_signaling(port, pkt);
		break;

	case GPTP_MANAGEMENT_MESSAGE:
		PRINT_INFO("MANAGEMENT", hdr, pkt);
		GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
		break;

	default:
		NET_DBG("Received unknown message %x", hdr->message_type);
		GPTP_STATS_INC(port, rx_ptp_packet_discard_count);
		break;
	}
}

enum net_verdict net_gptp_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct gptp_hdr *hdr = GPTP_HDR(pkt);

	if ((hdr->ptp_version != GPTP_VERSION) ||
			(hdr->transport_specific != GPTP_TRANSPORT_802_1_AS)) {
		/* The stack only supports PTP V2 and transportSpecific set
		 * to 1 with IEEE802.1AS-2011.
		 */
		return NET_DROP;
	}

	/* Handle critical messages. */
	if (!gptp_handle_critical_msg(iface, pkt)) {
		k_fifo_put(&gptp_rx_queue, pkt);

		/* Returning OK here makes sure the network statistics are
		 * properly updated.
		 */
		return NET_OK;
	}

	/* Message not propagated up in the stack. */
	return NET_DROP;
}

static void gptp_init_clock_ds(void)
{
	struct gptp_global_ds *global_ds;
	struct gptp_default_ds *default_ds;
	struct gptp_current_ds *current_ds;
	struct gptp_parent_ds *parent_ds;
	struct gptp_time_prop_ds *prop_ds;

	global_ds = GPTP_GLOBAL_DS();
	default_ds = GPTP_DEFAULT_DS();
	current_ds = GPTP_CURRENT_DS();
	parent_ds = GPTP_PARENT_DS();
	prop_ds = GPTP_PROPERTIES_DS();

	/* Initialize global data set. */
	(void)memset(global_ds, 0, sizeof(struct gptp_global_ds));

	/* Initialize default data set. */

	/* Compute the clock identity from the first port MAC address. */
	gptp_compute_clock_identity(GPTP_PORT_START);

	default_ds->gm_capable = IS_ENABLED(CONFIG_NET_GPTP_GM_CAPABLE);
	default_ds->clk_quality.clock_class = GPTP_CLASS_OTHER;
	default_ds->clk_quality.clock_accuracy =
		CONFIG_NET_GPTP_CLOCK_ACCURACY;
	default_ds->clk_quality.offset_scaled_log_var =
		GPTP_OFFSET_SCALED_LOG_VAR_UNKNOWN;

	if (default_ds->gm_capable) {
		default_ds->priority1 = GPTP_PRIORITY1_GM_CAPABLE;
	} else {
		default_ds->priority1 = GPTP_PRIORITY1_NON_GM_CAPABLE;
	}

	default_ds->priority2 = GPTP_PRIORITY2_DEFAULT;

	default_ds->cur_utc_offset = 37U; /* Current leap seconds TAI - UTC */
	default_ds->flags.all = 0U;
	default_ds->flags.octets[1] = GPTP_FLAG_TIME_TRACEABLE;
	default_ds->time_source = GPTP_TS_INTERNAL_OSCILLATOR;

	/* Initialize current data set. */
	(void)memset(current_ds, 0, sizeof(struct gptp_current_ds));

	/* Initialize parent data set. */

	/* parent clock id is initialized to default_ds clock id. */
	memcpy(parent_ds->port_id.clk_id, default_ds->clk_id,
	       GPTP_CLOCK_ID_LEN);
	memcpy(parent_ds->gm_id, default_ds->clk_id, GPTP_CLOCK_ID_LEN);
	parent_ds->port_id.port_number = 0U;

	/* TODO: Check correct value for below field. */
	parent_ds->cumulative_rate_ratio = 0;

	parent_ds->gm_clk_quality.clock_class =
		default_ds->clk_quality.clock_class;
	parent_ds->gm_clk_quality.clock_accuracy =
		default_ds->clk_quality.clock_accuracy;
	parent_ds->gm_clk_quality.offset_scaled_log_var =
		default_ds->clk_quality.offset_scaled_log_var;
	parent_ds->gm_priority1 = default_ds->priority1;
	parent_ds->gm_priority2 = default_ds->priority2;

	/* Initialize properties data set. */

	/* TODO: Get accurate values for below. From the GM. */
	prop_ds->cur_utc_offset = 37U; /* Current leap seconds TAI - UTC */
	prop_ds->cur_utc_offset_valid = false;
	prop_ds->leap59 = false;
	prop_ds->leap61 = false;
	prop_ds->time_traceable = false;
	prop_ds->freq_traceable = false;
	prop_ds->time_source = GPTP_TS_INTERNAL_OSCILLATOR;

	/* Set system values. */
	global_ds->sys_flags.all = default_ds->flags.all;
	global_ds->sys_current_utc_offset = default_ds->cur_utc_offset;
	global_ds->sys_time_source = default_ds->time_source;
	global_ds->clk_master_sync_itv =
		NSEC_PER_SEC * GPTP_POW2(CONFIG_NET_GPTP_INIT_LOG_SYNC_ITV);
}

static void gptp_init_port_ds(int port)
{
	struct gptp_default_ds *default_ds;
	struct gptp_port_ds *port_ds;

#if defined(CONFIG_NET_GPTP_STATISTICS)
	struct gptp_port_param_ds *port_param_ds;

	port_param_ds = GPTP_PORT_PARAM_DS(port);
#endif

	default_ds = GPTP_DEFAULT_DS();
	port_ds = GPTP_PORT_DS(port);

	/* Initialize port data set. */
	memcpy(port_ds->port_id.clk_id, default_ds->clk_id, GPTP_CLOCK_ID_LEN);
	port_ds->port_id.port_number = port;

	port_ds->ptt_port_enabled = true;
	port_ds->prev_ptt_port_enabled = true;

	port_ds->neighbor_prop_delay = 0;
	port_ds->neighbor_prop_delay_thresh = GPTP_NEIGHBOR_PROP_DELAY_THR;
	port_ds->delay_asymmetry = 0;

	port_ds->ini_log_announce_itv = CONFIG_NET_GPTP_INIT_LOG_ANNOUNCE_ITV;
	port_ds->cur_log_announce_itv = port_ds->ini_log_announce_itv;
	port_ds->announce_receipt_timeout =
		CONFIG_NET_GPTP_ANNOUNCE_RECEIPT_TIMEOUT;

	/* Subtract 1 to divide by 2 the sync interval. */
	port_ds->ini_log_half_sync_itv = CONFIG_NET_GPTP_INIT_LOG_SYNC_ITV - 1;
	port_ds->cur_log_half_sync_itv = port_ds->ini_log_half_sync_itv;
	port_ds->sync_receipt_timeout = CONFIG_NET_GPTP_SYNC_RECEIPT_TIMEOUT;
	port_ds->sync_receipt_timeout_time_itv = 10000000U; /* 10ms */

	port_ds->ini_log_pdelay_req_itv =
		CONFIG_NET_GPTP_INIT_LOG_PDELAY_REQ_ITV;
	port_ds->cur_log_pdelay_req_itv = port_ds->ini_log_pdelay_req_itv;
	port_ds->allowed_lost_responses = GPTP_ALLOWED_LOST_RESP;
	port_ds->version = GPTP_VERSION;

	gptp_set_time_itv(&port_ds->pdelay_req_itv, 1,
			  port_ds->cur_log_pdelay_req_itv);

	gptp_set_time_itv(&port_ds->half_sync_itv, 1,
			  port_ds->cur_log_half_sync_itv);

	port_ds->compute_neighbor_rate_ratio = true;
	port_ds->compute_neighbor_prop_delay = true;

	/* Random Sequence Numbers. */
	port_ds->sync_seq_id = (uint16_t)sys_rand32_get();
	port_ds->pdelay_req_seq_id = (uint16_t)sys_rand32_get();
	port_ds->announce_seq_id = (uint16_t)sys_rand32_get();
	port_ds->signaling_seq_id = (uint16_t)sys_rand32_get();

#if defined(CONFIG_NET_GPTP_STATISTICS)
	/* Initialize stats data set. */
	(void)memset(port_param_ds, 0, sizeof(struct gptp_port_param_ds));
#endif
}

static void gptp_init_state_machine(void)
{
	gptp_md_init_state_machine();
	gptp_mi_init_state_machine();
}

static void gptp_state_machine(void)
{
	int port;

	/* Manage port states. */
	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		struct gptp_port_ds *port_ds = GPTP_PORT_DS(port);

		switch (GPTP_GLOBAL_DS()->selected_role[port]) {
		case GPTP_PORT_DISABLED:
		case GPTP_PORT_MASTER:
		case GPTP_PORT_PASSIVE:
		case GPTP_PORT_SLAVE:
			gptp_md_state_machines(port);
			gptp_mi_port_sync_state_machines(port);
			gptp_mi_port_bmca_state_machines(port);
			break;
		default:
			NET_DBG("%s: Unknown port state", __func__);
			break;
		}

		port_ds->prev_ptt_port_enabled = port_ds->ptt_port_enabled;
	}

	gptp_mi_state_machines();
}

static void gptp_thread(void)
{
	int port;

	NET_DBG("Starting PTP thread");

	gptp_init_clock_ds();

	for (port = GPTP_PORT_START; port < GPTP_PORT_END; port++) {
		gptp_init_port_ds(port);
		gptp_change_port_state(port, GPTP_PORT_DISABLED);
	}

	while (1) {
		struct net_pkt *pkt;

		pkt = k_fifo_get(&gptp_rx_queue,
				 K_MSEC(GPTP_THREAD_WAIT_TIMEOUT_MS));
		if (pkt) {
			gptp_handle_msg(pkt);
			net_pkt_unref(pkt);
		}

		gptp_state_machine();
	}
}


static void gptp_add_port(struct net_if *iface, void *user_data)
{
	int *num_ports = user_data;
	struct device *clk;

	if (*num_ports >= CONFIG_NET_GPTP_NUM_PORTS) {
		return;
	}

#if defined(CONFIG_NET_GPTP_VLAN)
	if (CONFIG_NET_GPTP_VLAN_TAG >= 0 &&
	    CONFIG_NET_GPTP_VLAN_TAG < NET_VLAN_TAG_UNSPEC) {
		struct net_if *vlan_iface;

		vlan_iface = net_eth_get_vlan_iface(iface,
						    CONFIG_NET_GPTP_VLAN_TAG);
		if (vlan_iface != iface) {
			return;
		}
	}
#endif /* CONFIG_NET_GPTP_VLAN */

	/* Check if interface has a PTP clock. */
	clk = net_eth_get_ptp_clock(iface);
	if (clk) {
		gptp_domain.iface[*num_ports] = iface;
		net_eth_set_ptp_port(iface, *num_ports);
		(*num_ports)++;
	}
}

void gptp_set_time_itv(struct gptp_uscaled_ns *interval,
		       uint16_t seconds,
		       int8_t log_msg_interval)
{
	int i;

	if (seconds == 0U) {
		interval->low = 0U;
		interval->high = 0U;
		return;
	} else if (log_msg_interval >= 96) {
		/* Overflow, set maximum. */
		interval->low = UINT64_MAX;
		interval->high = UINT32_MAX;

		return;
	} else if (log_msg_interval <= -64) {
		/* Underflow, set to 0. */
		interval->low = 0U;
		interval->high = 0U;
		return;
	}


	/* NSEC_PER_SEC is between 2^30 and 2^31, seconds is less thant 2^16,
	 * thus the computation will be less than 2^63.
	 */
	interval->low =	(seconds * (uint64_t)NSEC_PER_SEC) << 16;

	if (log_msg_interval <= 0) {
		interval->low >>= -log_msg_interval;
		interval->high = 0U;
	} else {
		/* Find highest bit set. */
		for (i = 63; i >= 0; i--) {
			if (interval->low >> i) {
				break;
			}
		}

		if ((i + log_msg_interval) >= 96 || log_msg_interval > 64) {
			/* Overflow, set maximum. */
			interval->low = UINT64_MAX;
			interval->high = UINT32_MAX;
		} else {
			interval->high =
				interval->low >> (64 - log_msg_interval);

			/* << operator is undefined if the shift value is equal
			 * to the number of bits in the left expression’s type
			 */
			if (log_msg_interval == 64) {
				interval->low = 0U;
			} else {
				interval->low <<= log_msg_interval;
			}
		}
	}
}

int32_t gptp_uscaled_ns_to_timer_ms(struct gptp_uscaled_ns *usns)
{
	uint64_t tmp;

	if (usns->high) {
		/* Do not calculate, it reaches max value. */
		return INT32_MAX;
	}

	tmp = (usns->low >> 16) / USEC_PER_SEC;
	if (tmp == 0U) {
		/* Timer must be started with a minimum value of 1. */
		return 1;
	}

	if (tmp > INT32_MAX) {
		return INT32_MAX;
	}

	return (tmp & INT32_MAX);

}

static int32_t timer_get_remaining_and_stop(struct k_timer *timer)
{
	unsigned int key;
	int32_t timer_value;

	key = irq_lock();
	timer_value = k_timer_remaining_get(timer);

	/* Stop timer as the period is about to be modified. */
	k_timer_stop(timer);
	irq_unlock(key);

	return timer_value;
}

static int32_t update_itv(struct gptp_uscaled_ns *itv,
			 int8_t *cur_log_itv,
			 int8_t *ini_log_itv,
			 int8_t new_log_itv,
			 int8_t correction_log_itv)
{
	switch (new_log_itv) {
	case GPTP_ITV_KEEP:
		break;
	case GPTP_ITV_SET_TO_INIT:
		*cur_log_itv = *ini_log_itv;
		gptp_set_time_itv(itv, 1, *ini_log_itv);
		break;
	case GPTP_ITV_STOP:
	default:
		*cur_log_itv = new_log_itv + correction_log_itv;
		gptp_set_time_itv(itv, 1, *cur_log_itv);
		break;
	}

	return gptp_uscaled_ns_to_timer_ms(itv);
}

void gptp_update_pdelay_req_interval(int port, int8_t log_val)
{
	int32_t remaining;
	int32_t new_itv, old_itv;
	struct gptp_pdelay_req_state *state_pdelay;
	struct gptp_port_ds *port_ds;

	port_ds = GPTP_PORT_DS(port);
	state_pdelay = &GPTP_PORT_STATE(port)->pdelay_req;
	remaining = timer_get_remaining_and_stop(&state_pdelay->pdelay_timer);

	old_itv = gptp_uscaled_ns_to_timer_ms(&port_ds->pdelay_req_itv);
	new_itv = update_itv(&port_ds->pdelay_req_itv,
			     &port_ds->cur_log_pdelay_req_itv,
			     &port_ds->ini_log_pdelay_req_itv,
			     log_val,
			     0);

	new_itv -= (old_itv-remaining);
	if (new_itv <= 0) {
		new_itv = 1;
	}

	k_timer_start(&state_pdelay->pdelay_timer, K_MSEC(new_itv), K_NO_WAIT);
}

void gptp_update_sync_interval(int port, int8_t log_val)
{
	struct gptp_pss_send_state *state_pss_send;
	struct gptp_port_ds *port_ds;
	int32_t new_itv, old_itv, period;
	int32_t remaining;
	uint32_t time_spent;

	port_ds = GPTP_PORT_DS(port);
	state_pss_send = &GPTP_PORT_STATE(port)->pss_send;
	remaining =
		timer_get_remaining_and_stop(
				&state_pss_send->half_sync_itv_timer);
	old_itv = gptp_uscaled_ns_to_timer_ms(&port_ds->half_sync_itv);
	new_itv = update_itv(&port_ds->half_sync_itv,
			     &port_ds->cur_log_half_sync_itv,
			     &port_ds->ini_log_half_sync_itv,
			     log_val,
			     -1);
	period = new_itv;

	/* Get the time spent from the start of the timer. */
	time_spent = old_itv;
	if (state_pss_send->half_sync_itv_timer_expired) {
		time_spent *= 2U;
	}
	time_spent -= remaining;

	/* Calculate remaining time and if half timer has expired. */
	if ((time_spent / 2U) > new_itv) {
		state_pss_send->sync_itv_timer_expired = true;
		state_pss_send->half_sync_itv_timer_expired = true;
		new_itv = 1;
	} else if (time_spent > new_itv) {
		state_pss_send->sync_itv_timer_expired = false;
		state_pss_send->half_sync_itv_timer_expired = true;
		new_itv -= (time_spent - new_itv);
	} else {
		state_pss_send->sync_itv_timer_expired = false;
		state_pss_send->half_sync_itv_timer_expired = false;
		new_itv -= time_spent;
	}

	if (new_itv <= 0) {
		new_itv = 1;
	}

	k_timer_start(&state_pss_send->half_sync_itv_timer, K_MSEC(new_itv),
		      K_MSEC(period));
}

void gptp_update_announce_interval(int port, int8_t log_val)
{
	int32_t remaining;
	int32_t new_itv, old_itv;
	struct gptp_port_announce_transmit_state *state_ann;
	struct gptp_port_bmca_data *bmca_data;
	struct gptp_port_ds *port_ds;

	port_ds = GPTP_PORT_DS(port);
	state_ann = &GPTP_PORT_STATE(port)->pa_transmit;
	bmca_data = GPTP_PORT_BMCA_DATA(port);
	remaining = timer_get_remaining_and_stop(
			&state_ann->ann_send_periodic_timer);

	old_itv = gptp_uscaled_ns_to_timer_ms(&bmca_data->announce_interval);
	new_itv = update_itv(&bmca_data->announce_interval,
			     &port_ds->cur_log_announce_itv,
			     &port_ds->ini_log_announce_itv,
			     log_val,
			     0);

	new_itv -= (old_itv-remaining);
	if (new_itv <= 0) {
		new_itv = 1;
	}

	k_timer_start(&state_ann->ann_send_periodic_timer, K_MSEC(new_itv),
		      K_NO_WAIT);
}

struct port_user_data {
	gptp_port_cb_t cb;
	void *user_data;
};

static void gptp_get_port(struct net_if *iface, void *user_data)
{
	struct port_user_data *ud = user_data;
	struct device *clk;

	/* Check if interface has a PTP clock. */
	clk = net_eth_get_ptp_clock(iface);
	if (clk) {
		int port = gptp_get_port_number(iface);

		if (port < 0) {
			return;
		}

		ud->cb(port, iface, ud->user_data);
	}
}

void gptp_foreach_port(gptp_port_cb_t cb, void *user_data)
{
	struct port_user_data ud = {
		.cb = cb,
		.user_data = user_data
	};

	net_if_foreach(gptp_get_port, &ud);
}

struct gptp_domain *gptp_get_domain(void)
{
	return &gptp_domain;
}

int gptp_get_port_data(struct gptp_domain *domain,
		       int port,
		       struct gptp_port_ds **port_ds,
		       struct gptp_port_param_ds **port_param_ds,
		       struct gptp_port_states **port_state,
		       struct gptp_port_bmca_data **port_bmca_data,
		       struct net_if **iface)
{
	if (domain != &gptp_domain) {
		return -ENOENT;
	}

	if (port < GPTP_PORT_START || port >= GPTP_PORT_END) {
		return -EINVAL;
	}

	if (port_ds) {
		*port_ds = GPTP_PORT_DS(port);
	}

	if (port_param_ds) {
#if defined(CONFIG_NET_GPTP_STATISTICS)
		*port_param_ds = GPTP_PORT_PARAM_DS(port);
#else
		*port_param_ds = NULL;
#endif
	}

	if (port_state) {
		*port_state = GPTP_PORT_STATE(port);
	}

	if (port_bmca_data) {
		*port_bmca_data = GPTP_PORT_BMCA_DATA(port);
	}

	if (iface) {
		*iface = GPTP_PORT_IFACE(port);
	}

	return 0;
}

static void init_ports(void)
{
	net_if_foreach(gptp_add_port, &gptp_domain.default_ds.nb_ports);

	/* Only initialize the state machine once the ports are known. */
	gptp_init_state_machine();

	tid = k_thread_create(&gptp_thread_data, gptp_stack,
			      K_KERNEL_STACK_SIZEOF(gptp_stack),
			      (k_thread_entry_t)gptp_thread,
			      NULL, NULL, NULL, K_PRIO_COOP(5), 0, K_NO_WAIT);
	k_thread_name_set(&gptp_thread_data, "gptp");
}

#if defined(CONFIG_NET_GPTP_VLAN)
static struct net_mgmt_event_callback vlan_cb;

struct vlan_work {
	struct k_work work;
	struct net_if *iface;
} vlan;

static void disable_port(int port)
{
	GPTP_GLOBAL_DS()->selected_role[port] = GPTP_PORT_DISABLED;

	gptp_state_machine();
}

static void vlan_enabled(struct k_work *work)
{
	struct vlan_work *vlan = CONTAINER_OF(work,
					      struct vlan_work,
					      work);
	if (tid) {
		int port;

		port = gptp_get_port_number(vlan->iface);
		if (port < 0) {
			NET_DBG("No port found for iface %p", vlan->iface);
			return;
		}

		GPTP_GLOBAL_DS()->selected_role[port] = GPTP_PORT_SLAVE;

		gptp_state_machine();
	} else {
		init_ports();
	}
}

static void vlan_disabled(struct k_work *work)
{
	struct vlan_work *vlan = CONTAINER_OF(work,
					      struct vlan_work,
					      work);
	int port;

	port = gptp_get_port_number(vlan->iface);
	if (port < 0) {
		NET_DBG("No port found for iface %p", vlan->iface);
		return;
	}

	disable_port(port);
}

static void vlan_event_handler(struct net_mgmt_event_callback *cb,
			       uint32_t mgmt_event,
			       struct net_if *iface)
{
	uint16_t tag;

	if (mgmt_event != NET_EVENT_ETHERNET_VLAN_TAG_ENABLED &&
	    mgmt_event != NET_EVENT_ETHERNET_VLAN_TAG_DISABLED) {
		return;
	}

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	if (!cb->info) {
		return;
	}

	tag = *((uint16_t *)cb->info);
	if (tag != CONFIG_NET_GPTP_VLAN_TAG) {
		return;
	}

	vlan.iface = iface;

	if (mgmt_event == NET_EVENT_ETHERNET_VLAN_TAG_ENABLED) {
		/* We found the right tag, now start gPTP for this interface */
		k_work_init(&vlan.work, vlan_enabled);

		NET_DBG("VLAN tag %d %s for iface %p", tag, "enabled", iface);
	} else {
		k_work_init(&vlan.work, vlan_disabled);

		NET_DBG("VLAN tag %d %s for iface %p", tag, "disabled", iface);
	}

	k_work_submit(&vlan.work);
#else
	NET_WARN("VLAN event but tag info missing!");

	ARG_UNUSED(tag);
#endif
}

static void setup_vlan_events_listener(void)
{
	net_mgmt_init_event_callback(&vlan_cb, vlan_event_handler,
				     NET_EVENT_ETHERNET_VLAN_TAG_ENABLED |
				     NET_EVENT_ETHERNET_VLAN_TAG_DISABLED);
	net_mgmt_add_event_callback(&vlan_cb);
}
#endif /* CONFIG_NET_GPTP_VLAN */

void net_gptp_init(void)
{
	gptp_domain.default_ds.nb_ports = 0U;

#if defined(CONFIG_NET_GPTP_VLAN)
	/* If user has enabled gPTP over VLAN support, then we start gPTP
	 * support after we have received correct "VLAN tag enabled" event.
	 */
	if (CONFIG_NET_GPTP_VLAN_TAG >= 0 &&
	    CONFIG_NET_GPTP_VLAN_TAG < NET_VLAN_TAG_UNSPEC) {
		setup_vlan_events_listener();
	} else {
		NET_WARN("VLAN tag %d set but the value is not valid.",
			 CONFIG_NET_GPTP_VLAN_TAG);

		init_ports();
	}
#else
	init_ports();
#endif
}

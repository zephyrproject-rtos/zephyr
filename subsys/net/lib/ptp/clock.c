/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_clock, CONFIG_PTP_LOG_LEVEL);

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/slist.h>

#include "btca.h"
#include "clock.h"
#include "ddt.h"
#include "msg.h"
#include "port.h"
#include "tlv.h"
#include "transport.h"

#define MIN_NSEC_TO_TIMEINTERVAL (0xFFFF800000000000ULL)
#define MAX_NSEC_TO_TIMEINTERVAL (0x00007FFFFFFFFFFFULL)

/**
 * @brief PTP Clock structure.
 */
struct ptp_clock {
	const struct device	    *phc;
	struct ptp_default_ds	    default_ds;
	struct ptp_current_ds	    current_ds;
	struct ptp_parent_ds	    parent_ds;
	struct ptp_time_prop_ds	    time_prop_ds;
	struct ptp_dataset	    dataset;
	struct ptp_foreign_tt_clock *best;
	sys_slist_t		    ports_list;
	struct zsock_pollfd	    pollfd[2 * CONFIG_PTP_NUM_PORTS];
	bool			    pollfd_valid;
	bool			    state_decision_event;
	uint8_t			    time_src;
	struct {
		uint64_t	    t1;
		uint64_t	    t2;
		uint64_t	    t3;
		uint64_t	    t4;
	} timestamp;			/* latest timestamps in nanoseconds */
};

static struct ptp_clock clock = { 0 };
char str_clock_id[] = "FF:FF:FF:FF:FF:FF:FF:FF";

static int clock_generate_id(ptp_clk_id *clock_id, struct net_if *iface)
{
	struct net_linkaddr *addr = net_if_get_link_addr(iface);

	if (addr) {
		clock_id->id[0] = addr->addr[0];
		clock_id->id[1] = addr->addr[1];
		clock_id->id[2] = addr->addr[2];
		clock_id->id[3] = 0xFF;
		clock_id->id[4] = 0xFE;
		clock_id->id[5] = addr->addr[3];
		clock_id->id[6] = addr->addr[4];
		clock_id->id[7] = addr->addr[5];
		return 0;
	}
	return -1;
}

static const char *clock_id_str(ptp_clk_id *clock_id)
{
	uint8_t *cid = clock_id->id;

	snprintk(str_clock_id, sizeof(str_clock_id), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
		 cid[0],
		 cid[1],
		 cid[2],
		 cid[3],
		 cid[4],
		 cid[5],
		 cid[6],
		 cid[7]);

	return str_clock_id;
}

static ptp_timeinterval clock_ns_to_timeinterval(int64_t val)
{
	if (val < (int64_t)MIN_NSEC_TO_TIMEINTERVAL) {
		val = MIN_NSEC_TO_TIMEINTERVAL;
	} else if (val > (int64_t)MAX_NSEC_TO_TIMEINTERVAL) {
		val = MAX_NSEC_TO_TIMEINTERVAL;
	}

	return (uint64_t)val << 16;
}

static int clock_forward_msg(struct ptp_port *ingress,
			     struct ptp_port *port,
			     struct ptp_msg *msg,
			     bool *network_byte_order)
{
	if (ingress == port) {
		return 0;
	}

	if (*network_byte_order == false) {
		ptp_msg_pre_send(msg);
		*network_byte_order = true;
	}

	return ptp_transport_send(port, msg, PTP_SOCKET_GENERAL);
}

static void clock_forward_management_msg(struct ptp_port *port, struct ptp_msg *msg)
{
	int length;
	struct ptp_port *iter;
	bool net_byte_ord = false;
	enum ptp_port_state state = ptp_port_state(port);

	if (ptp_clock_type() != PTP_CLOCK_TYPE_BOUNDARY) {
		/* Clocks other than Boundary Clock shouldn't retransmit messages. */
		return;
	}

	if (msg->header.flags[0] & PTP_MSG_UNICAST_FLAG) {
		return;
	}

	if (msg->management.boundary_hops &&
	    (state == PTP_PS_GRAND_MASTER ||
	     state == PTP_PS_TIME_TRANSMITTER ||
	     state == PTP_PS_PRE_TIME_TRANSMITTER ||
	     state == PTP_PS_TIME_RECEIVER ||
	     state == PTP_PS_UNCALIBRATED)) {
		length = msg->header.msg_length;
		msg->management.boundary_hops--;

		SYS_SLIST_FOR_EACH_CONTAINER(&clock.ports_list, iter, node) {
			if (clock_forward_msg(port, iter, msg, &net_byte_ord)) {
				LOG_ERR("Failed to forward message to %d Port",
					iter->port_ds.id.port_number);
			}
		}

		if (net_byte_ord) {
			ptp_msg_post_recv(port, msg, length);
			msg->management.boundary_hops++;
		}
	}
}

static int clock_management_set(struct ptp_port *port,
				struct ptp_msg *req,
				struct ptp_tlv_mgmt *tlv)
{
	bool send_resp = false;

	switch (tlv->id) {
	case PTP_MGMT_PRIORITY1:
		clock.default_ds.priority1 = *tlv->data;
		send_resp = true;
		break;
	case PTP_MGMT_PRIORITY2:
		clock.default_ds.priority2 = *tlv->data;
		send_resp = true;
		break;
	default:
		break;
	}

	return send_resp ? ptp_port_management_resp(port, req, tlv) : 0;
}

static void clock_update_grandmaster(void)
{
	memset(&clock.current_ds, 0, sizeof(struct ptp_current_ds));

	memcpy(&clock.parent_ds.port_id.clk_id,
	       &clock.default_ds.clk_id,
	       sizeof(ptp_clk_id));
	memcpy(&clock.parent_ds.gm_id,
	       &clock.default_ds.clk_id,
	       sizeof(ptp_clk_id));
	clock.parent_ds.port_id.port_number = 0;
	clock.parent_ds.gm_clk_quality = clock.default_ds.clk_quality;
	clock.parent_ds.gm_priority1 = clock.default_ds.priority1;
	clock.parent_ds.gm_priority2 = clock.default_ds.priority2;

	clock.time_prop_ds.current_utc_offset = 37; /* IEEE 1588-2019 9.4 */
	clock.time_prop_ds.time_src = clock.time_src;
	clock.time_prop_ds.flags = 0;
}

static void clock_update_time_receiver(void)
{
	struct ptp_msg *best_msg = (struct ptp_msg *)k_fifo_peek_tail(&clock.best->messages);

	clock.current_ds.steps_rm = 1 + clock.best->dataset.steps_rm;

	memcpy(&clock.parent_ds.gm_id,
	       &best_msg->announce.gm_id,
	       sizeof(best_msg->announce.gm_id));
	memcpy(&clock.parent_ds.port_id,
	       &clock.best->dataset.sender,
	       sizeof(clock.best->dataset.sender));
	clock.parent_ds.gm_clk_quality = best_msg->announce.gm_clk_quality;
	clock.parent_ds.gm_priority1 = best_msg->announce.gm_priority1;
	clock.parent_ds.gm_priority2 = best_msg->announce.gm_priority2;

	clock.time_prop_ds.current_utc_offset = best_msg->announce.current_utc_offset;
	clock.time_prop_ds.flags = best_msg->header.flags[1];
}

const struct ptp_clock *ptp_clock_init(void)
{
	struct ptp_default_ds *dds = &clock.default_ds;
	struct ptp_parent_ds *pds  = &clock.parent_ds;
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	clock.time_src = (enum ptp_time_src)PTP_TIME_SRC_INTERNAL_OSC;

	/* Initialize Default Dataset. */
	int ret = clock_generate_id(&dds->clk_id, iface);

	if (ret) {
		LOG_ERR("Couldn't assign Clock Identity.");
		return NULL;
	}

	dds->type = (enum ptp_clock_type)CONFIG_PTP_CLOCK_TYPE;
	dds->n_ports = 0;
	dds->time_receiver_only = IS_ENABLED(CONFIG_PTP_TIME_RECEIVER_ONLY) ? true : false;

	dds->clk_quality.class = dds->time_receiver_only ? 255 : 248;
	dds->clk_quality.accuracy = CONFIG_PTP_CLOCK_ACCURACY;
	/* 0xFFFF means that value has not been computed - IEEE 1588-2019 7.6.3.3 */
	dds->clk_quality.offset_scaled_log_variance = 0xFFFF;

	dds->max_steps_rm = 255;

	dds->priority1 = CONFIG_PTP_PRIORITY1;
	dds->priority2 = CONFIG_PTP_PRIORITY2;

	/* Initialize Parent Dataset. */
	clock_update_grandmaster();
	pds->obsreved_parent_offset_scaled_log_variance = 0xFFFF;
	pds->obsreved_parent_clk_phase_change_rate = 0x7FFFFFFF;
	/* Parent statistics haven't been measured - IEEE 1588-2019 7.6.4.2 */
	pds->stats = false;

	clock.phc = net_eth_get_ptp_clock(iface);
	if (!clock.phc) {
		LOG_ERR("Couldn't get PTP HW Clock for the interface.");
		return NULL;
	}

	sys_slist_init(&clock.ports_list);
	LOG_DBG("PTP Clock %s initialized", clock_id_str(&dds->clk_id));
	return &clock;
}

int ptp_clock_management_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	static const ptp_clk_id all_ones = {
		.id = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
	};
	int ret;
	bool state_decision_required = false;
	enum ptp_mgmt_op action = ptp_mgmt_action(msg);
	struct ptp_port_id *target_port = &msg->management.target_port_id;
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	struct ptp_tlv_mgmt *mgmt = (struct ptp_tlv_mgmt *)msg->management.suffix;
	struct ptp_port *iter;

	if (!ptp_clock_id_eq(&dds->clk_id, &target_port->clk_id) &&
	    !ptp_clock_id_eq(&target_port->clk_id, &all_ones)) {
		return state_decision_required;
	}

	if (sys_slist_len(&msg->tlvs) != 1) {
		/* IEEE 1588-2019 15.3.2 - PTP mgmt msg transports single mgmt TLV */
		return state_decision_required;
	}

	clock_forward_management_msg(port, msg);

	switch (action) {
	case PTP_MGMT_SET:
		ret = clock_management_set(port, msg, mgmt);
		if (ret < 0) {
			return state_decision_required;
		}
		state_decision_required = ret ? true : false;
		break;
	case PTP_MGMT_GET:
		__fallthrough;
	case PTP_MGMT_CMD:
		break;
	default:
		return state_decision_required;
	}

	switch (mgmt->id) {
	case PTP_MGMT_CLOCK_DESCRIPTION:
		__fallthrough;
	case PTP_MGMT_USER_DESCRIPTION:
		__fallthrough;
	case PTP_MGMT_SAVE_IN_NON_VOLATILE_STORAGE:
		__fallthrough;
	case PTP_MGMT_RESET_NON_VOLATILE_STORAGE:
		__fallthrough;
	case PTP_MGMT_INITIALIZE:
		__fallthrough;
	case PTP_MGMT_FAULT_LOG:
		__fallthrough;
	case PTP_MGMT_FAULT_LOG_RESET:
		__fallthrough;
	case PTP_MGMT_DOMAIN:
		__fallthrough;
	case PTP_MGMT_TIME_RECEIVER_ONLY:
		__fallthrough;
	case PTP_MGMT_ANNOUNCE_RECEIPT_TIMEOUT:
		__fallthrough;
	case PTP_MGMT_VERSION_NUMBER:
		__fallthrough;
	case PTP_MGMT_ENABLE_PORT:
		__fallthrough;
	case PTP_MGMT_DISABLE_PORT:
		__fallthrough;
	case PTP_MGMT_TIME:
		__fallthrough;
	case PTP_MGMT_CLOCK_ACCURACY:
		__fallthrough;
	case PTP_MGMT_UTC_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_TRACEBILITY_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_TIMESCALE_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_UNICAST_NEGOTIATION_ENABLE:
		__fallthrough;
	case PTP_MGMT_PATH_TRACE_LIST:
		__fallthrough;
	case PTP_MGMT_PATH_TRACE_ENABLE:
		__fallthrough;
	case PTP_MGMT_GRANDMASTER_CLUSTER_TABLE:
		__fallthrough;
	case PTP_MGMT_UNICAST_TIME_TRANSMITTER_TABLE:
		__fallthrough;
	case PTP_MGMT_UNICAST_TIME_TRANSMITTER_MAX_TABLE_SIZE:
		__fallthrough;
	case PTP_MGMT_ACCEPTABLE_TIME_TRANSMITTER_TABLE:
		__fallthrough;
	case PTP_MGMT_ACCEPTABLE_TIME_TRANSMITTER_TABLE_ENABLED:
		__fallthrough;
	case PTP_MGMT_ACCEPTABLE_TIME_TRANSMITTER_MAX_TABLE_SIZE:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_TRANSMITTER:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_ENABLE:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_NAME:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_MAX_KEY:
		__fallthrough;
	case PTP_MGMT_ALTERNATE_TIME_OFFSET_PROPERTIES:
		__fallthrough;
	case PTP_MGMT_EXTERNAL_PORT_CONFIGURATION_ENABLED:
		__fallthrough;
	case PTP_MGMT_TIME_TRANSMITTER_ONLY:
		__fallthrough;
	case PTP_MGMT_HOLDOVER_UPGRADE_ENABLE:
		__fallthrough;
	case PTP_MGMT_EXT_PORT_CONFIG_PORT_DATA_SET:
		__fallthrough;
	case PTP_MGMT_TRANSPARENT_CLOCK_DEFAULT_DATA_SET:
		__fallthrough;
	case PTP_MGMT_TRANSPARENT_CLOCK_PORT_DATA_SET:
		__fallthrough;
	case PTP_MGMT_PRIMARY_DOMAIN:
		__fallthrough;
	case PTP_MGMT_DELAY_MECHANISM:
		__fallthrough;
	case PTP_MGMT_LOG_MIN_PDELAY_REQ_INTERVAL:
		ptp_port_management_error(port, msg, PTP_MGMT_ERR_NOT_SUPPORTED);
		break;
	default:
		if (target_port->port_number == port->port_ds.id.port_number) {
			ptp_port_management_msg_process(port, port, msg, mgmt);
		} else if (target_port->port_number == UINT16_MAX) {
			SYS_SLIST_FOR_EACH_CONTAINER(&clock.ports_list, iter, node) {
				if (ptp_port_management_msg_process(iter, port, msg, mgmt)) {
					break;
				}
			}
		}
		break;
	}

	return state_decision_required;
}

void ptp_clock_synchronize(uint64_t ingress, uint64_t egress)
{
	int64_t offset;
	uint64_t delay = clock.current_ds.mean_delay >> 16;

	clock.timestamp.t1 = egress;
	clock.timestamp.t2 = ingress;

	if (!clock.current_ds.mean_delay) {
		return;
	}

	offset = clock.timestamp.t2 - clock.timestamp.t1 - delay;

	/* If diff is too big, clock needs to be set first. */
	if (offset > NSEC_PER_SEC || offset < -NSEC_PER_SEC) {
		struct net_ptp_time current;

		LOG_WRN("Clock offset exceeds 1 second.");

		ptp_clock_get(clock.phc, &current);

		current.second -= (uint64_t)(offset / NSEC_PER_SEC);
		current.nanosecond -= (uint32_t)(offset % NSEC_PER_SEC);

		ptp_clock_set(clock.phc, &current);
		return;
	}

	LOG_DBG("Offset %lldns", offset);
	clock.current_ds.offset_from_tt = clock_ns_to_timeinterval(offset);

	ptp_clock_adjust(clock.phc, offset);
}

void ptp_clock_delay(uint64_t egress, uint64_t ingress)
{
	int64_t delay;

	clock.timestamp.t3 = egress;
	clock.timestamp.t4 = ingress;

	delay = ((clock.timestamp.t2 - clock.timestamp.t3) +
		 (clock.timestamp.t4 - clock.timestamp.t1)) / 2;

	LOG_DBG("Delay %lldns", delay);
	clock.current_ds.mean_delay = clock_ns_to_timeinterval(delay);
}

enum ptp_clock_type ptp_clock_type(void)
{
	return (enum ptp_clock_type)clock.default_ds.type;
}

const struct ptp_default_ds *ptp_clock_default_ds(void)
{
	return &clock.default_ds;
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &clock.parent_ds;
}

const struct ptp_current_ds *ptp_clock_current_ds(void)
{
	return &clock.current_ds;
}

const struct ptp_time_prop_ds *ptp_clock_time_prop_ds(void)
{
	return &clock.time_prop_ds;
}

const struct ptp_dataset *ptp_clock_ds(void)
{
	struct ptp_dataset *ds = &clock.dataset;

	ds->priority1		 = clock.default_ds.priority1;
	ds->clk_quality		 = clock.default_ds.clk_quality;
	ds->priority2		 = clock.default_ds.priority2;
	ds->steps_rm		 = 0;
	ds->sender.port_number	 = 0;
	ds->receiver.port_number = 0;
	memcpy(&ds->clk_id, &clock.default_ds.clk_id, sizeof(ptp_clk_id));
	memcpy(&ds->sender.clk_id, &clock.default_ds.clk_id, sizeof(ptp_clk_id));
	memcpy(&ds->receiver.clk_id, &clock.default_ds.clk_id, sizeof(ptp_clk_id));
	return ds;
}

const struct ptp_dataset *ptp_clock_best_foreign_ds(void)
{
	return clock.best ? &clock.best->dataset : NULL;
}

void ptp_clock_pollfd_invalidate(void)
{
	clock.pollfd_valid = false;
}

void ptp_clock_port_add(struct ptp_port *port)
{
	clock.default_ds.n_ports++;
	sys_slist_append(&clock.ports_list, &port->node);
}

const struct ptp_foreign_tt_clock *ptp_clock_best_time_transmitter(void)
{
	return clock.best;
}

bool ptp_clock_id_eq(const ptp_clk_id *c1, const ptp_clk_id *c2)
{
	return memcmp(c1, c2, sizeof(ptp_clk_id)) == 0;
}

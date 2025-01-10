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

#include <zephyr/posix/sys/eventfd.h>

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
	struct zsock_pollfd	    pollfd[1 + 2 * CONFIG_PTP_NUM_PORTS];
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

__maybe_unused static struct ptp_clock ptp_clk = { 0 };
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

		SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, iter, node) {
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
		ptp_clk.default_ds.priority1 = *tlv->data;
		send_resp = true;
		break;
	case PTP_MGMT_PRIORITY2:
		ptp_clk.default_ds.priority2 = *tlv->data;
		send_resp = true;
		break;
	default:
		break;
	}

	return send_resp ? ptp_port_management_resp(port, req, tlv) : 0;
}

static void clock_update_grandmaster(void)
{
	memset(&ptp_clk.current_ds, 0, sizeof(struct ptp_current_ds));

	memcpy(&ptp_clk.parent_ds.port_id.clk_id,
	       &ptp_clk.default_ds.clk_id,
	       sizeof(ptp_clk_id));
	memcpy(&ptp_clk.parent_ds.gm_id,
	       &ptp_clk.default_ds.clk_id,
	       sizeof(ptp_clk_id));
	ptp_clk.parent_ds.port_id.port_number = 0;
	ptp_clk.parent_ds.gm_clk_quality = ptp_clk.default_ds.clk_quality;
	ptp_clk.parent_ds.gm_priority1 = ptp_clk.default_ds.priority1;
	ptp_clk.parent_ds.gm_priority2 = ptp_clk.default_ds.priority2;

	ptp_clk.time_prop_ds.current_utc_offset = 37; /* IEEE 1588-2019 9.4 */
	ptp_clk.time_prop_ds.time_src = ptp_clk.time_src;
	ptp_clk.time_prop_ds.flags = 0;
}

static void clock_update_time_receiver(void)
{
	struct ptp_msg *best_msg = (struct ptp_msg *)k_fifo_peek_tail(&ptp_clk.best->messages);

	ptp_clk.current_ds.steps_rm = 1 + ptp_clk.best->dataset.steps_rm;

	memcpy(&ptp_clk.parent_ds.gm_id,
	       &best_msg->announce.gm_id,
	       sizeof(best_msg->announce.gm_id));
	memcpy(&ptp_clk.parent_ds.port_id,
	       &ptp_clk.best->dataset.sender,
	       sizeof(ptp_clk.best->dataset.sender));
	ptp_clk.parent_ds.gm_clk_quality = best_msg->announce.gm_clk_quality;
	ptp_clk.parent_ds.gm_priority1 = best_msg->announce.gm_priority1;
	ptp_clk.parent_ds.gm_priority2 = best_msg->announce.gm_priority2;

	ptp_clk.time_prop_ds.current_utc_offset = best_msg->announce.current_utc_offset;
	ptp_clk.time_prop_ds.flags = best_msg->header.flags[1];
}

static void clock_check_pollfd(void)
{
	struct ptp_port *port;
	struct zsock_pollfd *fd = &ptp_clk.pollfd[1];

	if (ptp_clk.pollfd_valid) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		for (int i = 0; i < PTP_SOCKET_CNT; i++) {
			fd->fd = port->socket[i];
			fd->events = ZSOCK_POLLIN | ZSOCK_POLLPRI;
			fd++;
		}
	}

	ptp_clk.pollfd_valid = true;
}

const struct ptp_clock *ptp_clock_init(void)
{
	struct ptp_default_ds *dds = &ptp_clk.default_ds;
	struct ptp_parent_ds *pds  = &ptp_clk.parent_ds;
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	ptp_clk.time_src = (enum ptp_time_src)PTP_TIME_SRC_INTERNAL_OSC;

	/* Initialize Default Dataset. */
	int ret = clock_generate_id(&dds->clk_id, iface);

	if (ret) {
		LOG_ERR("Couldn't assign Clock Identity.");
		return NULL;
	}

	dds->type = (enum ptp_clock_type)CONFIG_PTP_CLOCK_TYPE;
	dds->n_ports = 0;
	dds->time_receiver_only = IS_ENABLED(CONFIG_PTP_TIME_RECEIVER_ONLY) ? true : false;

	dds->clk_quality.cls = dds->time_receiver_only ? 255 : 248;
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

	ptp_clk.phc = net_eth_get_ptp_clock(iface);
	if (!ptp_clk.phc) {
		LOG_ERR("Couldn't get PTP HW Clock for the interface.");
		return NULL;
	}

	ptp_clk.pollfd[0].fd = eventfd(0, EFD_NONBLOCK);
	ptp_clk.pollfd[0].events = ZSOCK_POLLIN;

	sys_slist_init(&ptp_clk.ports_list);
	LOG_DBG("PTP Clock %s initialized", clock_id_str(&dds->clk_id));
	return &ptp_clk;
}

struct zsock_pollfd *ptp_clock_poll_sockets(void)
{
	int ret;

	clock_check_pollfd();
	ret = zsock_poll(ptp_clk.pollfd, PTP_SOCKET_CNT * ptp_clk.default_ds.n_ports + 1, -1);
	if (ret > 0 && ptp_clk.pollfd[0].revents) {
		eventfd_t value;

		eventfd_read(ptp_clk.pollfd[0].fd, &value);
	}

	return &ptp_clk.pollfd[1];
}

void ptp_clock_handle_state_decision_evt(void)
{
	struct ptp_foreign_tt_clock *best = NULL, *foreign;
	struct ptp_port *port;
	bool tt_changed = false;

	if (!ptp_clk.state_decision_event) {
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		foreign = ptp_port_best_foreign(port);
		if (!foreign) {
			continue;
		}
		if (!best || ptp_btca_ds_cmp(&foreign->dataset, &best->dataset)) {
			best = foreign;
		}
	}

	ptp_clk.best = best;

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		enum ptp_port_state state;
		enum ptp_port_event event;

		state = ptp_btca_state_decision(port);

		switch (state) {
		case PTP_PS_LISTENING:
			event = PTP_EVT_NONE;
			break;
		case PTP_PS_GRAND_MASTER:
			clock_update_grandmaster();
			event = PTP_EVT_RS_GRAND_MASTER;
			break;
		case PTP_PS_TIME_TRANSMITTER:
			event = PTP_EVT_RS_TIME_TRANSMITTER;
			break;
		case PTP_PS_TIME_RECEIVER:
			clock_update_time_receiver();
			event = PTP_EVT_RS_TIME_RECEIVER;
			break;
		case PTP_PS_PASSIVE:
			event = PTP_EVT_RS_PASSIVE;
			break;
		default:
			event = PTP_EVT_FAULT_DETECTED;
			break;
		}

		ptp_port_event_handle(port, event, tt_changed);
	}

	ptp_clk.state_decision_event = false;
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
			SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, iter, node) {
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
	int64_t delay = ptp_clk.current_ds.mean_delay >> 16;

	ptp_clk.timestamp.t1 = egress;
	ptp_clk.timestamp.t2 = ingress;

	if (!ptp_clk.current_ds.mean_delay) {
		return;
	}

	offset = (int64_t)(ptp_clk.timestamp.t2 - ptp_clk.timestamp.t1) - delay;

	/* If diff is too big, ptp_clk needs to be set first. */
	if ((offset > (int64_t)NSEC_PER_SEC) || (offset < -(int64_t)NSEC_PER_SEC)) {
		struct net_ptp_time current;
		int32_t dest_nsec;

		LOG_WRN("Clock offset exceeds 1 second.");

		ptp_clock_get(ptp_clk.phc, &current);

		current.second = (uint64_t)(current.second - (offset / NSEC_PER_SEC));
		dest_nsec = (int32_t)(current.nanosecond - (offset % NSEC_PER_SEC));

		if (dest_nsec < 0) {
			current.second--;
			dest_nsec += NSEC_PER_SEC;
		} else if (dest_nsec >= NSEC_PER_SEC) {
			current.second++;
			dest_nsec -= NSEC_PER_SEC;
		}

		current.nanosecond = (uint32_t)dest_nsec;

		ptp_clock_set(ptp_clk.phc, &current);
		return;
	}

	LOG_DBG("Offset %lldns", offset);
	ptp_clk.current_ds.offset_from_tt = clock_ns_to_timeinterval(offset);

	ptp_clock_adjust(ptp_clk.phc, -offset);
}

void ptp_clock_delay(uint64_t egress, uint64_t ingress)
{
	int64_t delay;

	ptp_clk.timestamp.t3 = egress;
	ptp_clk.timestamp.t4 = ingress;

	delay = ((int64_t)(ptp_clk.timestamp.t2 - ptp_clk.timestamp.t3) +
		 (int64_t)(ptp_clk.timestamp.t4 - ptp_clk.timestamp.t1)) /
		2LL;

	LOG_DBG("Delay %lldns", delay);
	ptp_clk.current_ds.mean_delay = clock_ns_to_timeinterval(delay);
}

sys_slist_t *ptp_clock_ports_list(void)
{
	return &ptp_clk.ports_list;
}

enum ptp_clock_type ptp_clock_type(void)
{
	return (enum ptp_clock_type)ptp_clk.default_ds.type;
}

const struct ptp_default_ds *ptp_clock_default_ds(void)
{
	return &ptp_clk.default_ds;
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &ptp_clk.parent_ds;
}

const struct ptp_current_ds *ptp_clock_current_ds(void)
{
	return &ptp_clk.current_ds;
}

const struct ptp_time_prop_ds *ptp_clock_time_prop_ds(void)
{
	return &ptp_clk.time_prop_ds;
}

const struct ptp_dataset *ptp_clock_ds(void)
{
	struct ptp_dataset *ds = &ptp_clk.dataset;

	ds->priority1		 = ptp_clk.default_ds.priority1;
	ds->clk_quality		 = ptp_clk.default_ds.clk_quality;
	ds->priority2		 = ptp_clk.default_ds.priority2;
	ds->steps_rm		 = 0;
	ds->sender.port_number	 = 0;
	ds->receiver.port_number = 0;
	memcpy(&ds->clk_id, &ptp_clk.default_ds.clk_id, sizeof(ptp_clk_id));
	memcpy(&ds->sender.clk_id, &ptp_clk.default_ds.clk_id, sizeof(ptp_clk_id));
	memcpy(&ds->receiver.clk_id, &ptp_clk.default_ds.clk_id, sizeof(ptp_clk_id));
	return ds;
}

const struct ptp_dataset *ptp_clock_best_foreign_ds(void)
{
	return ptp_clk.best ? &ptp_clk.best->dataset : NULL;
}

struct ptp_port *ptp_clock_port_from_iface(struct net_if *iface)
{
	struct ptp_port *port;

	SYS_SLIST_FOR_EACH_CONTAINER(&ptp_clk.ports_list, port, node) {
		if (port->iface == iface) {
			return port;
		}
	}

	return NULL;
}

void ptp_clock_pollfd_invalidate(void)
{
	ptp_clk.pollfd_valid = false;
}

void ptp_clock_signal_timeout(void)
{
	eventfd_write(ptp_clk.pollfd[0].fd, 1);
}

void ptp_clock_state_decision_req(void)
{
	ptp_clk.state_decision_event = true;
}

void ptp_clock_port_add(struct ptp_port *port)
{
	ptp_clk.default_ds.n_ports++;
	sys_slist_append(&ptp_clk.ports_list, &port->node);
}

const struct ptp_foreign_tt_clock *ptp_clock_best_time_transmitter(void)
{
	return ptp_clk.best;
}

bool ptp_clock_id_eq(const ptp_clk_id *c1, const ptp_clk_id *c2)
{
	return memcmp(c1, c2, sizeof(ptp_clk_id)) == 0;
}

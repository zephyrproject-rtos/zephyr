/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_port, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ptp.h>
#include <zephyr/net/ptp_time.h>
#include <zephyr/random/random.h>

#include "btca.h"
#include "clock.h"
#include "port.h"
#include "msg.h"
#include "tlv.h"
#include "transport.h"

#define DEFAULT_LOG_MSG_INTERVAL (0x7F)

#define PORT_DELAY_REQ_CLEARE_TO (3 * NSEC_PER_SEC)

#define PORT_LINK_UP	     BIT(0)
#define PORT_LINK_DOWN	     BIT(1)
#define PORT_LINK_CHANGED    BIT(2)
#define PORT_LINK_EVENT_MASK (NET_EVENT_IF_DOWN | NET_EVENT_IF_UP)

static struct ptp_port ports[CONFIG_PTP_NUM_PORTS];
static struct k_mem_slab foreign_tts_slab;
#if CONFIG_PTP_FOREIGN_TIME_TRANSMITTER_FEATURE
BUILD_ASSERT(CONFIG_PTP_FOREIGN_TIME_TRANSMITTER_RECORD_SIZE >= 5 * CONFIG_PTP_NUM_PORTS,
	     "PTP_FOREIGN_TIME_TRANSMITTER_RECORD_SIZE is smaller than expected!");

K_MEM_SLAB_DEFINE_STATIC(foreign_tts_slab,
			 sizeof(struct ptp_foreign_tt_clock),
			 CONFIG_PTP_FOREIGN_TIME_TRANSMITTER_RECORD_SIZE,
			 8);
#endif

char str_port_id[] = "FF:FF:FF:FF:FF:FF:FF:FF-FFFF";

const char *port_id_str(struct ptp_port_id *port_id)
{
	uint8_t *pid = port_id->clk_id.id;

	snprintk(str_port_id, sizeof(str_port_id), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X-%04X",
		 pid[0],
		 pid[1],
		 pid[2],
		 pid[3],
		 pid[4],
		 pid[5],
		 pid[6],
		 pid[7],
		 port_id->port_number);

	return str_port_id;
}

static const char *port_state_str(enum ptp_port_state state)
{
	const static char * const states[] = {
		[PTP_PS_INITIALIZING]	      = "INITIALIZING",
		[PTP_PS_FAULTY]		      = "FAULTY",
		[PTP_PS_DISABLED]	      = "DISABLED",
		[PTP_PS_LISTENING]	      = "LISTENING",
		[PTP_PS_PRE_TIME_TRANSMITTER] = "PRE TIME TRANSMITTER",
		[PTP_PS_TIME_TRANSMITTER]     = "TIME TRANSMITTER",
		[PTP_PS_GRAND_MASTER]	      = "GRAND MASTER",
		[PTP_PS_PASSIVE]	      = "PASSIVE",
		[PTP_PS_UNCALIBRATED]	      = "UNCALIBRATED",
		[PTP_PS_TIME_RECEIVER]	      = "TIME RECEIVER",
	};

	return states[state];
}

static int port_msg_send(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	ptp_msg_pre_send(msg);

	return ptp_transport_send(port, msg, idx);
}

static void port_timer_set_timeout(struct k_timer *timer, uint8_t factor, int8_t log_seconds)
{
	uint64_t timeout = log_seconds < 0 ?
		(uint64_t)((NSEC_PER_SEC * factor) >> (log_seconds * -1)) :
		(uint64_t)((NSEC_PER_SEC * factor) << log_seconds);

	k_timer_start(timer, K_NSEC(timeout), K_NO_WAIT);
}

static void port_timer_set_timeout_random(struct k_timer *timer,
					  int min_factor,
					  int span,
					  int log_seconds)
{
	uint64_t timeout, random_ns;

	if (log_seconds < 0) {
		timeout = (uint64_t)((NSEC_PER_SEC * min_factor) >> -log_seconds);
		random_ns = (uint64_t)(NSEC_PER_SEC >> -log_seconds);
	} else {
		timeout = (uint64_t)((NSEC_PER_SEC * min_factor) << log_seconds);
		random_ns = (uint64_t)((span * NSEC_PER_SEC) << log_seconds);
	}

	timeout = (uint64_t)(timeout + (random_ns * (sys_rand32_get() % (1 << 15) + 1) >> 15));
	k_timer_start(timer, K_NSEC(timeout), K_NO_WAIT);
}

static void port_synchronize(struct ptp_port *port,
			     struct net_ptp_time ingress_ts,
			     struct net_ptp_time origin_ts,
			     ptp_timeinterval correction1,
			     ptp_timeinterval correction2)
{
	uint64_t t1, t2, t1c;

	t1 = origin_ts.second * NSEC_PER_SEC + origin_ts.nanosecond;
	t2 = ingress_ts.second * NSEC_PER_SEC + ingress_ts.nanosecond;
	t1c = t1 + (correction1 >> 16) + (correction2 >> 16);

	ptp_clock_synchronize(t2, t1c);

	port_timer_set_timeout(&port->timers.sync,
			       port->port_ds.announce_receipt_timeout,
			       port->port_ds.log_sync_interval);
}

static void port_ds_init(struct ptp_port *port)
{
	struct ptp_port_ds *ds = &port->port_ds;
	const struct ptp_default_ds *dds = ptp_clock_default_ds();

	memcpy(&ds->id.clk_id, &dds->clk_id, sizeof(ptp_clk_id));
	ds->id.port_number = dds->n_ports + 1;

	ds->state			= PTP_PS_INITIALIZING;
	ds->log_min_delay_req_interval	= CONFIG_PTP_MIN_DELAY_REQ_LOG_INTERVAL;
	ds->log_announce_interval	= CONFIG_PTP_ANNOUNCE_LOG_INTERVAL;
	ds->announce_receipt_timeout	= CONFIG_PTP_ANNOUNCE_RECV_TIMEOUT;
	ds->log_sync_interval		= CONFIG_PTP_SYNC_LOG_INTERVAL;
	ds->delay_mechanism		= PTP_DM_E2E;
	ds->log_min_pdelay_req_interval = CONFIG_PTP_MIN_PDELAY_REQ_LOG_INTERVAL;
	ds->version			= PTP_VERSION;
	ds->delay_asymmetry		= 0;
}

static void port_delay_req_timestamp_cb(struct net_pkt *pkt)
{
	struct ptp_port *port = ptp_clock_port_from_iface(pkt->iface);
	struct ptp_msg *req, *msg = ptp_msg_from_pkt(pkt);
	sys_snode_t *iter, *last = NULL;

	if (!port || !msg) {
		return;
	}

	msg->header.src_port_id.port_number = ntohs(msg->header.src_port_id.port_number);

	if (!ptp_port_id_eq(&port->port_ds.id, &msg->header.src_port_id) ||
	    ptp_msg_type(msg) != PTP_MSG_DELAY_REQ) {
		return;
	}

	for (iter = sys_slist_peek_head(&port->delay_req_list);
	     iter;
	     iter = sys_slist_peek_next(iter), last = iter) {

		req =  CONTAINER_OF(iter, struct ptp_msg, node);

		if (req->header.sequence_id != msg->header.sequence_id) {
			continue;
		}

		if (pkt->timestamp.second == UINT64_MAX ||
		    (pkt->timestamp.second == 0 && pkt->timestamp.nanosecond == 0)) {
			net_if_unregister_timestamp_cb(&port->delay_req_ts_cb);
			sys_slist_remove(&port->delay_req_list, last, iter);
			ptp_msg_unref(req);
			return;
		}

		req->timestamp.host._sec.high = pkt->timestamp._sec.high;
		req->timestamp.host._sec.low = pkt->timestamp._sec.low;
		req->timestamp.host.nanosecond = pkt->timestamp.nanosecond;

		LOG_DBG("Port %d registered timestamp for %d Delay_Req",
			port->port_ds.id.port_number,
			ntohs(msg->header.sequence_id));

		if (iter == sys_slist_peek_tail(&port->delay_req_list)) {
			net_if_unregister_timestamp_cb(&port->delay_req_ts_cb);
		}
	}
}

static void port_sync_timestamp_cb(struct net_pkt *pkt)
{
	struct ptp_port *port = ptp_clock_port_from_iface(pkt->iface);
	struct ptp_msg *msg = ptp_msg_from_pkt(pkt);

	if (!port || !msg) {
		return;
	}

	msg->header.src_port_id.port_number = ntohs(msg->header.src_port_id.port_number);

	if (ptp_port_id_eq(&port->port_ds.id, &msg->header.src_port_id) &&
	    ptp_msg_type(msg) == PTP_MSG_SYNC) {

		const struct ptp_default_ds *dds = ptp_clock_default_ds();
		const struct ptp_time_prop_ds *tpds = ptp_clock_time_prop_ds();
		struct ptp_msg *resp = ptp_msg_alloc();

		if (!resp) {
			return;
		}

		resp->header.type_major_sdo_id = PTP_MSG_FOLLOW_UP;
		resp->header.version	       = PTP_VERSION;
		resp->header.msg_length	       = sizeof(struct ptp_follow_up_msg);
		resp->header.domain_number     = dds->domain;
		resp->header.flags[1]	       = tpds->flags;
		resp->header.src_port_id       = port->port_ds.id;
		resp->header.sequence_id       = port->seq_id.sync++;
		resp->header.log_msg_interval  = port->port_ds.log_sync_interval;

		resp->follow_up.precise_origin_timestamp.seconds_high = pkt->timestamp._sec.high;
		resp->follow_up.precise_origin_timestamp.seconds_low = pkt->timestamp._sec.low;
		resp->follow_up.precise_origin_timestamp.nanoseconds = pkt->timestamp.nanosecond;

		net_if_unregister_timestamp_cb(&port->sync_ts_cb);

		port_msg_send(port, resp, PTP_SOCKET_GENERAL);
		ptp_msg_unref(resp);

		LOG_DBG("Port %d sends Follow_Up message", port->port_ds.id.port_number);
	}
}

static int port_announce_msg_transmit(struct ptp_port *port)
{
	const struct ptp_parent_ds *pds = ptp_clock_parent_ds();
	const struct ptp_current_ds *cds = ptp_clock_current_ds();
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	const struct ptp_time_prop_ds *tpds = ptp_clock_time_prop_ds();
	struct ptp_msg *msg = ptp_msg_alloc();
	int ret;

	if (!msg) {
		return -ENOMEM;
	}

	msg->header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg->header.version	      = PTP_VERSION;
	msg->header.msg_length	      = sizeof(struct ptp_announce_msg);
	msg->header.domain_number     = dds->domain;
	msg->header.flags[1]	      = tpds->flags;
	msg->header.src_port_id	      = port->port_ds.id;
	msg->header.sequence_id	      = port->seq_id.announce++;
	msg->header.log_msg_interval  = port->port_ds.log_sync_interval;

	msg->announce.current_utc_offset = tpds->current_utc_offset;
	msg->announce.gm_priority1	 = pds->gm_priority1;
	msg->announce.gm_clk_quality	 = pds->gm_clk_quality;
	msg->announce.gm_priority2	 = pds->gm_priority2;
	msg->announce.gm_id		 = pds->gm_id;
	msg->announce.steps_rm		 = cds->steps_rm;
	msg->announce.time_src		 = tpds->time_src;

	ret = port_msg_send(port, msg, PTP_SOCKET_GENERAL);
	ptp_msg_unref(msg);

	if (ret < 0) {
		return -EFAULT;
	}

	LOG_DBG("Port %d sends Announce message", port->port_ds.id.port_number);
	return 0;
}

static int port_delay_req_msg_transmit(struct ptp_port *port)
{
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	struct ptp_msg *msg = ptp_msg_alloc();
	int ret;

	if (!msg) {
		return -ENOMEM;
	}

	msg->header.type_major_sdo_id = PTP_MSG_DELAY_REQ;
	msg->header.version	      = PTP_VERSION;
	msg->header.msg_length	      = sizeof(struct ptp_delay_req_msg);
	msg->header.domain_number     = dds->domain;
	msg->header.src_port_id	      = port->port_ds.id;
	msg->header.sequence_id	      = port->seq_id.delay++;
	msg->header.log_msg_interval  = DEFAULT_LOG_MSG_INTERVAL;

	net_if_register_timestamp_cb(&port->delay_req_ts_cb,
				     NULL,
				     port->iface,
				     port_delay_req_timestamp_cb);

	ret = port_msg_send(port, msg, PTP_SOCKET_EVENT);
	if (ret < 0) {
		ptp_msg_unref(msg);
		return -EFAULT;
	}

	sys_slist_append(&port->delay_req_list, &msg->node);

	LOG_DBG("Port %d sends Delay_Req message", port->port_ds.id.port_number);
	return 0;
}

static int port_sync_msg_transmit(struct ptp_port *port)
{
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	const struct ptp_time_prop_ds *tpds = ptp_clock_time_prop_ds();
	struct ptp_msg *msg = ptp_msg_alloc();
	int ret;

	if (!msg) {
		return -ENOMEM;
	}

	msg->header.type_major_sdo_id = PTP_MSG_SYNC;
	msg->header.version	      = PTP_VERSION;
	msg->header.msg_length	      = sizeof(struct ptp_sync_msg);
	msg->header.domain_number     = dds->domain;
	msg->header.flags[0]	      = PTP_MSG_TWO_STEP_FLAG;
	msg->header.flags[1]	      = tpds->flags;
	msg->header.src_port_id	      = port->port_ds.id;
	msg->header.sequence_id	      = port->seq_id.sync;
	msg->header.log_msg_interval  = port->port_ds.log_sync_interval;

	net_if_register_timestamp_cb(&port->sync_ts_cb,
				     NULL,
				     port->iface,
				     port_sync_timestamp_cb);

	ret = port_msg_send(port, msg, PTP_SOCKET_EVENT);
	ptp_msg_unref(msg);

	if (ret < 0) {
		return -EFAULT;
	}
	LOG_DBG("Port %d sends Sync message", port->port_ds.id.port_number);
	return 0;
}

static void port_timer_init(struct k_timer *timer, k_timer_expiry_t timeout_fn, void *user_data)
{
	k_timer_init(timer, timeout_fn, NULL);
	k_timer_user_data_set(timer, user_data);
}

static void port_timer_to_handler(struct k_timer *timer)
{
	struct ptp_port *port = (struct ptp_port *)k_timer_user_data_get(timer);

	if (timer == &port->timers.announce) {
		atomic_set_bit(&port->timeouts, PTP_PORT_TIMER_ANNOUNCE_TO);
	} else if (timer == &port->timers.sync) {
		atomic_set_bit(&port->timeouts, PTP_PORT_TIMER_SYNC_TO);
	} else if (timer == &port->timers.delay) {
		atomic_set_bit(&port->timeouts, PTP_PORT_TIMER_DELAY_TO);
	} else if (timer == &port->timers.qualification) {
		atomic_set_bit(&port->timeouts, PTP_PORT_TIMER_QUALIFICATION_TO);
	}

	ptp_clock_signal_timeout();
}

static void foreign_clock_cleanup(struct ptp_foreign_tt_clock *foreign)
{
	struct ptp_msg *msg;
	int64_t timestamp, timeout, current = k_uptime_get() * NSEC_PER_MSEC;

	while (foreign->messages_count > FOREIGN_TIME_TRANSMITTER_THRESHOLD) {
		msg = (struct ptp_msg *)k_fifo_get(&foreign->messages, K_NO_WAIT);
		ptp_msg_unref(msg);
		foreign->messages_count--;
	}

	/* Remove messages that don't arrived at
	 * FOREIGN_TIME_TRANSMITTER_TIME_WINDOW (4 * announce interval) - IEEE 1588-2019 9.3.2.4.5
	 */
	while (!k_fifo_is_empty(&foreign->messages)) {
		msg = (struct ptp_msg *)k_fifo_peek_head(&foreign->messages);

		if (msg->header.log_msg_interval <= -31) {
			timeout = 0;
		} else if (msg->header.log_msg_interval >= 31) {
			timeout = INT64_MAX;
		} else if (msg->header.log_msg_interval > 0) {
			timeout = FOREIGN_TIME_TRANSMITTER_TIME_WINDOW_MUL *
				  (1 << msg->header.log_msg_interval) * NSEC_PER_SEC;
		} else {
			timeout = FOREIGN_TIME_TRANSMITTER_TIME_WINDOW_MUL * NSEC_PER_SEC /
				  (1 << (-msg->header.log_msg_interval));
		}

		timestamp = msg->timestamp.host.second * NSEC_PER_SEC +
			    msg->timestamp.host.nanosecond;

		if (current - timestamp < timeout) {
			/* Remaining messages are within time window */
			break;
		}

		msg = (struct ptp_msg *)k_fifo_get(&foreign->messages, K_NO_WAIT);
		ptp_msg_unref(msg);
		foreign->messages_count--;
	}
}

static void port_clear_foreign_clock_records(struct ptp_foreign_tt_clock *foreign)
{
	struct ptp_msg *msg;

	while (!k_fifo_is_empty(&foreign->messages)) {
		msg = (struct ptp_msg *)k_fifo_get(&foreign->messages, K_NO_WAIT);
		ptp_msg_unref(msg);
		foreign->messages_count--;
	}
}

static void port_delay_req_cleanup(struct ptp_port *port)
{
	sys_snode_t *prev = NULL;
	struct ptp_msg *msg;
	int64_t timestamp, current = k_uptime_get() * NSEC_PER_MSEC;

	SYS_SLIST_FOR_EACH_CONTAINER(&port->delay_req_list, msg, node) {
		timestamp = msg->timestamp.host.second * NSEC_PER_SEC +
			    msg->timestamp.host.nanosecond;

		if (current - timestamp < PORT_DELAY_REQ_CLEARE_TO) {
			break;
		}

		ptp_msg_unref(msg);
		sys_slist_remove(&port->delay_req_list, prev, &msg->node);
		prev = &msg->node;
	}
}

static void port_clear_delay_req(struct ptp_port *port)
{
	sys_snode_t *prev = NULL;
	struct ptp_msg *msg;

	SYS_SLIST_FOR_EACH_CONTAINER(&port->delay_req_list, msg, node) {
		ptp_msg_unref(msg);
		sys_slist_remove(&port->delay_req_list, prev, &msg->node);
		prev = &msg->node;
	}
}

static void port_sync_fup_ooo_handle(struct ptp_port *port, struct ptp_msg *msg)
{
	struct ptp_msg *last = port->last_sync_fup;

	if (ptp_msg_type(msg) != PTP_MSG_FOLLOW_UP &&
	    ptp_msg_type(msg) != PTP_MSG_SYNC) {
		return;
	}

	if (!last) {
		port->last_sync_fup = msg;
		ptp_msg_ref(msg);
		return;
	}

	if (ptp_msg_type(last) == PTP_MSG_SYNC &&
	    ptp_msg_type(msg) == PTP_MSG_FOLLOW_UP &&
	    msg->header.sequence_id == last->header.sequence_id) {

		port_synchronize(port,
				 last->timestamp.host,
				 msg->timestamp.protocol,
				 last->header.correction,
				 msg->header.correction);

		ptp_msg_unref(port->last_sync_fup);
		port->last_sync_fup = NULL;
	} else if (ptp_msg_type(last) == PTP_MSG_FOLLOW_UP &&
		   ptp_msg_type(msg) == PTP_MSG_SYNC &&
		   msg->header.sequence_id == last->header.sequence_id) {

		port_synchronize(port,
				 msg->timestamp.host,
				 last->timestamp.protocol,
				 msg->header.correction,
				 last->header.correction);

		ptp_msg_unref(port->last_sync_fup);
		port->last_sync_fup = NULL;
	} else {
		ptp_msg_unref(port->last_sync_fup);
		port->last_sync_fup = msg;
		ptp_msg_ref(msg);
	}
}

static int port_announce_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	int ret = 0;
	const struct ptp_default_ds *dds = ptp_clock_default_ds();

	if (msg->announce.steps_rm >= dds->max_steps_rm) {
		return ret;
	}

	switch (ptp_port_state(port)) {
	case PTP_PS_INITIALIZING:
		__fallthrough;
	case PTP_PS_DISABLED:
		__fallthrough;
	case PTP_PS_FAULTY:
		break;
	case PTP_PS_LISTENING:
		__fallthrough;
	case PTP_PS_PRE_TIME_TRANSMITTER:
		__fallthrough;
	case PTP_PS_TIME_TRANSMITTER:
		__fallthrough;
	case PTP_PS_GRAND_MASTER:
#if CONFIG_PTP_FOREIGN_TIME_TRANSMITTER_FEATURE
		ret = ptp_port_add_foreign_tt(port, msg);
		break;
#else
		__fallthrough;
#endif
	case PTP_PS_TIME_RECEIVER:
		__fallthrough;
	case PTP_PS_UNCALIBRATED:
		__fallthrough;
	case PTP_PS_PASSIVE:
		ret = ptp_port_update_current_time_transmitter(port, msg);
		break;
	default:
		break;
	}

	return ret;
}

static void port_sync_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	enum ptp_port_state state = ptp_port_state(port);

	if (state != PTP_PS_TIME_RECEIVER && state != PTP_PS_UNCALIBRATED) {
		return;
	}

	if (!ptp_msg_current_parent(msg)) {
		return;
	}

	if (port->port_ds.log_sync_interval != msg->header.log_msg_interval) {
		port->port_ds.log_sync_interval = msg->header.log_msg_interval;
	}

	msg->header.correction += port->port_ds.delay_asymmetry;

	if (!(msg->header.flags[0] & PTP_MSG_TWO_STEP_FLAG)) {
		port_synchronize(port,
				 msg->timestamp.host,
				 msg->timestamp.protocol,
				 msg->header.correction,
				 0);

		if (port->last_sync_fup) {
			ptp_msg_unref(port->last_sync_fup);
			port->last_sync_fup = NULL;
		}

		return;
	}

	port_sync_fup_ooo_handle(port, msg);
}

static void port_follow_up_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	enum ptp_port_state state = ptp_port_state(port);

	if (state != PTP_PS_TIME_RECEIVER && state != PTP_PS_UNCALIBRATED) {
		return;
	}

	if (!ptp_msg_current_parent(msg)) {
		return;
	}

	port_sync_fup_ooo_handle(port, msg);
}

static int port_delay_req_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	int ret;
	struct ptp_msg *resp;
	enum ptp_port_state state = ptp_port_state(port);
	const struct ptp_default_ds *dds = ptp_clock_default_ds();

	if (state != PTP_PS_TIME_TRANSMITTER && state != PTP_PS_GRAND_MASTER) {
		return 0;
	}

	resp = ptp_msg_alloc();
	if (!resp) {
		return -ENOMEM;
	}

	resp->header.type_major_sdo_id = PTP_MSG_DELAY_RESP;
	resp->header.version	       = PTP_VERSION;
	resp->header.msg_length	       = sizeof(struct ptp_delay_resp_msg);
	resp->header.domain_number     = dds->domain;
	resp->header.correction	       = msg->header.correction;
	resp->header.src_port_id       = port->port_ds.id;
	resp->header.sequence_id       = msg->header.sequence_id;
	resp->header.log_msg_interval  = port->port_ds.log_min_delay_req_interval;

	resp->delay_resp.receive_timestamp.seconds_high = msg->timestamp.host._sec.high;
	resp->delay_resp.receive_timestamp.seconds_low = msg->timestamp.host._sec.low;
	resp->delay_resp.receive_timestamp.nanoseconds = msg->timestamp.host.nanosecond;
	resp->delay_resp.req_port_id = msg->header.src_port_id;

	if (msg->header.flags[0] & PTP_MSG_UNICAST_FLAG) {
		/* TODO handle unicast messages */
		resp->header.flags[0] |= PTP_MSG_UNICAST_FLAG;
	}

	ret = port_msg_send(port, resp, PTP_SOCKET_EVENT);
	ptp_msg_unref(resp);

	if (ret < 0) {
		return -EFAULT;
	}

	LOG_DBG("Port %d responds to Delay_Req message", port->port_ds.id.port_number);
	return 0;
}

static void port_delay_resp_msg_process(struct ptp_port *port, struct ptp_msg *msg)
{
	uint64_t t3, t4, t4c;
	sys_snode_t *prev = NULL;
	struct ptp_msg *req;
	enum ptp_port_state state = ptp_port_state(port);

	if (state != PTP_PS_TIME_RECEIVER && state != PTP_PS_UNCALIBRATED) {
		return;
	}

	if (!ptp_port_id_eq(&msg->delay_resp.req_port_id, &port->port_ds.id)) {
		/* Message is not meant for this PTP Port */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&port->delay_req_list, req, node) {
		if (msg->header.sequence_id == ntohs(req->header.sequence_id)) {
			break;
		}
		prev = &req->node;
	}

	if (!req) {
		return;
	}

	t3 = req->timestamp.host.second * NSEC_PER_SEC + req->timestamp.host.nanosecond;
	t4 = msg->timestamp.protocol.second * NSEC_PER_SEC + msg->timestamp.protocol.nanosecond;
	t4c = t4 - (msg->header.correction >> 16);

	ptp_clock_delay(t3, t4c);

	sys_slist_remove(&port->delay_req_list, prev, &req->node);
	ptp_msg_unref(req);

	port->port_ds.log_min_delay_req_interval = msg->header.log_msg_interval;
}

static struct ptp_msg *port_management_resp_prepare(struct ptp_port *port, struct ptp_msg *req)
{
	struct ptp_msg *resp = ptp_msg_alloc();
	const struct ptp_default_ds *dds = ptp_clock_default_ds();

	if (!resp) {
		return NULL;
	}

	resp->header.type_major_sdo_id = PTP_MSG_MANAGEMENT;
	resp->header.version	       = PTP_VERSION;
	resp->header.msg_length	       = sizeof(struct ptp_management_msg);
	resp->header.domain_number     = dds->domain;
	resp->header.src_port_id       = port->port_ds.id;
	resp->header.sequence_id       = req->header.sequence_id;
	resp->header.log_msg_interval  = port->port_ds.log_min_delay_req_interval;

	if (req->management.action == PTP_MGMT_GET ||
	    req->management.action == PTP_MGMT_SET) {
		resp->management.action = PTP_MGMT_RESP;
	} else if (req->management.action == PTP_MGMT_CMD) {
		resp->management.action = PTP_MGMT_ACK;
	}

	memcpy(&resp->management.target_port_id,
	       &req->header.src_port_id,
	       sizeof(struct ptp_port_id));

	resp->management.starting_boundary_hops = req->management.starting_boundary_hops -
						 req->management.boundary_hops;
	resp->management.boundary_hops = resp->management.starting_boundary_hops;

	return resp;
}

static int port_management_resp_tlv_fill(struct ptp_port *port,
					 struct ptp_msg *req,
					 struct ptp_msg *resp,
					 struct ptp_tlv_mgmt *req_mgmt)
{
	int length = 0;
	struct ptp_tlv_mgmt *mgmt;
	struct ptp_tlv_default_ds *tlv_dds;
	struct ptp_tlv_parent_ds *tlv_pds;
	const struct ptp_parent_ds *pds = ptp_clock_parent_ds();
	const struct ptp_current_ds *cds = ptp_clock_current_ds();
	const struct ptp_default_ds *dds = ptp_clock_default_ds();
	const struct ptp_time_prop_ds *tpds = ptp_clock_time_prop_ds();
	struct ptp_tlv_container *container  = ptp_tlv_alloc();

	if (!container) {
		return -ENOMEM;
	}

	container->tlv = (struct ptp_tlv *)resp->management.suffix;
	mgmt = (struct ptp_tlv_mgmt *)container->tlv;
	mgmt->type = PTP_TLV_TYPE_MANAGEMENT;
	mgmt->id = req_mgmt->id;

	switch (mgmt->id) {
	case PTP_MGMT_DEFAULT_DATA_SET:
		tlv_dds = (struct ptp_tlv_default_ds *)mgmt->data;

		length = sizeof(struct ptp_tlv_default_ds);
		tlv_dds->flags = 0x1 | (dds->time_receiver_only << 1);
		tlv_dds->n_ports = dds->n_ports;
		tlv_dds->priority1 = dds->priority1;
		tlv_dds->priority2 = dds->priority2;
		tlv_dds->domain = dds->domain;
		memcpy(&tlv_dds->clk_id, &dds->clk_id, sizeof(tlv_dds->clk_id));
		memcpy(&tlv_dds->clk_quality, &dds->clk_quality, sizeof(tlv_dds->clk_quality));
		break;
	case PTP_MGMT_CURRENT_DATA_SET:
		length = sizeof(struct ptp_tlv_current_ds);
		memcpy(mgmt->data, cds, sizeof(struct ptp_tlv_current_ds));
		break;
	case PTP_MGMT_PARENT_DATA_SET:
		tlv_pds = (struct ptp_tlv_parent_ds *)mgmt->data;

		length = sizeof(struct ptp_tlv_parent_ds);

		tlv_pds->obsreved_parent_offset_scaled_log_variance =
			pds->obsreved_parent_offset_scaled_log_variance;
		tlv_pds->obsreved_parent_clk_phase_change_rate =
			pds->obsreved_parent_clk_phase_change_rate;
		tlv_pds->gm_priority1 = pds->gm_priority1;
		tlv_pds->gm_priority2 = pds->gm_priority2;
		memcpy(&tlv_pds->port_id, &pds->port_id, sizeof(tlv_pds->port_id));
		memcpy(&tlv_pds->gm_id, &pds->gm_id, sizeof(tlv_pds->gm_id));
		memcpy(&tlv_pds->gm_clk_quality,
		       &pds->gm_clk_quality,
		       sizeof(tlv_pds->gm_clk_quality));

		break;
	case PTP_MGMT_TIME_PROPERTIES_DATA_SET:
		length = sizeof(struct ptp_tlv_time_prop_ds);
		memcpy(mgmt->data, tpds, sizeof(struct ptp_tlv_time_prop_ds));
		break;
	case PTP_MGMT_PORT_DATA_SET:
		length = sizeof(struct ptp_tlv_port_ds);
		memcpy(mgmt->data, &port->port_ds, sizeof(struct ptp_tlv_port_ds));
		break;
	case PTP_MGMT_PRIORITY1:
		length = sizeof(dds->priority1);
		*mgmt->data = dds->priority1;
		break;
	case PTP_MGMT_PRIORITY2:
		length = sizeof(dds->priority2);
		*mgmt->data = dds->priority2;
		break;
	case PTP_MGMT_DOMAIN:
		length = sizeof(dds->domain);
		*mgmt->data = dds->domain;
		break;
	case PTP_MGMT_TIME_RECEIVER_ONLY:
		length = sizeof(dds->time_receiver_only);
		*mgmt->data = dds->time_receiver_only;
		break;
	case PTP_MGMT_LOG_ANNOUNCE_INTERVAL:
		length = sizeof(port->port_ds.log_announce_interval);
		*mgmt->data = port->port_ds.log_announce_interval;
		break;
	case PTP_MGMT_LOG_SYNC_INTERVAL:
		length = sizeof(port->port_ds.log_sync_interval);
		*mgmt->data = port->port_ds.log_sync_interval;
		break;
	case PTP_MGMT_VERSION_NUMBER:
		length = sizeof(port->port_ds.version);
		*mgmt->data = port->port_ds.version;
		break;
	case PTP_MGMT_CLOCK_ACCURACY:
		length = sizeof(dds->clk_quality.accuracy);
		*mgmt->data = dds->clk_quality.accuracy;
		break;
	case PTP_MGMT_DELAY_MECHANISM:
		length = sizeof(port->port_ds.delay_mechanism);
		*(uint16_t *)mgmt->data = port->port_ds.delay_mechanism;
		break;
	default:
		ptp_tlv_free(container);
		return -EINVAL;
	}

	/* Management TLV length shall be an even number */
	if (length % 2) {
		mgmt->data[length] = 0;
		length++;
	}

	container->tlv->length = sizeof(mgmt->id) + length;
	resp->header.msg_length += sizeof(*container->tlv) + container->tlv->length;
	sys_slist_append(&resp->tlvs, &container->node);

	return 0;
}

static int port_management_set(struct ptp_port *port,
			       struct ptp_msg *req,
			       struct ptp_tlv_mgmt *tlv)
{
	bool send_resp = false;

	switch (tlv->id) {
	case PTP_MGMT_LOG_ANNOUNCE_INTERVAL:
		port->port_ds.log_announce_interval = *tlv->data;
		send_resp = true;
		break;
	case PTP_MGMT_LOG_SYNC_INTERVAL:
		port->port_ds.log_sync_interval = *tlv->data;
		send_resp = true;
		break;
	case PTP_MGMT_UNICAST_NEGOTIATION_ENABLE:
		/* TODO unicast */
		break;
	default:
		break;
	}

	return send_resp ? ptp_port_management_resp(port, req, tlv) : 0;
}

static int port_enable(struct ptp_port *port)
{
	while (!net_if_is_up(port->iface)) {
		return -1;
	}

	port->link_status = PORT_LINK_UP;

	if (ptp_transport_open(port)) {
		LOG_ERR("Couldn't open socket on Port %d.", port->port_ds.id.port_number);
		return -1;
	}

	port->port_ds.enable = true;

	ptp_clock_pollfd_invalidate();
	LOG_DBG("Port %d opened", port->port_ds.id.port_number);
	return 0;
}

static bool port_is_enabled(struct ptp_port *port)
{
	enum ptp_port_state state = ptp_port_state(port);

	if (state == PTP_PS_FAULTY ||
	    state == PTP_PS_DISABLED ||
	    state == PTP_PS_INITIALIZING) {
		return false;
	}
	return true;
}

static void port_disable(struct ptp_port *port)
{
	k_timer_stop(&port->timers.announce);
	k_timer_stop(&port->timers.delay);
	k_timer_stop(&port->timers.sync);
	k_timer_stop(&port->timers.qualification);

	atomic_clear(&port->timeouts);

	ptp_transport_close(port);
	ptp_port_free_foreign_tts(port);
	port->best = NULL;

	net_if_unregister_timestamp_cb(&port->sync_ts_cb);
	net_if_unregister_timestamp_cb(&port->delay_req_ts_cb);

	ptp_clock_pollfd_invalidate();
	port->port_ds.enable = false;
	LOG_DBG("Port %d disabled", port->port_ds.id.port_number);
}

int port_state_update(struct ptp_port *port, enum ptp_port_event event, bool tt_diff)
{
	enum ptp_port_state next_state = port->state_machine(ptp_port_state(port),
							     event,
							     tt_diff);

	if (next_state == PTP_PS_FAULTY) {
		/* clear fault if interface is UP */
		if (net_if_oper_state(port->iface) == NET_IF_OPER_UP) {
			next_state = port->state_machine(next_state, PTP_EVT_FAULT_CLEARED, false);
		}
	}

	if (next_state == PTP_PS_INITIALIZING) {
		if (port_is_enabled(port)) {
			port_disable(port);
		}
		if (port_enable(port)) {
			event = PTP_EVT_FAULT_DETECTED;
		} else {
			event = PTP_EVT_INIT_COMPLETE;
		}
		next_state = port->state_machine(next_state, event, false);
	}

	if (next_state != ptp_port_state(port)) {
		LOG_DBG("Port %d changed state from %s to %s",
			port->port_ds.id.port_number,
			port_state_str(ptp_port_state(port)),
			port_state_str(next_state));

		port->port_ds.state = next_state;
		return 1;
	}

	return 0;
}

static void port_link_monitor(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event,
			      struct net_if *iface)
{
	ARG_UNUSED(cb);

	enum ptp_port_event event = PTP_EVT_NONE;
	struct ptp_port *port = ptp_clock_port_from_iface(iface);
	uint8_t iface_state = mgmt_event == NET_EVENT_IF_UP ? PORT_LINK_UP : PORT_LINK_DOWN;

	if (!port) {
		return;
	}

	if (iface_state & port->link_status) {
		port->link_status = iface_state;
	} else {
		port->link_status = iface_state | PORT_LINK_CHANGED;
		LOG_DBG("Port %d link %s",
			port->port_ds.id.port_number,
			port->link_status & PORT_LINK_UP ? "up" : "down");
	}

	if (port->link_status & PORT_LINK_CHANGED) {
		event = iface_state == PORT_LINK_UP ?
			PTP_EVT_FAULT_CLEARED : PTP_EVT_FAULT_DETECTED;
		port->link_status ^= PORT_LINK_CHANGED;
	}

	if (port->link_status & PORT_LINK_DOWN) {
		ptp_clock_state_decision_req();
	}

	ptp_port_event_handle(port, event, false);
}

void ptp_port_init(struct net_if *iface, void *user_data)
{
	struct ptp_port *port;
	const struct ptp_default_ds *dds = ptp_clock_default_ds();

	ARG_UNUSED(user_data);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (dds->n_ports > CONFIG_PTP_NUM_PORTS) {
		LOG_WRN("Exceeded number of PTP Ports.");
		return;
	}

	port = &ports[dds->n_ports];

	port->iface = iface;
	port->best = NULL;
	port->socket[PTP_SOCKET_EVENT] = -1;
	port->socket[PTP_SOCKET_GENERAL] = -1;

	port->state_machine = dds->time_receiver_only ? ptp_tr_state_machine : ptp_state_machine;
	port->last_sync_fup = NULL;

	port_ds_init(port);
	sys_slist_init(&port->foreign_list);
	sys_slist_init(&port->delay_req_list);

	port_timer_init(&port->timers.delay, port_timer_to_handler, port);
	port_timer_init(&port->timers.announce, port_timer_to_handler, port);
	port_timer_init(&port->timers.sync, port_timer_to_handler, port);
	port_timer_init(&port->timers.qualification, port_timer_to_handler, port);

	ptp_clock_pollfd_invalidate();
	ptp_clock_port_add(port);

	net_mgmt_init_event_callback(&port->link_cb, port_link_monitor, PORT_LINK_EVENT_MASK);
	net_mgmt_add_event_callback(&port->link_cb);

	LOG_DBG("Port %d initialized", port->port_ds.id.port_number);
}

enum ptp_port_event ptp_port_event_gen(struct ptp_port *port, int idx)
{
	enum ptp_port_event event = PTP_EVT_NONE;
	struct ptp_msg *msg;
	int ret, cnt;

	if (idx < 0) {
		return event;
	}

	msg = ptp_msg_alloc();
	if (!msg) {
		return PTP_EVT_FAULT_DETECTED;
	}

	cnt = ptp_transport_recv(port, msg, idx);
	if (cnt <= 0) {
		LOG_ERR("Error during message reception");
		ptp_msg_unref(msg);
		return PTP_EVT_FAULT_DETECTED;
	}

	ret = ptp_msg_post_recv(port, msg, cnt);
	if (ret) {
		ptp_msg_unref(msg);
		return PTP_EVT_FAULT_DETECTED;
	}

	if (ptp_port_id_eq(&msg->header.src_port_id, &port->port_ds.id)) {
		ptp_msg_unref(msg);
		return PTP_EVT_NONE;
	}

	switch (ptp_msg_type(msg)) {
	case PTP_MSG_SYNC:
		port_sync_msg_process(port, msg);
		break;
	case PTP_MSG_DELAY_REQ:
		if (port_delay_req_msg_process(port, msg)) {
			event = PTP_EVT_FAULT_DETECTED;
		}
		break;
	case PTP_MSG_PDELAY_REQ:
		__fallthrough;
	case PTP_MSG_PDELAY_RESP:
		__fallthrough;
	case PTP_MSG_PDELAY_RESP_FOLLOW_UP:
		/* P2P delay mechanism not supported */
		break;
	case PTP_MSG_FOLLOW_UP:
		port_follow_up_msg_process(port, msg);
		break;
	case PTP_MSG_DELAY_RESP:
		port_delay_resp_msg_process(port, msg);
		break;
	case PTP_MSG_ANNOUNCE:
		if (port_announce_msg_process(port, msg)) {
			event = PTP_EVT_STATE_DECISION;
		}
		break;
	case PTP_MSG_SIGNALING:
		/* Signalling messages not supported */
		break;
	case PTP_MSG_MANAGEMENT:
		if (ptp_clock_management_msg_process(port, msg)) {
			event = PTP_EVT_STATE_DECISION;
		}
		break;
	default:
		break;
	}

	ptp_msg_unref(msg);
	return event;
}

void ptp_port_event_handle(struct ptp_port *port, enum ptp_port_event event, bool tt_diff)
{
	const struct ptp_current_ds *cds = ptp_clock_current_ds();

	if (event == PTP_EVT_NONE) {
		return;
	}

	if (!port_state_update(port, event, tt_diff)) {
		/* No PTP Port state change */
		return;
	}

	k_timer_stop(&port->timers.announce);
	k_timer_stop(&port->timers.delay);
	k_timer_stop(&port->timers.sync);
	k_timer_stop(&port->timers.qualification);

	switch (port->port_ds.state) {
	case PTP_PS_INITIALIZING:
		break;
	case PTP_PS_FAULTY:
		__fallthrough;
	case PTP_PS_DISABLED:
		port_disable(port);
		break;
	case PTP_PS_LISTENING:
		port_timer_set_timeout_random(&port->timers.announce,
					      port->port_ds.announce_receipt_timeout,
					      1,
					      port->port_ds.log_announce_interval);
		break;
	case PTP_PS_PRE_TIME_TRANSMITTER:
		port_timer_set_timeout(&port->timers.qualification,
				       1 + cds->steps_rm,
				       port->port_ds.log_announce_interval);
		break;
	case PTP_PS_GRAND_MASTER:
		__fallthrough;
	case PTP_PS_TIME_TRANSMITTER:
		port_timer_set_timeout(&port->timers.announce,
				       1,
				       port->port_ds.log_announce_interval);
		port_timer_set_timeout(&port->timers.sync, 1, port->port_ds.log_sync_interval);
		break;
	case PTP_PS_PASSIVE:
		port_timer_set_timeout_random(&port->timers.announce,
					      port->port_ds.announce_receipt_timeout,
					      1,
					      port->port_ds.log_announce_interval);
		break;
	case PTP_PS_UNCALIBRATED:
		if (port->last_sync_fup) {
			ptp_msg_unref(port->last_sync_fup);
			port->last_sync_fup = NULL;
		}
		port_clear_delay_req(port);
		__fallthrough;
	case PTP_PS_TIME_RECEIVER:
		port_timer_set_timeout_random(&port->timers.announce,
					      port->port_ds.announce_receipt_timeout,
					      1,
					      port->port_ds.log_announce_interval);
		port_timer_set_timeout_random(&port->timers.delay,
					      0,
					      2,
					      port->port_ds.log_min_delay_req_interval);
		break;
	};
}

enum ptp_port_state ptp_port_state(struct ptp_port *port)
{
	return (enum ptp_port_state)port->port_ds.state;
}

enum ptp_port_event ptp_port_timer_event_gen(struct ptp_port *port, struct k_timer *timer)
{
	enum ptp_port_event event = PTP_EVT_NONE;
	enum ptp_port_state state = ptp_port_state(port);

	switch (state) {
	case PTP_PS_PRE_TIME_TRANSMITTER:
		if (timer == &port->timers.qualification &&
		    atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_QUALIFICATION_TO)) {
			LOG_DBG("Port %d Qualification timeout", port->port_ds.id.port_number);
			atomic_clear_bit(&port->timeouts, PTP_PORT_TIMER_QUALIFICATION_TO);

			return PTP_EVT_QUALIFICATION_TIMEOUT_EXPIRES;
		}
		break;
	case PTP_PS_GRAND_MASTER:
		__fallthrough;
	case PTP_PS_TIME_TRANSMITTER:
		if (timer == &port->timers.sync &&
		    atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_SYNC_TO)) {
			LOG_DBG("Port %d TX Sync timeout", port->port_ds.id.port_number);
			port_timer_set_timeout(&port->timers.sync,
					       1,
					       port->port_ds.log_sync_interval);
			atomic_clear_bit(&port->timeouts, PTP_PORT_TIMER_SYNC_TO);

			return port_sync_msg_transmit(port) == 0 ? PTP_EVT_NONE :
								   PTP_EVT_FAULT_DETECTED;
		}

		if (timer == &port->timers.announce &&
		    atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_ANNOUNCE_TO)) {
			LOG_DBG("Port %d TimeTransmitter Announce timeout",
				port->port_ds.id.port_number);
			port_timer_set_timeout(&port->timers.announce,
					       1,
					       port->port_ds.log_announce_interval);
			atomic_clear_bit(&port->timeouts, PTP_PORT_TIMER_ANNOUNCE_TO);

			return port_announce_msg_transmit(port) == 0 ? PTP_EVT_NONE :
								       PTP_EVT_FAULT_DETECTED;
		}
		break;
	case PTP_PS_TIME_RECEIVER:
		if (timer == &port->timers.delay &&
		    atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_DELAY_TO)) {

			atomic_clear_bit(&port->timeouts, PTP_PORT_TIMER_DELAY_TO);
			port_delay_req_cleanup(port);
			port_timer_set_timeout(&port->timers.delay,
					       1,
					       port->port_ds.log_announce_interval);

			if (port_delay_req_msg_transmit(port) < 0) {
				return PTP_EVT_FAULT_DETECTED;
			}
		}
		__fallthrough;
	case PTP_PS_PASSIVE:
		__fallthrough;
	case PTP_PS_UNCALIBRATED:
		__fallthrough;
	case PTP_PS_LISTENING:
		if ((timer == &port->timers.announce || timer == &port->timers.sync) &&
		    (atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_ANNOUNCE_TO) ||
		     atomic_test_bit(&port->timeouts, PTP_PORT_TIMER_SYNC_TO))) {

			LOG_DBG("Port %d %s timeout",
				port->port_ds.id.port_number,
				timer == &port->timers.announce ? "Announce" : "RX Sync");

			atomic_clear_bit(&port->timeouts, PTP_PORT_TIMER_ANNOUNCE_TO);
			atomic_clear_bit(&port->timeouts, PTP_PORT_TIMER_SYNC_TO);

			if (port->best) {
				port_clear_foreign_clock_records(port->best);
			}

			port_delay_req_cleanup(port);
			port_timer_set_timeout_random(&port->timers.announce,
						      port->port_ds.announce_receipt_timeout,
						      1,
						      port->port_ds.log_announce_interval);

			return PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES;
		}
		break;
	default:
		break;
	}

	return event;
}

bool ptp_port_id_eq(const struct ptp_port_id *p1, const struct ptp_port_id *p2)
{
	return memcmp(p1, p2, sizeof(struct ptp_port_id)) == 0;
}

struct ptp_dataset *ptp_port_best_foreign_ds(struct ptp_port *port)
{
	return port->best ? &port->best->dataset : NULL;
}

struct ptp_foreign_tt_clock *ptp_port_best_foreign(struct ptp_port *port)
{
	struct ptp_foreign_tt_clock *foreign;
	struct ptp_announce_msg *last;

	port->best = NULL;

	if (port->port_ds.time_transmitter_only) {
		return NULL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&port->foreign_list, foreign, node) {
		if (!foreign->messages_count) {
			continue;
		}

		foreign_clock_cleanup(foreign);

		if (foreign->messages_count < FOREIGN_TIME_TRANSMITTER_THRESHOLD) {
			continue;
		}

		last = (struct ptp_announce_msg *)k_fifo_peek_head(&foreign->messages);

		foreign->dataset.priority1 = last->gm_priority1;
		foreign->dataset.priority2 = last->gm_priority2;
		foreign->dataset.steps_rm = last->steps_rm;

		memcpy(&foreign->dataset.clk_quality,
		       &last->gm_clk_quality,
		       sizeof(last->gm_clk_quality));
		memcpy(&foreign->dataset.clk_id, &last->gm_id, sizeof(last->gm_id));
		memcpy(&foreign->dataset.receiver, &port->port_ds.id, sizeof(port->port_ds.id));

		if (!port->best) {
			port->best = foreign;
		} else if (ptp_btca_ds_cmp(&foreign->dataset, &port->best->dataset)) {
			port->best = foreign;
		} else {
			port_clear_foreign_clock_records(foreign);
		}
	}
	return port->best;
}

int ptp_port_add_foreign_tt(struct ptp_port *port, struct ptp_msg *msg)
{
	struct ptp_foreign_tt_clock *foreign;
	struct ptp_msg *last;
	int diff = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&port->foreign_list, foreign, node) {
		if (ptp_port_id_eq(&msg->header.src_port_id, &foreign->dataset.sender)) {
			break;
		}
	}

	if (!foreign) {
		LOG_DBG("Port %d has a new foreign timeTransmitter %s",
			port->port_ds.id.port_number,
			port_id_str(&msg->header.src_port_id));

		int ret = k_mem_slab_alloc(&foreign_tts_slab, (void **)&foreign, K_NO_WAIT);

		if (ret) {
			LOG_ERR("Couldn't allocate memory for new foreign timeTransmitter");
			return 0;
		}

		memset(foreign, 0, sizeof(*foreign));
		memcpy(&foreign->dataset.sender,
		       &msg->header.src_port_id,
		       sizeof(foreign->dataset.sender));
		k_fifo_init(&foreign->messages);
		foreign->port = port;

		sys_slist_append(&port->foreign_list, &foreign->node);

		/* First message is not added to records. */
		return 0;
	}

	foreign_clock_cleanup(foreign);
	ptp_msg_ref(msg);

	foreign->messages_count++;
	k_fifo_put(&foreign->messages, (void *)msg);

	if (foreign->messages_count > 1) {
		last = (struct ptp_msg *)k_fifo_peek_tail(&foreign->messages);
		diff = ptp_msg_announce_cmp(&msg->announce, &last->announce);
	}

	return (foreign->messages_count == FOREIGN_TIME_TRANSMITTER_THRESHOLD ? 1 : 0) || diff;
}

void ptp_port_free_foreign_tts(struct ptp_port *port)
{
	sys_snode_t *iter;
	struct ptp_foreign_tt_clock *foreign;

	while (!sys_slist_is_empty(&port->foreign_list)) {
		iter = sys_slist_get(&port->foreign_list);
		foreign = CONTAINER_OF(iter, struct ptp_foreign_tt_clock, node);

		while (foreign->messages_count > FOREIGN_TIME_TRANSMITTER_THRESHOLD) {
			struct ptp_msg *msg = (struct ptp_msg *)k_fifo_get(&foreign->messages,
									   K_NO_WAIT);
			foreign->messages_count--;
			ptp_msg_unref(msg);
		}

		k_mem_slab_free(&foreign_tts_slab, (void *)foreign);
	}
}

int ptp_port_update_current_time_transmitter(struct ptp_port *port, struct ptp_msg *msg)
{
	struct ptp_foreign_tt_clock *foreign = port->best;

	if (!foreign ||
	    !ptp_port_id_eq(&msg->header.src_port_id, &foreign->dataset.sender)) {
		return ptp_port_add_foreign_tt(port, msg);
	}

	foreign_clock_cleanup(foreign);
	ptp_msg_ref(msg);

	k_fifo_put(&foreign->messages, (void *)msg);
	foreign->messages_count++;

	port_timer_set_timeout_random(&port->timers.announce,
				      port->port_ds.announce_receipt_timeout,
				      1,
				      port->port_ds.log_announce_interval);

	if (foreign->messages_count > 1) {
		struct ptp_msg *last = (struct ptp_msg *)k_fifo_peek_tail(&foreign->messages);

		return ptp_msg_announce_cmp(&msg->announce, &last->announce);
	}

	return 0;
}

int ptp_port_management_msg_process(struct ptp_port *port,
				    struct ptp_port *ingress,
				    struct ptp_msg *msg,
				    struct ptp_tlv_mgmt *tlv)
{
	int ret = 0;
	uint16_t target_port = msg->management.target_port_id.port_number;

	if (target_port != port->port_ds.id.port_number && target_port != 0xFFFF) {
		return ret;
	}

	if (ptp_mgmt_action(msg) == PTP_MGMT_SET) {
		ret = port_management_set(port, msg, tlv);
	} else {
		ret = ptp_port_management_resp(port, msg, tlv);
	}

	return ret;
}

int ptp_port_management_error(struct ptp_port *port, struct ptp_msg *msg, enum ptp_mgmt_err err)
{
	int ret;
	struct ptp_tlv *tlv;
	struct ptp_tlv_mgmt_err *mgmt_err;
	struct ptp_tlv_mgmt *mgmt = (struct ptp_tlv_mgmt *)msg->management.suffix;

	struct ptp_msg *resp = port_management_resp_prepare(port, msg);

	if (!resp) {
		return -ENOMEM;
	}

	tlv = ptp_msg_add_tlv(resp, sizeof(struct ptp_tlv_mgmt_err));
	if (!tlv) {
		ptp_msg_unref(resp);
		return -ENOMEM;
	}

	mgmt_err = (struct ptp_tlv_mgmt_err *)tlv;

	mgmt_err->type = PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS;
	mgmt_err->length = 8;
	mgmt_err->err_id = err;
	mgmt_err->id = mgmt->id;

	ret = port_msg_send(port, resp, PTP_SOCKET_GENERAL);
	ptp_msg_unref(resp);

	if (ret) {
		return -EFAULT;
	}

	LOG_DBG("Port %d sends Menagement Error message", port->port_ds.id.port_number);
	return 0;
}

int ptp_port_management_resp(struct ptp_port *port, struct ptp_msg *req, struct ptp_tlv_mgmt *tlv)
{
	int ret;
	struct ptp_msg *resp = port_management_resp_prepare(port, req);

	if (!resp) {
		return -ENOMEM;
	}

	ret = port_management_resp_tlv_fill(port, req, resp, tlv);
	if (ret) {
		return ret;
	}

	ret = port_msg_send(port, resp, PTP_SOCKET_GENERAL);
	ptp_msg_unref(resp);

	if (ret < 0) {
		return -EFAULT;
	}

	LOG_DBG("Port %d sends Menagement message response", port->port_ds.id.port_number);
	return 0;
}

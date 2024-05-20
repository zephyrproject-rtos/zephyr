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

#include "clock.h"
#include "port.h"
#include "transport.h"

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

	port = ports + dds->n_ports;

	port->iface = iface;
	port->best = NULL;
	port->socket[PTP_SOCKET_EVENT] = -1;
	port->socket[PTP_SOCKET_GENERAL] = -1;

	port->state_machine = dds->time_receiver_only ? ptp_tr_state_machine : ptp_state_machine;
	port_ds_init(port);
	sys_slist_init(&port->foreign_list);

	port_timer_init(&port->timers.delay, port_timer_to_handler, port);
	port_timer_init(&port->timers.announce, port_timer_to_handler, port);
	port_timer_init(&port->timers.sync, port_timer_to_handler, port);
	port_timer_init(&port->timers.qualification, port_timer_to_handler, port);

	ptp_clock_pollfd_invalidate();
	ptp_clock_port_add(port);

	LOG_DBG("Port %d initialized", port->port_ds.id.port_number);
}

bool ptp_port_id_eq(const struct ptp_port_id *p1, const struct ptp_port_id *p2)
{
	return memcmp(p1, p2, sizeof(struct ptp_port_id)) == 0;
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

		k_mem_slab_alloc(&foreign_tts_slab, (void **)&foreign, K_NO_WAIT);
		if (!foreign) {
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

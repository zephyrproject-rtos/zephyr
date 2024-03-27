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

static struct ptp_port ports[CONFIG_PTP_NUM_PORTS];

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
	port->socket[0] = -1;
	port->socket[1] = -1;

	port->state_machine = dds->time_receiver_only ? ptp_tr_state_machine : ptp_state_machine;
	port_ds_init(port);

	port_timer_init(&port->timers.delay, port_timer_to_handler, port);
	port_timer_init(&port->timers.announce, port_timer_to_handler, port);
	port_timer_init(&port->timers.sync, port_timer_to_handler, port);
	port_timer_init(&port->timers.qualification, port_timer_to_handler, port);

	ptp_clock_pollfd_invalidate();
	ptp_clock_port_add(port);

	LOG_DBG("Port %d initialized", port->port_ds.id.port_number);
}

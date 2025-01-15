/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp, CONFIG_PTP_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ptp.h>

#include "clock.h"
#include "port.h"
#include "transport.h"

K_KERNEL_STACK_DEFINE(ptp_stack, CONFIG_PTP_STACK_SIZE);

static struct k_thread ptp_thread_data;

static void ptp_thread(void *p1, void *p2, void *p3)
{
	static const size_t timer_offset[] = {
		offsetof(struct ptp_port, timers.announce),
		offsetof(struct ptp_port, timers.delay),
		offsetof(struct ptp_port, timers.sync),
		offsetof(struct ptp_port, timers.qualification)
	};
	static const int timeout_bit[] = {
		PTP_PORT_TIMER_ANNOUNCE_TO,
		PTP_PORT_TIMER_DELAY_TO,
		PTP_PORT_TIMER_SYNC_TO,
		PTP_PORT_TIMER_QUALIFICATION_TO,
	};

	struct k_timer *timer;
	struct ptp_port *port;
	struct zsock_pollfd *fd;
	enum ptp_port_event event;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		fd = ptp_clock_poll_sockets();

		SYS_SLIST_FOR_EACH_CONTAINER(ptp_clock_ports_list(), port, node) {

			for (int i = 0; i < ARRAY_SIZE(timer_offset); i++) {
				timer = (struct k_timer *)((uint8_t *)port +
							   timer_offset[i]);

				if (!atomic_test_bit(&port->timeouts, timeout_bit[i])) {
					continue;
				}

				event = ptp_port_timer_event_gen(port, timer);

				if (event == PTP_EVT_STATE_DECISION ||
				    event == PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES) {
					ptp_clock_state_decision_req();
				}

				ptp_port_event_handle(port, event, false);
			}

			for (int i = 0; i < PTP_SOCKET_CNT; i++, fd++) {
				if (!(fd->revents & (ZSOCK_POLLIN | ZSOCK_POLLPRI))) {
					continue;
				}

				event = ptp_port_event_gen(port, i);

				if (event == PTP_EVT_STATE_DECISION ||
				    event == PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES) {
					ptp_clock_state_decision_req();
				}

				ptp_port_event_handle(port, event, false);
			}
		}

		ptp_clock_handle_state_decision_evt();
	}
}

static int ptp_init(void)
{
	k_tid_t tid;
	const struct ptp_clock *domain = ptp_clock_init();
	struct ptp_port *port;

	if (!domain) {
		return -ENODEV;
	}

	net_if_foreach(ptp_port_init, NULL);

	SYS_SLIST_FOR_EACH_CONTAINER(ptp_clock_ports_list(), port, node) {
		ptp_port_event_handle(port, PTP_EVT_INITIALIZE, false);
	}

	tid = k_thread_create(&ptp_thread_data, ptp_stack, K_KERNEL_STACK_SIZEOF(ptp_stack),
			      ptp_thread, NULL, NULL, NULL,
			      K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&ptp_thread_data, "PTP");

	return 0;
}

SYS_INIT(ptp_init, APPLICATION, CONFIG_PTP_INIT_PRIO);

static enum net_verdict ptp_recv(struct net_if *iface, uint16_t ptype,
				 struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(ptype);

	net_pkt_set_family(pkt, AF_UNSPEC);

	return NET_CONTINUE;
}

ETH_NET_L3_REGISTER(PTP, NET_ETH_PTYPE_PTP, ptp_recv);

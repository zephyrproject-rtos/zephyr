/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/sched.h>

int xen_sched_poll(evtchn_port_t *ports, unsigned int nr_ports, uint64_t timeout)
{
	struct sched_poll poll = {
		.ports.p = ports,
		.nr_ports = 1,
		.timeout = timeout,
	};

	return HYPERVISOR_sched_op(SCHEDOP_poll, &poll);
}

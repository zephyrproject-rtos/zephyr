/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/hypercall.h>
#include <zephyr/xen/sched.h>

#include <errno.h>
#include <stddef.h>

int xen_sched_yield(void)
{
	return HYPERVISOR_sched_op(SCHEDOP_yield, NULL);
}

int xen_sched_block(void)
{
	return HYPERVISOR_sched_op(SCHEDOP_block, NULL);
}

int xen_sched_poll(evtchn_port_t *ports, unsigned int nr_ports, uint64_t timeout)
{
	struct sched_poll poll = {
		.nr_ports = nr_ports,
		.timeout = timeout,
	};

	if (!ports || !nr_ports) {
		return -EINVAL;
	}

	set_xen_guest_handle(poll.ports, ports);

	return HYPERVISOR_sched_op(SCHEDOP_poll, &poll);
}

int xen_sched_shutdown(unsigned int reason)
{
	struct sched_shutdown shutdown = {
		.reason = reason,
	};

	return HYPERVISOR_sched_op(SCHEDOP_shutdown, &shutdown);
}

int xen_sched_remote_shutdown(domid_t domid, unsigned int reason)
{
	struct sched_remote_shutdown shutdown = {
		.domain_id = domid,
		.reason = reason,
	};

	return HYPERVISOR_sched_op(SCHEDOP_remote_shutdown, &shutdown);
}

int xen_sched_shutdown_code(unsigned int reason)
{
	struct sched_shutdown shutdown = {
		.reason = reason,
	};

	return HYPERVISOR_sched_op(SCHEDOP_shutdown_code, &shutdown);
}

int xen_sched_pin_override(int32_t pcpu)
{
	struct sched_pin_override pin_override = {
		.pcpu = pcpu,
	};

	return HYPERVISOR_sched_op(SCHEDOP_pin_override, &pin_override);
}

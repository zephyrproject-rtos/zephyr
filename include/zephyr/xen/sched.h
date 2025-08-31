/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_XEN_SCHED_H_
#define ZEPHYR_XEN_SCHED_H_

#include <zephyr/xen/public/sched.h>

/**
 * @brief Poll one or more Xen event channels for activity.
 *
 * Issues the SCHEDOP_poll hypercall to wait for events on the specified ports.
 *
 * @param ports      Array of event channel ports to poll.
 * @param nr_ports   Number of ports in the array.
 * @param timeout    Timeout in microseconds to wait for an event.
 * @return           0 if an event occurred, -EAGAIN on timeout, or negative errno on error.
 */
int xen_sched_poll(evtchn_port_t *ports, unsigned int nr_ports, uint64_t timeout);

#endif /* ZEPHYR_XEN_SCHED_H_ */

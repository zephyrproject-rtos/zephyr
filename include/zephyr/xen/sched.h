/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_XEN_SCHED_H_
#define ZEPHYR_XEN_SCHED_H_

#include <xen/public/sched.h>

int sched_poll(evtchn_port_t *ports, unsigned int nr_ports, uint64_t timeout);

#endif /* ZEPHYR_XEN_SCHED_H_ */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Xen scheduler operation helpers.
 */

#ifndef ZEPHYR_INCLUDE_XEN_SCHED_H_
#define ZEPHYR_INCLUDE_XEN_SCHED_H_

#include <xen/public/sched.h>

/**
 * @brief Yield the current VCPU.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_yield(void);

/**
 * @brief Block the current VCPU until an event is available.
 *
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_block(void);

/**
 * @brief Poll event-channel ports until one becomes pending or a timeout expires.
 *
 * @param ports Event-channel ports to poll.
 * @param nr_ports Number of entries in @p ports.
 * @param timeout Absolute Xen system time timeout, or 0 to poll without timeout.
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_poll(evtchn_port_t *ports, unsigned int nr_ports, uint64_t timeout);

/**
 * @brief Shut down the current domain with the supplied shutdown reason.
 *
 * @param reason SHUTDOWN_* reason code.
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_shutdown(unsigned int reason);

/**
 * @brief Request shutdown of another domain.
 *
 * @param domid Domain ID to shut down.
 * @param reason SHUTDOWN_* reason code.
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_remote_shutdown(domid_t domid, unsigned int reason);

/**
 * @brief Store a shutdown reason for later domain shutdown reporting.
 *
 * @param reason SHUTDOWN_* reason code.
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_shutdown_code(unsigned int reason);

/**
 * @brief Override or restore the current VCPU physical CPU pinning.
 *
 * @param pcpu Physical CPU to pin to, or -1 to restore normal pinning behavior.
 * @return 0 on success, negative errno value on failure.
 */
int xen_sched_pin_override(int32_t pcpu);

#endif /* ZEPHYR_INCLUDE_XEN_SCHED_H_ */

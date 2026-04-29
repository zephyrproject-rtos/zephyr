/**
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_TIMEOUT_PRIV_H_
#define ZEPHYR_KERNEL_INCLUDE_TIMEOUT_PRIV_H_

#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>

/**
 * Shared timeout subsystem state
 *
 * Exactly one backend (timeout.c or timeout_heap.c) provides the definitions.
 */

extern struct k_spinlock timeout_lock;
extern uint64_t curr_tick;
extern int announce_remaining;

/**
 * elapsed() - ticks consumed by the hardware since the last announce.
 *
 * While sys_clock_announce() is executing, new relative timeouts will be
 * scheduled relatively to the currently firing timeout's original tick
 * value (=curr_tick) rather than relative to the current sys_clock_elapsed().
 *
 * This means that timeouts being scheduled from within timeout callbacks will be
 * scheduled at well-defined offsets from the currently firing timeout.
 *
 * As a side effect, the same will happen if an ISR with higher priority preempts
 * a timeout callback and schedules a timeout.
 *
 * The distinction is implemented by looking at announce_remaining which will be
 * non-zero while sys_clock_announce() is executing and zero otherwise.
 */
static inline int32_t elapsed(void)
{
	return announce_remaining == 0 ? sys_clock_elapsed() : 0U;
}

#endif /* ZEPHYR_KERNEL_INCLUDE_TIMEOUT_PRIV_H_ */

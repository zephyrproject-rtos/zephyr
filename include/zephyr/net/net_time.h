/*
 * Copyright (c) 2023 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Representation of nanosecond resolution elapsed time and timestamps in
 * the network stack.
 *
 * Inspired by
 * https://github.com/torvalds/linux/blob/master/include/linux/ktime.h and
 * https://github.com/torvalds/linux/blob/master/[tools/]include/linux/time64.h
 *
 * @defgroup net_time Network time representation.
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_TIME_H_
#define ZEPHYR_INCLUDE_NET_NET_TIME_H_

/* Include required for NSEC_PER_* constants. */
#include <zephyr/sys_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Any occurrence of net_time_t specifies a concept of nanosecond
 * resolution scalar time span.
 *
 * @details Timed network protocols (PTP, TDMA, ...) usually require several
 * local or remote interfaces to share a common notion of elapsed time within
 * well-defined tolerances. Network uptime therefore differs from time
 * represented by a single hardware counter peripheral in that it will need to
 * be represented in several distinct hardware peripherals with different
 * frequencies, accuracy and precision. To co-operate, these hardware counters
 * will have to be "syntonized" or "disciplined" (i.e. frequency and phase
 * locked) with respect to a common local or remote network reference time
 * signal. Be aware that while syntonized clocks share the same frequency and
 * phase, they do not usually share the same epoch (zero-point).
 *
 */
typedef uint64_t net_time_t;

/** The largest time value that can be represented by net_time_t */
#define NET_TIME_MAX UINT64_MAX

/** The smallest time value that can be represented by net_time_t */
#define NET_TIME_MIN 0

/** The largest number of seconds that can be safely represented by net_time_t */
#define NET_TIME_SEC_MAX (NET_TIME_MAX / NSEC_PER_SEC)

/** The smallest number of seconds that can be safely represented by net_time_t */
#define NET_TIME_SEC_MIN (NET_TIME_MIN / NSEC_PER_SEC)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_TIME_H_ */

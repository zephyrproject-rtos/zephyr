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
 * resolution scalar time span, future (positive) or past (negative) relative
 * time or absolute timestamp referred to some local network uptime reference
 * clock that does not wrap during uptime and is - in a certain, well-defined
 * sense - common to all local network interfaces, sometimes even to remote
 * interfaces on the same network.
 *
 * This type is EXPERIMENTAL. Usage is currently restricted to representation of
 * time within the network subsystem.
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
 * This also explains why network time, if represented as a cycle value of some
 * specific hardware counter, will never be "precise" but only can be "good
 * enough" with respect to the tolerances (resolution, drift, jitter) required
 * by a given network protocol. All counter peripherals involved in a timed
 * network protocol must comply with these tolerances.
 *
 * Please use specific cycle/tick counter values rather than net_time_t whenever
 * possible especially when referring to the kernel system clock or values of
 * any single counter peripheral.
 *
 * net_time_t cannot represent general clocks referred to an arbitrary epoch as
 * it only covers roughly +/- ~290 years. It also cannot be used to represent
 * time according to a more complex timescale (e.g. including leap seconds, time
 * adjustments, complex calendars or time zones). In these cases you may use
 * @ref timespec (C11, POSIX.1-2001), @ref timeval (POSIX.1-2001) or broken down
 * time as in @ref tm (C90). The advantage of net_time_t over these structured
 * time representations is lower memory footprint, faster and simpler scalar
 * arithmetics and easier conversion from/to low-level hardware counter values.
 * Also net_time_t can be used in the network stack as well as in applications
 * while POSIX concepts cannot. Converting net_time_t from/to structured time
 * representations is possible in a limited way but - except for @ref timespec -
 * requires concepts that must be implemented by higher-level APIs. Utility
 * functions converting from/to @ref timespec will be provided as part of the
 * net_time_t API as and when needed.
 *
 * If you want to represent more coarse grained scalar time in network
 * applications, use @ref time_t (C99, POSIX.1-2001) which is specified to
 * represent seconds or @ref suseconds_t (POSIX.1-2001) for microsecond
 * resolution. Kernel @ref k_ticks_t and cycles (both specific to Zephyr) have
 * an unspecified resolution but are useful to represent kernel timer values and
 * implement high resolution spinning.
 *
 * If you need even finer grained time resolution, you may want to look at
 * (g)PTP concepts, see @ref net_ptp_extended_time.
 *
 * The reason why we don't use int64_t directly to represent scalar nanosecond
 * resolution times in the network stack is that it has been shown in the past
 * that fields using generic type will often not be used correctly (e.g. with
 * the wrong resolution or to represent underspecified concepts of time with
 * unclear syntonization semantics).
 *
 * Any API that exposes or consumes net_time_t values SHALL ensure that it
 * maintains the specified contract including all protocol specific tolerances
 * and therefore clients can rely on common semantics of this type. This makes
 * times coming from different hardware peripherals and even from different
 * network nodes comparable within well-defined limits and therefore net_time_t
 * is the ideal intermediate building block for timed network protocols.
 */
typedef int64_t net_time_t;

/** The largest positive time value that can be represented by net_time_t */
#define NET_TIME_MAX INT64_MAX

/** The smallest negative time value that can be represented by net_time_t */
#define NET_TIME_MIN INT64_MIN

/** The largest positive number of seconds that can be safely represented by net_time_t */
#define NET_TIME_SEC_MAX (NET_TIME_MAX / NSEC_PER_SEC)

/** The smallest negative number of seconds that can be safely represented by net_time_t */
#define NET_TIME_SEC_MIN (NET_TIME_MIN / NSEC_PER_SEC)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_NET_TIME_H_ */

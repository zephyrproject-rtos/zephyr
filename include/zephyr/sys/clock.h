/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System clock APIs
 *
 * APIs for getting, setting, and sleeping with respect to system clocks.
 */

#ifndef ZEPHYR_INCLUDE_SYSCLOCK_H_
#define ZEPHYR_INCLUDE_SYSCLOCK_H_

#include <time.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief The real-time clock (i.e. "wall clock")
 *
 * This clock is used to measure time since the epoch (1970-01-01 00:00:00 UTC).
 *
 * It is not a steady clock; i.e. it may be adjusted for a number of reasons from initialization
 * of a hardware real-time-clock, to network-time synchronization, to manual adjustment from the
 * application.
 */
#define SYS_CLOCK_REALTIME 1

/**
 * @brief The monotonic clock
 *
 * This steady clock is used to measure time since the system booted. Time from this clock is
 * always monotonically increasing.
 */
#define SYS_CLOCK_MONOTONIC 4

/**
 * @brief The flag used for specifying absolute timeouts
 *
 * This flag may be passed to @ref sys_clock_nanosleep to indicate the requested timeout is an
 * absolute time with respect to the specified clock.
 */
#define SYS_TIMER_ABSTIME 4

/**
 * @brief Get the offset @ref SYS_CLOCK_REALTIME with respect to @ref SYS_CLOCK_MONOTONIC
 *
 * The "wall clock" (i.e. @ref SYS_CLOCK_REALTIME) depends on a base time that is set by the
 * system. The base time may be updated for a number of reasons, such as initialization of a
 * hardware real-time-clock (RTC), network time protocol (NTP) synchronization, or manual
 * adjustment by the application.
 *
 * This function retrieves the current time offset, as a `timespec` object, for
 * @ref SYS_CLOCK_REALTIME, with respect to @ref SYS_CLOCK_MONOTONIC, and writes it to the
 * provided memory location pointed-to by @a tp.
 *
 * @note This function may assert if @a tp is NULL.
 *
 * @param tp Pointer to memory where time will be written.
 */
__syscall void sys_clock_getrtoffset(struct timespec *tp);

/**
 * @brief Get the current time from the specified clock
 *
 * @param clock_id The clock from which to query time.
 * @param tp Pointer to memory where time will be written.
 * @retval 0 on success.
 * @retval -EINVAL when an invalid @a clock_id is specified.
 */
int sys_clock_gettime(int clock_id, struct timespec *tp);

/**
 * @brief Set the current time for the specified clock
 *
 * @param clock_id The clock for which the time should be set.
 * @param tp Pointer to memory specifying the desired time.
 * @retval 0 on success.
 * @retval -EINVAL when an invalid @a clock_id is specified or when @a tp contains nanoseconds
 * outside of the range `[0, 999999999]`.
 */
__syscall int sys_clock_settime(int clock_id, const struct timespec *tp);

/**
 * @brief Sleep for the specified amount of time with respect to the specified clock.
 *
 * This function will cause the calling thread to sleep either
 * - until the absolute time specified by @a rqtp (if @a flags includes @ref SYS_TIMER_ABSTIME), or
 * - until the relative time specified by @a rqtp (if @a flags does not include
 *   @ref SYS_TIMER_ABSTIME).
 *
 * The accepted values for @a clock_id include
 * - @ref SYS_CLOCK_REALTIME
 * - @ref SYS_CLOCK_MONOTONIC
 *
 * If @a rmtp is not NULL, and the thread is awoken prior to the time specified by @a rqtp, then
 * any remaining time will be written to @a rmtp. If the thread has slept for at least the time
 * specified by @a rqtp, then @a rmtp will be set to zero.
 *
 * @param clock_id The clock to by which to sleep.
 * @param flags Flags to modify the behavior of the sleep operation.
 * @param rqtp Pointer to the requested time to sleep.
 * @param rmtp Pointer to memory into which to copy the remaining time, if any.
 *
 * @retval 0 on success.
 * @retval -EINVAL when an invalid @a clock_id, when @a rqtp contains nanoseconds outside of the
 *         range `[0, 999999999]`, or when @a rqtp contains a negative value.
 */
__syscall int sys_clock_nanosleep(int clock_id, int flags, const struct timespec *rqtp,
				  struct timespec *rmtp);

/**
 * @}
 */

#include <zephyr/syscalls/clock.h>

#ifdef __cplusplus
}
#endif

#endif

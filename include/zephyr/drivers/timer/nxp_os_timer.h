/*
 * Copyright (c) 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_NXP_OS_TIMER_H
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_NXP_OS_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The timer may have overflowed and triggered a wakeup.
 *
 * OS Timer for certain low power modes switches to a counter maintain
 * system time as it is powered off. These counters could overflow for
 * certain tick values like K_TICKS_FOREVER. This could trigger a wakeup
 * event which the system could ignore and go back to sleep.
 *
 * @retval true  if the PM subsystem should ignore wakeup events and go back
 *               to sleep.
 *         false if the timer wakeup event is as expected
 */
bool z_nxp_os_timer_ignore_timer_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_NXP_OS_TIMER_H */

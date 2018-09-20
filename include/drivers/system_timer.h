/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Timer driver API
 *
 *
 * Declare API implemented by system timer driver and used by kernel components.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <device.h>
#include <stdbool.h>

extern int _sys_clock_driver_init(struct device *device);

extern void _timer_int_handler(void *arg);

/**
 * @brief Set system clock timeout
 *
 * Informs the system clock driver that the next needed call to
 * z_clock_announce() will not be until the specified number of ticks
 * from the the current time have elapsed.  Note that spurious calls
 * to z_clock_announce() are allowed (i.e. it's legal to announce
 * every tick and implement this function as a noop), the requirement
 * is that one tick announcement should occur within one tick after
 * the specified expiration.
 *
 * Note that ticks can also be passed the special value K_FOREVER,
 * indicating that no future timer interrupts are expected or required
 * and that the system is permitted to enter an indefinite sleep even
 * if this could cause rolloever of the internal counter (i.e. the
 * system uptime counter is allowed to be wrong, see
 * k_enable_sys_clock_always_on().
 *
 * Note also that it is conventional for the kernel to pass INT_MAX
 * for ticks if it wants to preserve the uptime tick count but doesn't
 * have a specific event to await.  The intent here is that the driver
 * will schedule any needed timeout as far into the future as
 * possible.  For the specific case of INT_MAX, the next call to
 * z_clock_announce() may occur at any point in the future, not just
 * at INT_MAX ticks.  But the correspondence between the announced
 * ticks and real-world time must be correct.
 *
 * @param ticks Timeout in tick units
 * @param idle Hint to the driver that the system is about to enter
 *        the idle state immediately after setting the timeout
 */
extern void z_clock_set_timeout(s32_t ticks, bool idle);

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE
extern void sys_clock_disable(void);
#endif

#ifdef CONFIG_TICKLESS_IDLE
extern void _timer_idle_exit(void);
#else
#define _timer_idle_exit() do { } while (false)
#endif /* CONFIG_TICKLESS_IDLE */

/**
 * @brief Announce time progress to the kernel
 *
 * Informs the kernel that the specified number of ticks have elapsed
 * since the last call to z_clock_announce() (or system startup for
 * the first call).
 *
 * @param ticks Elapsed time, in ticks
 */
extern void z_clock_announce(s32_t ticks);

#ifdef CONFIG_TICKLESS_KERNEL
extern u32_t _get_program_time(void);
extern u32_t _get_remaining_program_time(void);
extern u32_t _get_elapsed_program_time(void);
extern u64_t _get_elapsed_clock_time(void);
#endif

extern int sys_clock_device_ctrl(struct device *device,
				 u32_t ctrl_command, void *context);

/*
 * Currently regarding timers, only loapic timer and arcv2_timer0 implements
 * device pm functionality. For other timers, use default handler in case
 * the app enables CONFIG_DEVICE_POWER_MANAGEMENT.
 */
#if !defined(CONFIG_LOAPIC_TIMER) && !defined(CONFIG_ARCV2_TIMER)
#define sys_clock_device_ctrl device_pm_control_nop
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSTEM_TIMER_H_ */

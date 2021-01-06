/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_LEGACY_SET_TIME_H__
#define ZEPHYR_LEGACY_SET_TIME_H__

/* Stub implementation of z_clock_set_timeout() and z_clock_elapsed()
 * in terms of the original APIs.  Used by older timer drivers.
 * Should be replaced.
 *
 * Yes, this "header" includes function definitions and must be
 * included only once in a single compilation.
 */

#ifdef CONFIG_TICKLESS_IDLE
void z_timer_idle_enter(int32_t ticks);
void z_clock_idle_exit(void);
#endif

#ifdef CONFIG_TICKLESS_KERNEL
void z_set_time(uint32_t time);
extern uint32_t z_get_program_time(void);
extern uint32_t z_get_remaining_program_time(void);
extern uint32_t z_get_elapsed_program_time(void);
#endif

extern uint64_t z_clock_uptime(void);

void z_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_IDLE) && defined(CONFIG_TICKLESS_KERNEL)
	if (idle) {
		z_timer_idle_enter(ticks);
	} else {
		z_set_time(ticks == K_TICKS_FOREVER ? 0 : ticks);
	}
#endif	/* TICKLESS_IDLE && TICKLESS_KERNEL */
}

/* The old driver "now" API would return a full uptime value.  The new
 * one only requires the driver to track ticks since the last announce
 * call.  Implement the new call in terms of the old one on legacy
 * drivers by keeping (yet another) uptime value locally.
 */
static uint32_t driver_uptime;

uint32_t z_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return (uint32_t)(z_clock_uptime() - driver_uptime);
#else
	return 0;
#endif
}

static void wrapped_announce(int32_t ticks)
{
	driver_uptime += ticks;
	z_clock_announce(ticks);
}

#define z_clock_announce(t) wrapped_announce(t)

#define _sys_clock_always_on (0)

static inline void z_tick_set(int64_t val)
{
	/* noop with current kernel code, use z_clock_announce() */
	ARG_UNUSED(val);
}

#endif /* ZEPHYR_LEGACY_SET_TIME_H__ */

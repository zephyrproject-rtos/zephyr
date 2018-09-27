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
void _timer_idle_enter(s32_t ticks);
void z_clock_idle_exit(void);
#endif

#ifdef CONFIG_TICKLESS_KERNEL
void _set_time(u32_t time);
extern u32_t _get_program_time(void);
extern u32_t _get_remaining_program_time(void);
extern u32_t _get_elapsed_program_time(void);
#endif

extern u64_t z_clock_uptime(void);

void z_clock_set_timeout(s32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (idle) {
		_timer_idle_enter(ticks);
	} else {
		_set_time(ticks == K_FOREVER ? 0 : ticks);
	}
#endif
}

/* The old driver "now" API would return a full uptime value.  The new
 * one only requires the driver to track ticks since the last announce
 * call.  Implement the new call in terms of the old one on legacy
 * drivers by keeping (yet another) uptime value locally.
 */
static u32_t driver_uptime;

u32_t z_clock_elapsed(void)
{
	return (u32_t)(z_clock_uptime() - driver_uptime);
}

static void wrapped_announce(s32_t ticks)
{
	driver_uptime += ticks;
	z_clock_announce(ticks);
}

#define z_clock_announce(t) wrapped_announce(t)

#endif /* ZEPHYR_LEGACY_SET_TIME_H__ */

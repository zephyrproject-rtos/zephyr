#ifndef ZEPHYR_LEGACY_SET_TIME_H__
#define ZEPHYR_LEGACY_SET_TIME_H__

/* Stub implementation of z_clock_set_timeout() in terms of the
 * original APIs.  Used by older timer drivers.  Should be replaced.
 *
 * Yes, this "header" includes a function definition and must be
 * included only once in a single compilation.
 */


#ifdef CONFIG_TICKLESS_KERNEL
void _set_time(u32_t time);
#endif

#ifdef CONFIG_TICKLESS_IDLE
void _timer_idle_enter(s32_t ticks);
void z_clock_idle_exit(void);
#endif

extern void z_clock_set_timeout(s32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	if (idle) {
		_timer_idle_enter(ticks);
	} else {
		_set_time(ticks == K_FOREVER ? 0 : ticks);
	}
#endif
}

#endif /* ZEPHYR_LEGACY_SET_TIME_H__ */

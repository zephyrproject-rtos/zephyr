/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "dev/epoch_time"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#include <kernel.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <init.h>
#include <rtc.h>
#include <time/epoch_time.h>

static volatile u64_t _base_usecs;
static volatile u64_t _base_cycles;

#if defined(EPOCH_TIME_DEBUG)
static void print_u64(char *string, u64_t v)
{
	u32_t hp, lp;

	hp = v >> 32;
	lp = (u32_t)v;

	SYS_LOG_DBG("%s %lx %lx", string, hp, lp);
}
#define PRINT_U64(...) print_64(##__VA_ARGS__)
#else
#define PRINT_U64(...) do {} while (0)
#endif

#if defined(CONFIG_RTC)
static u64_t read_rtc(void)
{
	struct device *rtc_dev = NULL;
	u32_t ticks;
	u32_t hz;
	u64_t us = 0;

	rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	if (!rtc_dev) {
		goto error_exit;
	}

	hz = rtc_get_ticks_per_sec(rtc_dev);
	if (hz == 0) {
		/* Divide by zero */
		goto error_exit;
	}

	ticks = rtc_read(rtc_dev);
	/* Performance isuse if hz is not power of 2 */
	us = ticks * USEC_PER_SEC / hz;

error_exit:
	return us;
}
#endif

#if defined(EPOCH_TIME_RTC_WRITE_THROUGH)
static void write_rtc(u32_t s, u32_t us)
{
	struct device *rtc_dev = NULL;
	u32_t hz;
	u32_t ticks;

	rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	if (!rtc_dev) {
		return;
	}

	hz = rtc_get_ticks_per_sec(rtc_dev);
	if (hz == 0) {
		return;
	}

	ticks = s * hz + (hz * us / USEC_PER_SEC);
	rtc_set_time(rtc_dev, ticks);
}
#endif

int epoch_time_init(struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_RTC)
	_base_usecs = read_rtc();
#else
	_base_usecs = 0;
#endif

	PRINT_U64("_base_usecs: ", _base_usecs);
	PRINT_U64("_base_cycles: ", _base_cycles);

	return 0;
}

SYS_INIT(epoch_time_init, POST_KERNEL, CONFIG_EPOCH_TIME_INIT_PRIORITY);

int epoch_time_get(struct epoch_time *time)
{
	u64_t now;
	u64_t diff;
	u64_t us;
	u64_t q;
	u64_t r;
	u64_t cycles_per_usec = sys_clock_hw_cycles_per_sec / USEC_PER_SEC;

	if (!time) {
		return -EFAULT;
	}

	now = k_cycle_get();
	diff = now - _base_cycles;

	/* Convert hw cycles to us */
	us = _base_usecs + (diff / cycles_per_usec);
	/* Convert us to s */
	q = us / USEC_PER_SEC;
	/* Calculate remainder us */
	r = us - (q * USEC_PER_SEC);

	time->secs = q;
	time->usecs = r;

	PRINT_U64("now: ", now);
	PRINT_U64("base hw cycle: ", _base_cycles);
	PRINT_U64("diff hw cycle: ", diff);

	return 0;
}

int epoch_time_set(struct epoch_time *time)
{
	if (!time) {
		return -EFAULT;
	}

	_base_usecs = (time->secs * USEC_PER_SEC) + time->usecs;
	_base_cycles = k_cycle_get();

#if defined(EPOCH_TIME_RTC_WRITE_THROUGH)
	write_rtc(time->secs, time->usecs);
#endif

	return 0;
}

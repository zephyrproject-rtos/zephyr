/* test_tickless.c - tickless idle tests */

/*
 * Copyright (c) 2011, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* DESCRIPTION Unit test for tickless idle feature. */

#include <zephyr.h>
#include <ztest.h>

#include <misc/printk.h>
#include <arch/cpu.h>
#include <tc_util.h>

#define STACKSIZE 4096
#define PRIORITY 6

#define SLEEP_TICKS 10

static struct k_thread thread_tickless;
static K_THREAD_STACK_DEFINE(thread_tickless_stack, STACKSIZE);

#ifdef CONFIG_TICKLESS_IDLE
extern s32_t _sys_idle_threshold_ticks;
#endif

#define TICKS_TO_MS  (MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)


/* NOTE: Clock speed may change between platforms */

#define CAL_REPS 16    /* # of loops in timestamp calibration */

/*
 * Arch-specific timer resolution/size types, definitions and
 * timestamp routines.
 */

#if defined(CONFIG_X86) || defined(CONFIG_ARC)
typedef u64_t _timer_res_t;
#define _TIMER_ZERO  0ULL

/* timestamp routines */
#define _TIMESTAMP_OPEN()
#define _TIMESTAMP_READ()	(_tsc_read())
#define _TIMESTAMP_CLOSE()

#elif defined(CONFIG_ARM)

#  if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
/* A bug in the QEMU ARMv7-M sysTick timer prevents tickless idle support */
#error "This QEMU target does not support tickless idle!"
#  endif

typedef u32_t _timer_res_t;
#define _TIMER_ZERO  0

/* timestamp routines, from timestamps.c */
extern void _timestamp_open(void);
extern u32_t _timestamp_read(void);
extern void _timestamp_close(void);

#define _TIMESTAMP_OPEN()	(_timestamp_open())
#define _TIMESTAMP_READ()	(_timestamp_read())
#define _TIMESTAMP_CLOSE()	(_timestamp_close())

#else
#error "Unknown target"
#endif

void ticklessTestThread(void)
{
	s32_t start_time;
	s32_t end_time;
	s32_t diff_time;
	s32_t diff_ticks;
	_timer_res_t start_tsc;
	_timer_res_t end_tsc;
	_timer_res_t cal_tsc = _TIMER_ZERO;
	_timer_res_t diff_tsc = _TIMER_ZERO;
	_timer_res_t diff_per;

#ifdef CONFIG_TICKLESS_IDLE
	s32_t oldThreshold;
#endif
	int i;

	printk("Tickless Idle Test\n");
#ifndef CONFIG_TICKLESS_IDLE
	printk("WARNING! Tickless idle support has not been enabled!\n");
#endif

	printk("Calibrating TSC...\n");
#ifdef CONFIG_TICKLESS_IDLE
	oldThreshold = _sys_idle_threshold_ticks;
	/* make sure we do not enter tickless idle mode */
	_sys_idle_threshold_ticks = 0x7FFFFFFF;
#endif

	/* initialize the timer, if necessary */
	_TIMESTAMP_OPEN();

	for (i = 0; i < CAL_REPS; i++) {
		/*
		 * Do a single tick sleep to get us as close to a tick boundary
		 * as we can.
		 */
		k_sleep(TICKS_TO_MS);
		start_time = k_uptime_get_32();
		start_tsc = _TIMESTAMP_READ();
		/* FIXME: one tick less to account for
		 * one  extra tick for _TICK_ALIGN in k_sleep
		 */
		k_sleep((SLEEP_TICKS - 1) * TICKS_TO_MS);
		end_tsc = _TIMESTAMP_READ();
		end_time = k_uptime_get_32();
		cal_tsc += end_tsc - start_tsc;
	}
	cal_tsc /= CAL_REPS;

#if defined(CONFIG_X86) || defined(CONFIG_ARC)
	printk("Calibrated time stamp period = 0x%x%x\n",
		   (u32_t)(cal_tsc >> 32), (u32_t)(cal_tsc & 0xFFFFFFFFLL));
#elif defined(CONFIG_ARM)
	printk("Calibrated time stamp period = 0x%x\n", cal_tsc);
#endif

	printk("Do the real test with tickless enabled\n");

#ifdef CONFIG_TICKLESS_IDLE
	_sys_idle_threshold_ticks = oldThreshold;
#endif

	printk("Going idle for %d ticks...\n", SLEEP_TICKS);

	for (i = 0; i < CAL_REPS; i++) {
		/*
		 * Do a single tick sleep to get us as close to a tick boundary
		 * as we can.
		 */
		k_sleep(TICKS_TO_MS);
		start_time = k_uptime_get_32();
		start_tsc = _TIMESTAMP_READ();
		/* FIXME: one tick less to account for
		 * one  extra tick for _TICK_ALIGN in k_sleep
		 */
		k_sleep((SLEEP_TICKS - 1) * TICKS_TO_MS);
		end_tsc = _TIMESTAMP_READ();
		end_time = k_uptime_get_32();
		diff_tsc += end_tsc - start_tsc;
	}

	diff_tsc /= CAL_REPS;

	diff_time = (end_time - start_time);
	/* Convert ms to ticks*/
	diff_ticks = (diff_time / TICKS_TO_MS);

	printk("start time     : %d\n", start_time);
	printk("end   time     : %d\n", end_time);
	printk("diff  time     : %d\n", diff_time);
	printk("diff  ticks    : %d\n", diff_ticks);

#if defined(CONFIG_X86) || defined(CONFIG_ARC)
	printk("diff  time stamp: 0x%x%x\n",
		   (u32_t)(diff_tsc >> 32), (u32_t)(diff_tsc & 0xFFFFFFFFULL));
	printk("Cal   time stamp: 0x%x%x\n",
		   (u32_t)(cal_tsc >> 32), (u32_t)(cal_tsc & 0xFFFFFFFFLL));
#elif defined(CONFIG_ARM) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
	printk("diff  time stamp: 0x%x\n", diff_tsc);
	printk("Cal   time stamp: 0x%x\n", cal_tsc);
#endif

	/*
	 * Calculate percentage difference between
	 * calibrated TSC diff and measured result
	 */
	if (diff_tsc > cal_tsc) {
		diff_per = (100 * (diff_tsc - cal_tsc)) / cal_tsc;
	} else {
		diff_per = (100 * (cal_tsc - diff_tsc)) / cal_tsc;
	}

	printk("variance in time stamp diff: %d percent\n", (s32_t)diff_per);

	zassert_equal(diff_ticks, SLEEP_TICKS,
			"* TEST FAILED. TICK COUNT INCORRECT *");

	/* release the timer, if necessary */
	_TIMESTAMP_CLOSE();

	while (1) {
		;
	}

}

void test_tickless(void)
{
	k_thread_create(&thread_tickless, thread_tickless_stack,
			STACKSIZE,
			(k_thread_entry_t) ticklessTestThread,
			NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);
	k_sleep(4000);
}

void test_main(void)
{
	ztest_test_suite(test_tick_less,
			ztest_unit_test(test_tickless));
	ztest_run_test_suite(test_tick_less);
}

/* test_tickless.c - tickless idle tests */

/*
 * Copyright (c) 2011, 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION
Unit test for tickless idle feature.
 */

#include <zephyr.h>

#include <misc/printk.h>
#include <arch/cpu.h>
#include <tc_util.h>

#define SLEEP_TICKS 10

#ifdef CONFIG_TICKLESS_IDLE
extern int32_t _sys_idle_threshold_ticks;
#endif

/* NOTE: Clock speed may change between platforms */

#define CAL_REPS 16    /* # of loops in timestamp calibration */

/*
 * Arch-specific timer resolution/size types, definitions and
 * timestamp routines.
 */

#if defined(CONFIG_X86)
typedef uint64_t _timer_res_t;
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

typedef uint32_t _timer_res_t;
#define _TIMER_ZERO  0

/* timestamp routines, from timestamps.c */
extern void _TimestampOpen(void);
extern uint32_t _TimestampRead(void);
extern void _TimestampClose(void);

#define _TIMESTAMP_OPEN()	(_TimestampOpen())
#define _TIMESTAMP_READ()	(_TimestampRead())
#define _TIMESTAMP_CLOSE()	(_TimestampClose())

#elif defined(CONFIG_ARC)

typedef uint32_t _timer_res_t;
#define _TIMER_ZERO  0

extern void timestamp_open(void);
extern uint32_t timestamp_read(void);
extern void timestamp_close(void);

#define _TIMESTAMP_OPEN()	(timestamp_open())
#define _TIMESTAMP_READ()	(timestamp_read())
#define _TIMESTAMP_CLOSE()	(timestamp_close())

#else
#error "Unknown target"
#endif

void ticklessTestTask(void)
{
	int32_t start_ticks;
	int32_t end_ticks;
	int32_t diff_ticks;
	_timer_res_t start_tsc;
	_timer_res_t end_tsc;
	_timer_res_t cal_tsc = _TIMER_ZERO;
	_timer_res_t diff_tsc = _TIMER_ZERO;
	_timer_res_t diff_per;

#ifdef CONFIG_TICKLESS_IDLE
	int32_t oldThreshold;
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
		task_sleep(1);
		start_ticks = sys_tick_get_32();
		start_tsc = _TIMESTAMP_READ();
		task_sleep(SLEEP_TICKS);
		end_tsc = _TIMESTAMP_READ();
		end_ticks = sys_tick_get_32();
		cal_tsc += end_tsc - start_tsc;
	}
	cal_tsc /= CAL_REPS;

#if defined(CONFIG_X86)
	printk("Calibrated time stamp period = 0x%x%x\n",
		   (uint32_t)(cal_tsc >> 32), (uint32_t)(cal_tsc & 0xFFFFFFFFLL));
#elif defined(CONFIG_ARM)  || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
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
		task_sleep(1);
		start_ticks = sys_tick_get_32();
		start_tsc = _TIMESTAMP_READ();
		task_sleep(SLEEP_TICKS);
		end_tsc = _TIMESTAMP_READ();
		end_ticks = sys_tick_get_32();
		diff_tsc += end_tsc - start_tsc;
	}

	diff_tsc /= CAL_REPS;

	diff_ticks = end_ticks - start_ticks;

	printk("start ticks     : %d\n", start_ticks);
	printk("end   ticks     : %d\n", end_ticks);
	printk("diff  ticks     : %d\n", diff_ticks);

#if defined(CONFIG_X86)
	printk("diff  time stamp: 0x%x%x\n",
		   (uint32_t)(diff_tsc >> 32), (uint32_t)(diff_tsc & 0xFFFFFFFFULL));
	printk("Cal   time stamp: 0x%x%x\n",
		   (uint32_t)(cal_tsc >> 32), (uint32_t)(cal_tsc & 0xFFFFFFFFLL));
#elif defined(CONFIG_ARM) || defined(CONFIG_SOC_QUARK_SE_C1000_SS)
	printk("diff  time stamp: 0x%x\n", diff_tsc);
	printk("Cal   time stamp: 0x%x\n", cal_tsc);
#endif

	/* Calculate percentage difference between calibrated TSC diff and measured result */
	if (diff_tsc > cal_tsc) {
		diff_per = (100 * (diff_tsc - cal_tsc)) / cal_tsc;
	} else {
		diff_per = (100 * (cal_tsc - diff_tsc)) / cal_tsc;
	}

	printk("variance in time stamp diff: %d percent\n", (int32_t)diff_per);

	if (diff_ticks != SLEEP_TICKS) {
		printk("* TEST FAILED. TICK COUNT INCORRECT *\n");
		TC_END_REPORT(TC_FAIL);
	} else {
		TC_END_REPORT(TC_PASS);
	}

	/* release the timer, if necessary */
	_TIMESTAMP_CLOSE();

	while (1);

}

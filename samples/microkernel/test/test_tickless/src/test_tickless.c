/* test_tickless.c - tickless idle tests */

/*
 * Copyright (c) 2011, 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

/* Clock speed - will change from BSP to BSP */
#define CAL_REPS 16

/*
 * Arch-specific timer resolution/size types, definitions and
 * timestamp routines.
 */

#if defined(CONFIG_X86_32)
typedef uint64_t _timer_res_t;
#define _TIMER_ZERO  0ULL

/* timestamp routines */
#define _TIMESTAMP_OPEN()
#define _TIMESTAMP_READ()	(_NanoTscRead())
#define _TIMESTAMP_CLOSE()

#elif defined(CONFIG_ARM)

#  if defined(CONFIG_BSP_TI_LM3S6965_QEMU)
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
		start_ticks = task_tick_get_32();
		start_tsc = _TIMESTAMP_READ();
		task_sleep(SLEEP_TICKS);
		end_tsc = _TIMESTAMP_READ();
		end_ticks = task_tick_get_32();
		cal_tsc += end_tsc - start_tsc;
	}
	cal_tsc /= CAL_REPS;

#if defined(CONFIG_X86_32)
	printk("Calibrated time stamp period = 0x%x%x\n",
		   (uint32_t)(cal_tsc >> 32), (uint32_t)(cal_tsc & 0xFFFFFFFFLL));
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
		task_sleep(1);
		start_ticks = task_tick_get_32();
		start_tsc = _TIMESTAMP_READ();
		task_sleep(SLEEP_TICKS);
		end_tsc = _TIMESTAMP_READ();
		end_ticks = task_tick_get_32();
		diff_tsc += end_tsc - start_tsc;
	}

	diff_tsc /= CAL_REPS;

	diff_ticks = end_ticks - start_ticks;

	printk("start ticks     : %d\n", start_ticks);
	printk("end   ticks     : %d\n", end_ticks);
	printk("diff  ticks     : %d\n", diff_ticks);

#if defined(CONFIG_X86_32)
	printk("diff  time stamp: 0x%x%x\n",
		   (uint32_t)(diff_tsc >> 32), (uint32_t)(diff_tsc & 0xFFFFFFFFULL));
	printk("Cal   time stamp: 0x%x%x\n",
		   (uint32_t)(cal_tsc >> 32), (uint32_t)(cal_tsc & 0xFFFFFFFFLL));
#elif defined(CONFIG_ARM)
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

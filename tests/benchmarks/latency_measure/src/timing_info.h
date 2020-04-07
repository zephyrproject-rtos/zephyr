/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TIMING_INFO_H_
#define _TIMING_INFO_H_

#include <timestamp.h>

#ifdef CONFIG_NRF_RTC_TIMER
#include <soc.h>

/* To get current count of timer, first 1 need to be written into
 * Capture Register and Current Count will be copied into corresponding
 * current count register.
 */
#define TIMING_INFO_PRE_READ()        (NRF_TIMER2->TASKS_CAPTURE[0] = 1)
#define TIMING_INFO_OS_GET_TIME()     (NRF_TIMER2->CC[0])

#elif CONFIG_SOC_SERIES_MEC1501X
#include <soc.h>

#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()     (B32TMR1_REGS->CNT)

#else
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#endif

/******************************************************************************/
/* NRF RTC TIMER runs ar very slow rate (32KHz), So in order to measure
 * Kernel starts a dedicated timer to measure kernel stats.
 */
#ifdef CONFIG_NRF_RTC_TIMER
#define NANOSECS_PER_SEC (1000000000)
#define CYCLES_PER_SEC   (16000000/(1 << NRF_TIMER2->PRESCALER))

#define CYCLES_TO_NS(x)        ((x) * (NANOSECS_PER_SEC/CYCLES_PER_SEC))
#define CYCLES_TO_NS_AVG(x, NCYCLES)	\
	(CYCLES_TO_NS(x) / NCYCLES)

/* Configure Timer parameters */
static inline void benchmark_timer_init(void)
{
	NRF_TIMER2->TASKS_CLEAR = 1;	/* Clear Timer */
	NRF_TIMER2->MODE = 0;		/* Timer Mode */
	NRF_TIMER2->PRESCALER = 0;	/* 16M Hz */
	NRF_TIMER2->BITMODE = 3;	/* 32 - bit */
}

/* Stop the timer */
static inline void benchmark_timer_stop(void)
{
	NRF_TIMER2->TASKS_STOP = 1;	/* Stop Timer */
}

/*Start the timer */
static inline void benchmark_timer_start(void)
{
	NRF_TIMER2->TASKS_START = 1;	/* Start Timer */
}

#elif CONFIG_SOC_SERIES_MEC1501X

#define NANOSECS_PER_SEC	(1000000000)
#define CYCLES_PER_SEC		(48000000)
#define CYCLES_TO_NS(x)		((x) * (NANOSECS_PER_SEC/CYCLES_PER_SEC))
#define CYCLES_TO_NS_AVG(x, NCYCLES)	\
	(CYCLES_TO_NS(x) / NCYCLES)

/* Configure Timer parameters */
static inline void benchmark_timer_init(void)
{
	/* Setup counter */
	B32TMR1_REGS->CTRL =
		MCHP_BTMR_CTRL_ENABLE |
		MCHP_BTMR_CTRL_AUTO_RESTART |
		MCHP_BTMR_CTRL_COUNT_UP;

	B32TMR1_REGS->PRLD = 0;		/* Preload */
	B32TMR1_REGS->CNT = 0;		/* Counter value */

	B32TMR1_REGS->IEN = 0;		/* Disable interrupt */
	B32TMR1_REGS->STS = 1;		/* Clear interrupt */
}

/* Stop the timer */
static inline void benchmark_timer_stop(void)
{
	B32TMR1_REGS->CTRL &= ~MCHP_BTMR_CTRL_START;
}

/* Start the timer */
static inline void benchmark_timer_start(void)
{
	B32TMR1_REGS->CTRL |= MCHP_BTMR_CTRL_START;
}

#else  /* All other architectures */
/* Done because weak attribute doesn't work on static inline. */
static inline void benchmark_timer_init(void)  {       }
static inline void benchmark_timer_stop(void)  {       }
static inline void benchmark_timer_start(void) {       }

#define CYCLES_TO_NS(x) ((u32_t)k_cyc_to_ns_floor64(x))
#define CYCLES_TO_NS_AVG(x, NCYCLES) \
	((u32_t)(k_cyc_to_ns_floor64(x) / NCYCLES))

#endif /* CONFIG_NRF_RTC_TIMER */

static inline u32_t TIMING_INFO_GET_DELTA(u32_t start, u32_t end)
{
	return (end >= start) ? (end - start) : (ULONG_MAX - start + end);
}

#endif /* _TIMING_INFO_H_ */

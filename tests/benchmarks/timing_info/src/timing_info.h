/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <timestamp.h>
#include <kernel_internal.h>

#define CALCULATE_TIME(special_char, profile, name)			     \
	{								     \
		total_##profile##_##name##_time = CYCLES_TO_NS( \
			special_char##profile##_##name##_end_time -	     \
			special_char##profile##_##name##_start_time);	     \
	}

#define DECLARE_VAR(profile, name) \
	u64_t total_##profile##_##name##_time;

/* Stack size for all the threads created in this benchmark */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

/******************************************************************************/
/*
 * Note on: SUBTRACT_CLOCK_CYCLES
 * If we are using x86 based controller we tend to read the tsc value which is
 * always incrementing i.e count up counter.
 * If we are using the ARM based controllers the systick is a
 * count down counter.
 * If we are using nrf SOC, we are using external timer which always increments
 * ie count up counter.
 * Hence to calculate the cycles taken up by the code we need to adjust the
 * values accordingly.
 *
 * NOTE: Needed only when reading value from end of swap operation
 */
#if defined(CONFIG_NRF_RTC_TIMER)

/* To get current count of timer, first 1 need to be written into
 * Capture Register and Current Count will be copied into corresponding
 * current count register.
 */
#define TIMING_INFO_PRE_READ()        (NRF_TIMER2->TASKS_CAPTURE[0] = 1)
#define TIMING_INFO_OS_GET_TIME()     (NRF_TIMER2->CC[0])
#define TIMING_INFO_GET_TIMER_VALUE() (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)    (val)

#elif defined(CONFIG_SOC_SERIES_MEC1501X)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()     (B32TMR1_REGS->CNT)
#define TIMING_INFO_GET_TIMER_VALUE() (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)    (val)

#elif defined(CONFIG_X86)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (z_tsc_read())
#define TIMING_INFO_GET_TIMER_VALUE()  (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)     (val)

#elif defined(CONFIG_CPU_CORTEX_M)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (SysTick->VAL)
#define SUBTRACT_CLOCK_CYCLES(val)     (SysTick->LOAD - (u32_t)val)

#elif defined(CONFIG_ARC)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()     (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE() (z_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT))
#define SUBTRACT_CLOCK_CYCLES(val)    ((u32_t)val)

#elif defined(CONFIG_NIOS2)
#include "altera_avalon_timer_regs.h"
#define TIMING_INFO_PRE_READ()         \
	(IOWR_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE, 10))

#define TIMING_INFO_OS_GET_TIME()      (SUBTRACT_CLOCK_CYCLES(\
	((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE) << 16)\
	| ((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE))))

#define TIMING_INFO_GET_TIMER_VALUE()  (\
	((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE) << 16)\
	| ((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE)))

#define SUBTRACT_CLOCK_CYCLES(val)     \
	((IORD_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE)	\
	  << 16 |						\
	  (IORD_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE)))	\
	 - ((u32_t)val))

#else
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (k_cycle_get_32())
#define SUBTRACT_CLOCK_CYCLES(val)     ((u32_t)val)
#endif

/******************************************************************************/
/* NRF RTC TIMER runs ar very slow rate (32KHz), So in order to measure
 * Kernel starts a dedicated timer to measure kernel stats.
 */
#if defined(CONFIG_NRF_RTC_TIMER)
#define NANOSECS_PER_SEC (1000000000)
#define CYCLES_PER_SEC   (16000000/(1 << NRF_TIMER2->PRESCALER))

#define CYCLES_TO_NS(x)        ((x) * (NANOSECS_PER_SEC/CYCLES_PER_SEC))
#define PRINT_STATS(x, y, z)   PRINT_F(x, (y*((SystemCoreClock)/	\
					      CYCLES_PER_SEC)), z)

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

/* Get Core Frequency in MHz */
static inline u32_t get_core_freq_MHz(void)
{
	return SystemCoreClock/1000000;
}

#elif defined(CONFIG_SOC_SERIES_MEC1501X)

#define NANOSECS_PER_SEC	(1000000000)
#define CYCLES_PER_SEC		(48000000)
#define CYCLES_TO_NS(x)		((x) * (NANOSECS_PER_SEC/CYCLES_PER_SEC))
#define PRINT_STATS(x, y, z)   PRINT_F(x, y, z)

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

/* 48MHz counter frequency */
static inline u32_t get_core_freq_MHz(void)
{
	return CYCLES_PER_SEC;
}

#elif defined(CONFIG_X86)

static inline void benchmark_timer_init(void)  {       }
static inline void benchmark_timer_stop(void)  {       }
static inline void benchmark_timer_start(void) {       }

extern u32_t x86_cyc_to_ns_floor64(u64_t cyc);
extern u32_t x86_get_timer_freq_MHz(void);

#define CYCLES_TO_NS(x) x86_cyc_to_ns_floor64(x)

static inline u32_t get_core_freq_MHz(void)
{
	return x86_get_timer_freq_MHz();
}

#define PRINT_STATS(x, y, z)   PRINT_F(x, y, z)

#else  /* All other architectures */
/* Done because weak attribute doesn't work on static inline. */
static inline void benchmark_timer_init(void)  {       }
static inline void benchmark_timer_stop(void)  {       }
static inline void benchmark_timer_start(void) {       }

#define CYCLES_TO_NS(x) (u32_t)k_cyc_to_ns_floor64(x)

/* Get Core Frequency in MHz */
static inline u32_t get_core_freq_MHz(void)
{
	return  (sys_clock_hw_cycles_per_sec() / 1000000);
}

#define PRINT_STATS(x, y, z)   PRINT_F(x, y, z)
#endif /* CONFIG_NRF_RTC_TIMER */

/******************************************************************************/
/* PRINT_F
 * Macro to print a formatted output string. fprintf is used when
 * Assumed that sline character array of SLINE_LEN + 1 characters
 * is defined in the main file
 */

/* #define CSV_FORMAT_OUTPUT */
/* printf format defines. */
#ifdef CSV_FORMAT_OUTPUT
#define FORMAT "%-45s,%4u,%5u\n"
#else
#define FORMAT "%-45s:%4u cycles , %5u ns\n"
#endif
#include <stdio.h>

#define GET_2ND_ARG(first, second, ...) (second)
#define GET_3ND_ARG(first, second, third, ...) (third)

/* Enable this macro to print all the measurements.
 * Note: Some measurements in few architectures are not valid
 */
#define PRINT_ALL_MEASUREMENTS
#ifndef PRINT_ALL_MEASUREMENTS
/*If the measured cycles is greater than 10000 then one of the following is
 * possible.
 * 1. the selected measurement is not supported in the architecture
 * 2. The measurement went wrong somewhere.(less likely to happen)
 */
#define PRINT_F(...)						     \
	{							     \
		if ((GET_2ND_ARG(__VA_ARGS__) <= 20000) &&	     \
		    (GET_2ND_ARG(__VA_ARGS__) != 0)) {		     \
			snprintf(sline, 254, FORMAT, ##__VA_ARGS__); \
			TC_PRINT("%s", sline);			     \
		}						     \
	}
#else
/* Prints all outputs*/
#define PRINT_F(...)						     \
	{							     \
		snprintf(sline, 254, FORMAT, ##__VA_ARGS__); \
		TC_PRINT("%s", sline);			     \
	}

#endif

/******************************************************************************/
/* Function prototypes */
void system_thread_bench(void);
void yield_bench(void);
void heap_malloc_free_bench(void);
void semaphore_bench(void);
void mutex_bench(void);
void msg_passing_bench(void);
void userspace_bench(void);

#ifdef CONFIG_USERSPACE
#include <syscall_handler.h>
__syscall int k_dummy_syscall(void);
__syscall u32_t userspace_read_timer_value(void);
__syscall int validation_overhead_syscall(void);
#include <syscalls/timing_info.h>
#endif	/* CONFIG_USERSPACE */

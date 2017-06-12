/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <timestamp.h>


#define CALCULATE_TIME(special_char, profile, name)			     \
	{								     \
		total_##profile##_##name##_time = SYS_CLOCK_HW_CYCLES_TO_NS( \
			special_char##profile##_##name##_end_tsc -	     \
			special_char##profile##_##name##_start_tsc);	     \
	}

/*total_##profile##_##name##_time =
 * profile##_##name##_end_us - profile##_##name##_start_us;
 */

#define DECLARE_VAR(profile, name) \
	u64_t total_##profile##_##name##_time;

extern u64_t __start_swap_tsc;
extern u64_t __end_swap_tsc;
extern u64_t __start_intr_tsc;
extern u64_t __end_intr_tsc;
extern u64_t __start_tick_tsc;
extern u64_t __end_tick_tsc;



/* Function prototypes */
void system_thread_bench(void);
void yield_bench(void);
void heap_malloc_free_bench(void);
void semaphore_bench(void);
void mutex_bench(void);
void msg_passing_bench(void);

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
/* #define PRINT_ALL_MEASUREMENTS */
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


/* If we are using x86 based controller we tend to read the tsc value which is
 * always incrementing i.e count up counter.
 * If we are using the ARM based controllers the systick is a
 * count down counter.
 * Hence to calculate the cycles taken up by the code we need to adjust the
 * values accordingly.
 *
 * NOTE: Needed only when reading value from end of swap operation
 */
#if CONFIG_ARM
#define SUBTRACT_CLOCK_CYCLES(val) (SysTick->LOAD - (u32_t)val)
#else
#define SUBTRACT_CLOCK_CYCLES(val) (val)
#endif

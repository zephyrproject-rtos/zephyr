/* timestamp.h - macroses for measuring time in benchmarking tests */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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
 * DESCRIPTION
 * This file contains the macroses for taking and converting time for
 * benchmarking tests.
 */

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <limits.h>

#include <timemacro.h>


#if defined (CONFIG_NANOKERNEL)

#include <nanokernel.h>

#define OS_GET_TIME() nano_node_cycle_get_32 ()

/* number of ticks before timer overflows */
#define BENCH_MAX_TICKS (sys_clock_ticks_per_sec - 1)

typedef uint64_t TICK_TYPE;
#define TICK_GET(x) (TICK_TYPE)nano_node_tick_delta(x)

static inline void TICK_SYNCH (void)
{
	TICK_TYPE  reftime;

	(void) nano_node_tick_delta(&reftime);
	while (nano_node_tick_delta(&reftime) == 0) {
	}
}

#elif (defined (CONFIG_MICROKERNEL) && defined (KERNEL))

#define OS_GET_TIME() task_node_cycle_get_32 ()

typedef int64_t TICK_TYPE;
#define TICK_GET(x) (TICK_TYPE)task_node_tick_delta(x)

#define TICK_SYNCH() task_sleep(1)

#else
#error either  CONFIG_NANOKERNEL or CONFIG_MICROKERNEL must be defined
#endif /*  CONFIG_NANOKERNEL */

/* time necessary to read the time */
extern uint32_t tm_off;

#if defined(__DCC__)
__asm volatile void _TIME_STAMP_DELTA_GET_inline (void)
    {
%
! "ax", "bx", "cx", "dx"
    xorl   %eax, %eax
    pushl  %ebx
    cpuid
    popl   %ebx
    }
#endif

static inline uint32_t TIME_STAMP_DELTA_GET (uint32_t ts)
    {
    uint32_t t;

    /* serialize so OS_GET_TIME() is not reordered */
#if defined(__GNUC__)
    __asm__ __volatile__ (/* serialize */ \
	"xorl %%eax,%%eax \n        cpuid"	\
	::: "%eax", "%ebx", "%ecx", "%edx");
#elif defined(__DCC__)
    _TIME_STAMP_DELTA_GET_inline ();
#endif

    t = OS_GET_TIME ();
    uint32_t res = (t >= ts)? (t - ts): (ULONG_MAX - ts + t);
    if (ts > 0)
	res -= tm_off;
    return res;
    }

/*
 * Routine initializes the benchmark timing measurement
 * The function sets up the global variable tm_off
 */
static inline void bench_test_init (void)
    {
    uint32_t t = OS_GET_TIME ();
    tm_off = OS_GET_TIME () - t;
    }

#if defined (CONFIG_MICROKERNEL) && defined (KERNEL)
#include <vxmicro.h>

/* number of ticks before timer overflows */
#define BENCH_MAX_TICKS (sys_clock_ticks_per_sec - 1)

#endif /* CONFIG_MICROKERNEL */

#if (defined (CONFIG_NANOKERNEL) || defined (CONFIG_MICROKERNEL))  && defined (KERNEL)
/* tickstamp used for timer counter overflow check */
static TICK_TYPE tCheck;

/*
 * Routines are invoked before and after the benchmark and check
 * if penchmarking code took less time than necessary for the
 * high precision timer register overflow.
 * Functions modify the tCheck global variable.
 */
static inline void bench_test_start (void)
    {
    tCheck = 0;
    /* before reading time we synchronize to the start of the timer tick */
    TICK_SYNCH ();
    tCheck = TICK_GET(&tCheck);
    }


/* returns 0 if the number of ticks is valid and -1 if not */
static inline int bench_test_end (void)
    {
    tCheck = TICK_GET(&tCheck);
    if (tCheck > BENCH_MAX_TICKS)
	return -1;
    else
	return 0;
    }

/*
 * Returns -1 if number of ticks cause high precision timer counter
 * overflow and 0 otherwise
 * Called after bench_test_end to see if we still can use timing
 * results or is it completely invalid
 */
static inline int high_timer_overflow (void)
    {
    if (tCheck >= (UINT_MAX / sys_clock_hw_cycles_per_tick))
	return -1;
    else
	return 0;
    }
#endif /*  CONFIG_NANOKERNEL || CONFIG_MICROKERNEL */

#endif /* _TIMESTAMP_H_ */

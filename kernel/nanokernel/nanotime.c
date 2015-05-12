/* nanotime.c - time tracking for nanokernel-only systems */

/*
 * Copyright (c) 1997-2015 Wind River Systems, Inc.
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

#ifdef CONFIG_NANOKERNEL

#include <nanok.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/system_timer.h>

#ifdef CONFIG_SYS_CLOCK_EXISTS
int sys_clock_us_per_tick = 1000000 / sys_clock_ticks_per_sec;
int sys_clock_hw_cycles_per_tick =
	sys_clock_hw_cycles_per_sec / sys_clock_ticks_per_sec;
#else
/* don't initialize to avoid division-by-zero error */
int sys_clock_us_per_tick;
int sys_clock_hw_cycles_per_tick;
#endif

int64_t _nano_ticks = 0;
struct nano_timer *_nano_timer_list = NULL;

/*******************************************************************************
*
* nano_time_init - constructor that initializes nanokernel time tracking system
*
* RETURNS: N/A
*
*/

NANO_INIT_SYS_NORMAL void nano_time_init(void)
{
	timer_driver(0); /* note: priority parameter is unused */
}

#ifdef VXMICRO_ARCH_arm
void (*__ctor_nano_time_init)(void) __attribute__((section(".ctors.250"))) =
	nano_time_init;
#endif

/*******************************************************************************
*
* nano_tick_get_32 - return the lower part of the current system tick count
*
* RETURNS: the current system tick count
*
*/

uint32_t nano_tick_get_32(void)
{
	return (uint32_t)_nano_ticks;
}

/*******************************************************************************
*
* nano_tick_get - return the current system tick count
*
* RETURNS: the current system tick count
*
*/

int64_t nano_tick_get(void)
{
	int64_t tmp_nano_ticks;
	/*
	 * Lock the interrupts when reading _nano_ticks 64-bit variable.
	 * Some architectures (x86) do not handle 64-bit atomically, so
	 * we have to lock the timer interrupt that causes change of
	 * _nano_ticks
	 */
	unsigned int imask = irq_lock_inline();
	tmp_nano_ticks = _nano_ticks;
	irq_unlock_inline(imask);
	return tmp_nano_ticks;
}

/*******************************************************************************
*
* nano_cycle_get_32 - return a high resolution timestamp
*
* RETURNS: the current timer hardware count
*
*/

uint32_t nano_cycle_get_32(void)
{
	return timer_read();
}

/*******************************************************************************
*
* nano_tick_delta - return number of ticks since a reference time
*
* This function is meant to be used in contained fragments of code. The first
* call to it in a particular code fragment fills in a reference time variable
* which then gets passed and updated every time the function is called. From
* the second call on, the delta between the value passed to it and the current
* tick count is the return value. Since the first call is meant to only fill in
* the reference time, its return value should be discarded.
*
* Since a code fragment that wants to use nano_tick_delta passes in its
* own reference time variable, multiple code fragments can make use of this
* function concurrently.
*
* e.g.
* uint64_t  reftime;
* (void) nano_tick_delta(&reftime);  /# prime it #/
* [do stuff]
* x = nano_tick_delta(&reftime);     /# how long since priming #/
* [do more stuff]
* y = nano_tick_delta(&reftime);     /# how long since [do stuff] #/
*
* RETURNS: tick count since reference time; undefined for first invocation
*
* NOTE: We use inline function for both 64-bit and 32-bit functions.
* Compiler optimizes out 64-bit result handling in 32-bit version.
*/

static ALWAYS_INLINE int64_t _nano_tick_delta(int64_t *reftime)
{
	int64_t  delta;
	int64_t  saved;

	/*
	 * Lock the interrupts when reading _nano_ticks 64-bit variable.
	 * Some architectures (x86) do not handle 64-bit atomically, so
	 * we have to lock the timer interrupt that causes change of
	 * _nano_ticks
	 */
	unsigned int imask = irq_lock_inline();
	saved = _nano_ticks;
	irq_unlock_inline(imask);
	delta = saved - (*reftime);
	*reftime = saved;

	return delta;
}

/*******************************************************************************
*
* nano_tick_delta - return number of ticks since a reference time
*
* RETURNS: tick count since reference time; undefined for first invocation
*/

int64_t nano_tick_delta(int64_t *reftime)
{
	return _nano_tick_delta(reftime);
}

/*******************************************************************************
*
* nano_tick_delta_32 - return 32-bit number of ticks since a reference time
*
* RETURNS: 32-bit tick count since reference time; undefined for first invocation
*/

uint32_t nano_tick_delta_32(int64_t *reftime)
{
	return (uint32_t)_nano_tick_delta(reftime);
}

#endif /*  CONFIG_NANOKERNEL */

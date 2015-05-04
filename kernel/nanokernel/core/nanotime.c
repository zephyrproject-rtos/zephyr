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

#ifndef CONFIG_TICKLESS_KERNEL
int sys_clock_us_per_tick = 1000000 / sys_clock_ticks_per_sec;
int sys_clock_hw_cycles_per_tick =
	sys_clock_hw_cycles_per_sec / sys_clock_ticks_per_sec;
#else
/* don't initialize to avoid division-by-zero error */
int sys_clock_us_per_tick;
int sys_clock_hw_cycles_per_tick;
#endif

uint32_t _nano_ticks = 0;
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
* nano_tick_get_32 - return the current system tick count
*
* RETURNS: the current system tick count
*
*/

uint32_t nano_tick_get_32(void)
{
	return _nano_ticks;
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
* nano_node_tick_delta - return number of ticks since a reference time
*
* This function is meant to be used in contained fragments of code. The first
* call to it in a particular code fragment fills in a reference time variable
* which then gets passed and updated every time the function is called. From
* the second call on, the delta between the value passed to it and the current
* tick count is the return value. Since the first call is meant to only fill in
* the reference time, its return value should be discarded.
*
* Since a code fragment that wants to use nano_node_tick_delta passes in its
* own reference time variable, multiple code fragments can make use of this
* function concurrently.
*
* e.g.
* uint64_t  reftime;
* (void) nano_node_tick_delta(&reftime);  /# prime it #/
* [do stuff]
* x = nano_node_tick_delta(&reftime);     /# how long since priming #/
* [do more stuff]
* y = nano_node_tick_delta(&reftime);     /# how long since [do stuff] #/
*
* RETURNS: tick count since reference time; undefined for first invocation
*/

uint32_t nano_node_tick_delta(uint64_t *reftime)
{
	uint32_t  delta;
	uint32_t  saved;

	saved = _nano_ticks;
	delta = saved - (uint32_t)(*reftime);
	*reftime = (uint64_t) saved;

	return delta;
}

#endif /*  CONFIG_NANOKERNEL */

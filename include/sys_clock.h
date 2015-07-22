/* Variables needed needed for system clock */

/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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

Declare variables used by both system timer device driver and kernel components
that use timer functionality.
 */

#ifndef _SYS_CLOCK__H_
#define _SYS_CLOCK__H_

#ifndef _ASMLANGUAGE
#include <stdint.h>

#if defined(CONFIG_SYS_CLOCK_EXISTS) && \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 0)
#error "SYS_CLOCK_HW_CYCLES_PER_SEC must be non-zero!"
#endif

#define sys_clock_ticks_per_sec CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define sys_clock_hw_cycles_per_sec CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

/*
 * sys_clock_us_per_tick global variable represents a number
 * of microseconds in one OS timer tick
 */
extern int sys_clock_us_per_tick;

/*
 * sys_clock_hw_cycles_per_tick global variable represents a number
 * of platform clock ticks in one OS timer tick.
 * sys_clock_hw_cycles_per_tick often represents a value of divider
 * of the board clock frequency
 */
extern int sys_clock_hw_cycles_per_tick;

/* number of nsec per usec */
#define NSEC_PER_USEC 1000

/* SYS_CLOCK_HW_CYCLES_TO_NS64 converts CPU clock cycles to nanoseconds */
#define SYS_CLOCK_HW_CYCLES_TO_NS64(X) \
	(((uint64_t)(X) * sys_clock_us_per_tick * NSEC_PER_USEC) / \
	 sys_clock_hw_cycles_per_tick)

/*
 * SYS_CLOCK_HW_CYCLES_TO_NS_AVG converts CPU clock cycles to nanoseconds
 * and calculates the average cycle time
 */
#define SYS_CLOCK_HW_CYCLES_TO_NS_AVG(X, NCYCLES) \
	(uint32_t)(SYS_CLOCK_HW_CYCLES_TO_NS64(X) / NCYCLES)

#define SYS_CLOCK_HW_CYCLES_TO_NS(X) (uint32_t)(SYS_CLOCK_HW_CYCLES_TO_NS64(X))

extern int64_t _nano_ticks;
extern struct nano_timer *_nano_timer_list;

#endif /* !_ASMLANGUAGE */

#endif /* _SYS_CLOCK__H_ */

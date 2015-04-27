/* ticks.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
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

#include <microkernel/k_struct.h>
#include <minik.h>
#include <kticks.h> /* WL and timeslice stuff */
#include <ktask.h>  /* K_yield */
#include <microkernel/ticks.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>

#ifndef LITE
K_TIMER  *_k_timer_list_head = NULL;
K_TIMER  *_k_timer_list_tail = NULL;
#endif

int64_t _k_sys_clock_tick_count = 0;

/* these two access routines can be removed if atomic operators are
 * functional on all platforms */
void _LowTimeInc(int inc)
{
	int key = irq_lock_inline();

	_k_sys_clock_tick_count += inc;
	irq_unlock_inline(key);
}

int64_t _LowTimeGet(void)
{
	int64_t ticks;
	int key = irq_lock_inline();
	ticks = _k_sys_clock_tick_count;

	irq_unlock_inline(key);
	return ticks;
}

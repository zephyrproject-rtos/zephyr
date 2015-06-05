/* advidle.h - header file for custom advanced idle manager */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
This header file specifies the custom advanced idle management interface.
All of the APIs declared here must be supplied by the custom advanced idle
management system, namely the _AdvIdleCheckSleep(), _AdvIdleFunc()
and _AdvIdleStart() functions.
 */

#ifndef __INCadvidle
#define __INCadvidle

#ifdef CONFIG_ADVANCED_IDLE

/*
 * _AdvIdleCheckSleep - determine if advanced sleep has occurred
 *
 * This routine checks if the system is recovering from advanced
 * sleep or cold booting.
 *
 * RETURNS: 0 if the system is cold booting on a non-zero
 * value if the system is recovering from advanced sleep.
 */

extern int _AdvIdleCheckSleep(void);

/*
 * _AdvIdleStart - continue kernel start-up or awaken kernel from sleep
 *
 * This routine checks if the system is recovering from advanced sleep and
 * either continues the kernel's cold boot sequence at _Cstart or resumes
 * kernel operation at the point it went to sleep; in the latter case, control
 * passes to the _AdvIdleFunc() that put the system to sleep, which then
 * finishes executing.
 *
 * RETURNS: does not return to caller
 */

extern void _AdvIdleStart(
	void (*_Cstart)(void), /* addr of _Cstart function */
	void *_gdt,	    /* addr of global descriptor table in RAM */
	void *_GlobalTss       /* addr of TSS descriptor */
	);

/*
 * _AdvIdleFunc - perform advanced sleep
 *
 * This routine checks if the upcoming kernel idle interval is sufficient to
 * justify entering advanced sleep mode. If it is, the routine puts the system
 * to sleep and then later allows it to resume processing; if not, the routine
 * returns immediately without sleeping.
 *
 * RETURNS:  non-zero if advanced sleep occurred; otherwise zero
 */

extern int _AdvIdleFunc(int32_t ticks /* upcoming kernel idle time */
			);

#endif /* CONFIG_ADVANCED_IDLE */

#endif /* __INCadvidle */

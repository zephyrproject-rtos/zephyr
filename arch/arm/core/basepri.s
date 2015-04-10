/* basepri.s - ARM Cortex-M interrupt locking via BASEPRI */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

Provide irq_lock() and irq_unlock() via the BASEPRI register. This
allows locking up to a certain interrupt priority. VxMicro locks out priorities
2 and lower (higher numbered priorities), in essence leaving priorities 0 and 1
unlocked. This achieves two purposes:

1. The service call exception is installed at priority 0, allowing it to be
   invoked with interrupts locked. This is needed since 'svc #0' is the
   implementation of _Swap(), which is invoked with interrupts locked in the
   common implementation of nanokernel objects.

2. Zero Interrupt Latency (ZLI) is achievable via this by allowing certain
   interrupts to set their priority to 1, thus being allowed in when interrupts
   are locked for regular interrupts.
*/

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>
#include <nanokernel/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(irq_lock)
GTEXT(irq_unlock)

/*******************************************************************************
*
* irq_lock - lock interrupts
*
* Prevent exceptions of priority lower than to the two highest priorities from
* interrupting the CPU.
*
* This function can be called recursively: it will return a key to return the
* state of interrupt locking to the previous level.
*
* RETURNS: a key to return to the previous interrupt locking level
*/

SECTION_FUNC(TEXT,irq_lock)
    movs.n r1, #_EXC_IRQ_DEFAULT_PRIO
    mrs r0, BASEPRI
    msr BASEPRI, r1
    bx lr

/*******************************************************************************
*
* irq_unlock - unlock interrupts
*
* Return the state of interrupt locking to a previous level, passed in via the
* <key> parameter, obtained from a previous call to irq_lock().
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT,irq_unlock)
    msr BASEPRI, r0
    bx lr

    .end

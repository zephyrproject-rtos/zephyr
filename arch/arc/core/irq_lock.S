/* irq_lock.S - interrupt locking */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

#ifndef _IRQ_LOCK__H_
#define _IRQ_LOCK__H_

#define _ASMLANGUAGE

#include <nano_private.h>
#include <offsets.h>
#include <toolchain.h>
#include <arch/cpu.h>

/*******************************************************************************
*
* irq_lock - disable all interrupts on the local CPU
*
* This routine disables interrupts.  It can be called from either interrupt,
* task or fiber level.  This routine returns an architecture-dependent
* lock-out key representing the "interrupt disable state" prior to the call;
* this key can be passed to irq_unlock() to re-enable interrupts.
*
* The lock-out key should only be used as the argument to the
* irq_unlock() API.  It should never be used to manually re-enable
* interrupts or to inspect or manipulate the contents of the source register.
*
* WARNINGS
* Invoking a kernel routine with interrupts locked may result in
* interrupts being re-enabled for an unspecified period of time.  If the
* called routine blocks, interrupts will be re-enabled while another
* context executes, or while the system is idle.
*
* The "interrupt disable state" is an attribute of a context.  Thus, if a
* fiber or task disables interrupts and subsequently invokes a kernel
* routine that causes the calling context to block, the interrupt
* disable state will be restored when the context is later rescheduled
* for execution.
*
* RETURNS: An architecture-dependent lock-out key representing the
* "interrupt disable state" prior to the call.
*
* \NOMANUAL
*/

SECTION_FUNC(TEXT, irq_lock)
	j_s.d [blink]
	clri r0

/*******************************************************************************
*
* irq_unlock - enable all interrupts on the local CPU
*
* This routine re-enables interrupts on the local CPU.  The <key> parameter
* is an architecture-dependent lock-out key that is returned by a previous
* invocation of irq_lock().
*
* This routine can be called from either interrupt, task or fiber level.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

SECTION_FUNC(TEXT, irq_unlock)
	j_s.d [blink]
	seti r0

#endif /* _IRQ_LOCK__H_ */

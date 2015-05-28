/* intboiexit.s - VxMicro spurious interrupt support for IA-32 architecture */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
This module implements the single interrupt handling routine _IntBoiExit().
This routine is used by some interrupt controller drivers (e.g. the Intel
8259A driver) to short circuit the execution of normal interrupt stub
processing when a spurious interrupt is detected.

When a spurious interrupt condition is detected by the interrupt controller's
"beginning of interrupt" (BOI) handler it forces a return to _IntBoiExit()
rather than returning back to the interrupt stub.  The _IntBoiExit() routine
then pops the parameter passed to the BOI handler and branches to _IntExit(),
thereby circumventing execution of the "application" ISR and the interrupt
controller driver's "end of interrupt" (EOI) handler (if present).

\INTERNAL
The _IntBoiExit() routine is provided in a separate module so that it gets
included in the final image only if an interrupt controller driver utilizing
_IntBoiExit() is present.

*/

#define _ASMLANGUAGE
#include <arch/x86/asm.h>
#include <offsets.h>		/* nanokernel structure offset definitions */


	/* exports (internal APIs) */

	GTEXT(_IntBoiExit)

 	/* externs */

	GTEXT(_IntExit)

/*******************************************************************************
*
* _IntBoiExit - exit interrupt handler stub without invoking ISR
*
* This routine exits an interrupt handler stub without invoking the associated
* ISR handler (or the EOI handler, if present).  It should only be jumped to
* by an interrupt controller driver's BOI routine, and only if the BOI routine
* is passed a single parameter by the interrupt stub.
*
* \INTERNAL
* A BOI routine that has no parameters can jump directly to _IntExit().
*/

SECTION_FUNC(TEXT, _IntBoiExit)
        addl    $4, %esp		/* pop off the $BoiParameter */
        jmp     FUNC(_IntExit)		/* exit via kernel */

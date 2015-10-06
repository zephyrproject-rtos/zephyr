/* intboiexit.S - spurious interrupt support for IA-32 architecture */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

/**
 *
 * @brief Exit interrupt handler stub without invoking ISR
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

/* test_stubs.S - Exception and interrupt stubs */

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
This module implements assembler exception and interrupt stubs for regression
testing.
*/

#define _ASMLANGUAGE

#ifdef CONFIG_ISA_IA32

/* IA-32 specific */

#include <arch/cpu.h>
#include <nano_private.h>
#include <arch/x86/asm.h>
#include <asmPrv.h>
#include <asm_inline.h>

	/* exports (internal APIs) */

	GTEXT(_ExcEnt)
	GTEXT(_ExcExit)
	GTEXT(_IntEnt)
	GTEXT(_IntExit)


/* Static exception handler stubs */
SYS_NANO_CPU_EXC_CONNECT(exc_divide_error_handler,IV_DIVIDE_ERROR)

/* Static interrupt handler stubs */


	GTEXT(nanoIntStub)

SECTION_FUNC(TEXT, nanoIntStub)
        call    _IntEnt
        pushl   $0
        call    isr_handler
        addl    $4, %esp
        jmp     _IntExit

#else

#error Arch not supported

#endif /* CONFIG_ISA_IA32 */

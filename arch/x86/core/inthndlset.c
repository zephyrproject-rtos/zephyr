/* inthndlset.c - interrupt management support for IA-32 arch */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * This module contains the irq_handler_set() API. This routine is closely
 * associated with irq_connect(), and any changes to the layout of the
 * constructed interrupt stub must be reflected in both places.
 *
 * INTERNAL
 * This routine is defined here, rather than in intconnect.c, so that it can be
 * omitted from a system image if it isn't required.
 */


#include <nano_private.h>


/* the _idt_base_address symbol is generated via a linker script */

extern unsigned char _idt_base_address[];

/*
 * The FIRST_OPT_OPCODE_OFF macro defines the offset of the first optional
 * opcode in an interrupt stub.  Given that only the "call _IntEnt" is
 * mandatory, the subsequent instruction at offset 5 is "optional".
 */

#define FIRST_OPT_OPCODE_OFF 5

/**
 *
 * @brief Set the handler in an already connected stub
 *
 * This routine is used to modify an already fully constructed interrupt stub
 * to specify a new <routine> and/or <parameter>.
 *
 * WARNINGS:
 *
 * A fully constructed interrupt stub is generated via irq_connect(), i.e.
 * the irq_handler_set() function must only be called after invoking
 * irq_connect().
 *
 * The caller must ensure that the associated interrupt does not occur while
 * this routine is executing, otherwise race conditions may arise that could
 * cause the interrupt stub to invoke the handler using an incorrect routine
 * and/or parameter. If possible, silence the source of the associated interrupt
 * only, rather than locking out all interrupts.
 *
 * @return N/A
 *
 */

void irq_handler_set(unsigned int vector,
					 void (*oldRoutine)(void *parameter),
					 void (*newRoutine)(void *parameter),
					 void *parameter)
{
	unsigned int ix =
		FIRST_OPT_OPCODE_OFF; /* call _IntEnt is not optional */
	unsigned int *pIdtEntry;
	unsigned char *pIntStubMem;

	pIdtEntry = (unsigned int *)(_idt_base_address + (vector << 3));
	pIntStubMem = (unsigned char *)(((uint16_t)pIdtEntry[0]) |
					(pIdtEntry[1] & 0xffff0000));

	/*
	 * Given the generation of the stub is dynamic, i.e. the invocations of
	 * an EOI routine (with parameter) and/or BOI routine (with parameter)
	 * are optional based on the requirements of the interrupt controller,
	 * the <oldRoutine> parameter is used to quickly find the correct
	 * bytes in the stub code to update.
	 */

	while (ix < _INT_STUB_SIZE) {
		/* locate the call opcode */

		if (pIntStubMem[ix] == IA32_CALL_OPCODE) {
			unsigned int opcodeOffToMatch;
			unsigned int opcodeOff;

			/* read the offset encoded in the call opcode */

			opcodeOff = UNALIGNED_READ(
				(unsigned int *)&pIntStubMem[ix + 1]);
			opcodeOffToMatch = (unsigned int)oldRoutine -
					   (unsigned int)&pIntStubMem[ix + 5];

			/* does the encoded offset match <oldRoutine> ? */

			if (opcodeOff == opcodeOffToMatch) {
				/* match found -> write new routine and parameter */

				UNALIGNED_WRITE(
					(unsigned int *)&pIntStubMem[ix + 1],
					(unsigned int)newRoutine -
						(unsigned int)&pIntStubMem[ix +
									   5]);

				UNALIGNED_WRITE(
					(unsigned int *)&pIntStubMem[ix - 4],
					(unsigned int)parameter);


				return; /* done */
			}
		}

		/* all instructions in the stub are 5 bytes long */

		ix += 5;
	}
}


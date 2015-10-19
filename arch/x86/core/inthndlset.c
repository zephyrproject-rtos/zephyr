/* inthndlset.c - interrupt management support for IA-32 arch */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * This module contains the _irq_handler_set() API. This routine is closely
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
 * @internal
 *
 * @brief Set the handler in an already connected stub
 *
 * This routine is used to modify an already fully constructed interrupt stub
 * to specify a new <routine> and/or <parameter>.
 *
 * WARNINGS:
 *
 * A fully constructed interrupt stub is generated via irq_connect(), i.e.
 * the _irq_handler_set() function must only be called after invoking
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

void _irq_handler_set(unsigned int vector,
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
	 * an EOI routine (with parameter) are optional based on the
	 * requirements of the interrupt controller,
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


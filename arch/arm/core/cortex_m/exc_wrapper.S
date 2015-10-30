/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

/**
 * @file dynamic exception handler wrapper
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GDATA(_sw_exc_table)
GTEXT(_exc_wrapper)

SECTION_FUNC(TEXT, _exc_wrapper)

	_GDB_STUB_EXC_ENTRY

	ldr ip, =_SCS_ICSR
	ldr ip, [ip]
	ands.w ip, #_SCS_ICSR_RETTOBASE

	itte eq			/* is the RETTOBASE bit zero ? */
	mrseq r0, MSP	/* if so, we're not returning to thread mode, thus this
					 * is a nested exception: the stack frame is on the MSP */
	addeq.w r0, #4	/* ESF returned here is off by 4 because of the push {r2}
					 * in_GDB_STUB_EXC_ENTRY */
	mrsne r0, PSP	/* if not, we are returning to thread mode, thus this is
					 * not a nested exception: the stack frame is on the PSP */

	/* r0 now contains the pointer to the full ESF */

	push {lr}	/* lr is now the first item on the stack */

	ldr r1, =_sw_exc_table

	mrs r2, IPSR	/* get exception number */

	/* find and call handler: table is 4-bytes wide, shift index by 2 */
	ldr r1, [r1, r2, LSL #2]
	blx r1

	pop {lr}

	/* exception return is done in _ExcExit(), including _GDB_STUB_EXC_EXIT */
	b _ExcExit

	.end

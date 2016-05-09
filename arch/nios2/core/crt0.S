/*
 * Copyright (c) 2016 Intel Corporation
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

#define _ASMLANGUAGE
#include <arch/nios2/asm.h>

/* exports */
GTEXT(__start)
GTEXT(__text_start)

/* imports */
GTEXT(_PrepC)
GTEXT(_interrupt_stack)

	/* Allow use of r1/at (the assembler temporary register) in this
	 * code, normally reserved for internal assembler use
	 */
	.set noat

/*
 * Reset vector entry point into the system. Placed into special 'reset'
 * section so that the linker puts this at ALT_CPU_RESET_ADDR defined in
 * system.h
 *
 * This code can be at most 0x20 bytes, since the exception vector for Nios II
 * is usually configured to be 0x20 past the reset vector.
 */
SECTION_FUNC(reset, __start)

	/* TODO initialize instruction cache, if present
	 * ZEP-275
	 */

	/* Done all we need to do here, jump to __text_start */
	movhi r1, %hi(__text_start)
	ori r1, r1, %lo(__text_start)
	jmp r1

/* Remainder of asm-land initialization code before we can jump into
 * the C domain
 */
SECTION_FUNC(TEXT, __text_start)

	/* TODO if shadow register sets enabled, ensure we are in set 0
	 * ZEP-258
	 */

	/* TODO initialize data cache, if present
	 * ZEP-275
	 */

	/* Set up the initial stack pointer
	 * ZEP-269
	 */

	/* TODO Setup the global pointer
	 * ZEP-272
	 */

	/* TODO if shadow register sets enabled, interate through them to set
	 * up. Need to clear r0, write gp, set the execption stack pointer
	 * ZEP-258
	 */

	/* Jump into C domain. _PrepC zeroes BSS, copies rw data into RAM,
	 * and then enters nanokernel _Cstart */
	call _PrepC


/*
 * Copyright (c) 2015 Intel Corporation.
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

#include <toolchain.h>
#include <sections.h>
#include <nano_private.h>
#include <arch/x86/asm.h>
#include <offsets.h>	/* nanokernel structure offset definitions */
#include <arch/cpu.h>	/* _NANO_ERR_SPURIOUS_INT */


GTEXT(_thread_entry_wrapper)
GTEXT(_thread_entry)
GTEXT(_irq_sw_handler)

/*
 * @brief Wrapper for _thread_entry
 *
 * The routine pops parameters for the _thread_entry from stack frame, prepared
 * by the _new_thread() routine.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _thread_entry_wrapper)
	popl %eax
	popl %edx
	popl %ecx
	jmp _thread_entry


#if CONFIG_IRQ_OFFLOAD
SECTION_FUNC(TEXT, _irq_sw_handler)
	pushl %eax
	pushl %edx
	pushl %ecx
	movl $_irq_do_offload, %eax
	call _execute_handler
	pop %ecx
	pop %edx
	pop %eax
	iret
#endif

#if ALL_DYN_IRQ_STUBS > 0
BRANCH_LABEL(_DynIntStubCommon)
	pushl %eax
	pushl %ecx
	movl $_common_dynamic_irq_handler, %eax
	call _execute_handler
	/* Clean up and call IRET */
	pop %ecx
	pop %eax
	pop %edx
	iret


/* Create all the dynamic IRQ stubs
 *
 * NOTE: Please update DYN_STUB_SIZE in include/arch/x86/arch.h if you change
 * how large the generated stubs are, otherwise _get_dynamic_stub() will
 * be unable to correctly determine the offset
 */

/*
 * Create nice labels for all the stubs so we can see where we
 * are in a debugger
 */
.altmacro
.macro __INT_STUB_NUM id
BRANCH_LABEL(_DynIntStub\id)
.endm
.macro INT_STUB_NUM id
__INT_STUB_NUM %id
.endm
GTEXT(_DynIntStubsBegin)
SECTION_FUNC(TEXT, _DynIntStubsBegin)
stub_num = 0

.rept ((ALL_DYN_IRQ_STUBS + DYN_STUB_PER_BLOCK - 1) / DYN_STUB_PER_BLOCK)
	block_counter = 0
	.rept DYN_STUB_PER_BLOCK
		.if stub_num < ALL_DYN_IRQ_STUBS
			INT_STUB_NUM stub_num

			pushl %edx


			/*
			 * 2-byte push imm8. Consumed by
			 * common_dynamic_handler(), see intconnect.c
			 */
			movl $stub_num, %edx

			/*
			 * Check to make sure this isn't the last stub in
			 * a block, in which case we just fall through
			 */
			.if (block_counter <> (DYN_STUB_PER_BLOCK - 1) && \
			     (stub_num <> ALL_DYN_IRQ_STUBS - 1))
				/* This should always be a 2-byte jmp rel8 */
				jmp 1f
			.endif
			stub_num = stub_num + 1
			block_counter = block_counter + 1
		.endif
	.endr
	/*
	 * This must a 5-bvte jump rel32, which is why _DynStubCommon
	 * is before the actual stubs
	 */
1:	jmp _DynIntStubCommon
.endr
#endif /* ALL_DYN_IRQ_STUBS */


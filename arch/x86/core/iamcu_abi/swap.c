/*
 * Copyright (c) 2015 Intel Corporation
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

#include <toolchain.h>
#include <sections.h>
#include <nano_private.h>

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
extern void _sys_k_event_logger_context_switch(void);
#else
#define _sys_k_event_logger_context_switch()
#endif

/* Stack protector disabled here; we switch stacks, so the sentinel
 * placed by the stack protection code isn't there when it checks for it
 * at the end of the function
 */
unsigned int __attribute__((optimize("-fno-stack-protector")))
_Swap(unsigned int eflags)
{
	struct tcs *next;
	int rv;

	/* Save the current context onto the stack */
	__asm__ volatile("pushl	%eax\n\t" /* push eflags _Swap argumet*/
			 "pushl	%edi\n\t"
			 "pushl	%esi\n\t"
			 "pushl	%ebx\n\t"
			 "pushl	%ebp\n\t"
			 "pushl	%ebx\n\t"); /* eax slot for fiber return */

	/* save the stack pointer into the current context structure */
	__asm__ volatile("mov %%esp, %0"
			 :"=m" (_nanokernel.current->coopReg.esp));

	_sys_k_event_logger_context_switch();

	/* find the next context to run */
	if (_nanokernel.fiber) {
		next = _nanokernel.fiber;
		_nanokernel.fiber = (struct tcs *)_nanokernel.fiber->link;
	} else {
		next = _nanokernel.task;
	}
	_nanokernel.current = next;

	/* recover the stack pointer for the incoming context */
	__asm__ volatile("mov %0, %%esp"
			 :
			 :"m" (next->coopReg.esp));

	/* restore the context */
	__asm__ volatile("popl %eax\n\t"
			 "popl %ebp\n\t"
			 "popl %ebx\n\t"
			 "popl %esi\n\t"
			 "popl %edi\n\t"
			 "popfl \n\t"
			 );

	/* The correct return value is already in eax but this makes
	 * the compiler happy
	 */
	__asm__ volatile("mov %%eax, %0"
			 :"=m" (rv)
			 );
	return rv;
}

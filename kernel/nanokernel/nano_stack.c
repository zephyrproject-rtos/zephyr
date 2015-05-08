/* nanokernel fixed-size stack object */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
This module provides the VxMicro nanokernel (aka system-level) 'stack'
implementation.  This module provides the backing implementation for the
following APIs:

   nano_stack_init
   nano_fiber_stack_push, nano_task_stack_push, nano_isr_stack_push
   nano_fiber_stack_pop, nano_task_stack_pop, nano_isr_stack_pop
   nano_fiber_stack_pop_wait, nano_task_stack_pop_wait


INTERNAL
In some cases the compiler "alias" attribute is used to map two or more
APIs to the same function, since they have identical implementations.
*/

#include <nanok.h>
#include <toolchain.h>
#include <sections.h>

/*******************************************************************************
*
* nano_stack_init - initialize a nanokernel stack object
*
* This function initializes a nanokernel stack object structure.
*
* It may be called from either a fiber or a task context.
*
* RETURNS: N/A
*
* INTERNAL
* Although the existing implementation will support invocation from an ISR
* context, for future flexibility, this API will be restricted from ISR
* level invocation.
*/

void nano_stack_init(
	struct nano_stack *stack,	/* stack to initialize */
	uint32_t *data				/* container for stack */
	)
{
	stack->next = stack->base = data;
	stack->proc = (tCCS *)0;
}

#ifdef CONFIG_MICROKERNEL

/*
 * For legacy reasons, the microkernel utilizes the _Cpsh() API which is
 * functionally equivalent to nano_fiber_stack_pushC() (which has been renamed
 * nano_fiber_stack_push() given that, by default, APIs will be a C
 * interface), an an alias will be generated.
 */

FUNC_ALIAS(_stack_push_non_preemptible, _Cpsh, void);
#endif /* CONFIG_MICROKERNEL */

FUNC_ALIAS(_stack_push_non_preemptible, nano_isr_stack_push, void);
FUNC_ALIAS(_stack_push_non_preemptible, nano_fiber_stack_push, void);

/*******************************************************************************
*
* _stack_push_non_preemptible - push data onto a stack (no context switch)
*
* This routine pushes a data item onto a stack object; it may be called from
* either a fiber or ISR context.  A fiber pending on the stack object will be
* made ready, but will NOT be scheduled to execute.
*
* RETURNS: N/A
*
* INTERNAL
* This function is capable of supporting invocations from both a fiber and an
* ISR context.  However, the nano_isr_stack_push and nano_fiber_stack_push
* aliases are created to support any required implementation differences in
* the future without introducing a source code migration issue.
*/

void _stack_push_non_preemptible(
	struct nano_stack *stack, /* stack on which to interact */
	uint32_t data /* data to push on stack */
	)
{
	tCCS *ccs;
	unsigned int imask;

	imask = irq_lock_inline();

	ccs = stack->proc;
	if (ccs) {
		stack->proc = 0;
		fiberRtnValueSet(ccs, data);
		_insert_ccs((tCCS **)&_NanoKernel.fiber, ccs);
	} else {
		*(stack->next) = data;
		stack->next++;
	}

	irq_unlock_inline(imask);
}

/*******************************************************************************
*
* nano_task_stack_push - push data onto a nanokernel stack
*
* This routine pushes a data item onto a stack object; it may be called only
* from a task context.  A fiber pending on the stack object will be
* made ready, and will preempt the running task immediately.
*
* RETURNS: N/A
*/

void nano_task_stack_push(
	struct nano_stack *stack, /* stack on which to interact */
	uint32_t data     /* data to push on stack */
	)
{
	tCCS *ccs;
	unsigned int imask;

	imask = irq_lock_inline();

	ccs = stack->proc;
	if (ccs) {
		stack->proc = 0;
		fiberRtnValueSet(ccs, data);
		_insert_ccs((tCCS **)&_NanoKernel.fiber, ccs);
		_Swap(imask);
		return;
	} else {
		*(stack->next) = data;
		stack->next++;
	}

	irq_unlock_inline(imask);
}

FUNC_ALIAS(_stack_pop, nano_isr_stack_pop, int);
FUNC_ALIAS(_stack_pop, nano_fiber_stack_pop, int);
FUNC_ALIAS(_stack_pop, nano_task_stack_pop, int);

/*******************************************************************************
*
* _stack_pop - pop data from a nanokernel stack
*
* Pop the first data word from a nanokernel stack object; it may be called
* from a fiber, task, or ISR context.
*
* If the stack is not empty, a data word is popped and copied to the provided
* address <pData> and a non-zero value is returned. If the stack is empty,
* zero is returned.
*
* RETURNS: 1 if stack is not empty, 0 otherwise
*
* INTERNAL
* This function is capable of supporting invocations from fiber, task, and
* ISR contexts.  However, the nano_isr_stack_pop, nano_task_stack_pop, and
* nano_fiber_stack_pop aliases are created to support any required
* implementation differences in the future without intoducing a source code
* migration issue.
*/

int _stack_pop(
	struct nano_stack *stack,	/* stack on which to interact */
	uint32_t *pData				/* container for data to pop */
	)
{
	unsigned int imask;
	int rv = 0;

	imask = irq_lock_inline();

	if (stack->next > stack->base) {
		stack->next--;
		*pData = *(stack->next);
		rv = 1;
	}

	irq_unlock_inline(imask);
	return rv;
}

/*******************************************************************************
*
* nano_fiber_stack_pop_wait - pop data from a nanokernel stack, wait if empty
*
* Pop the first data word from a nanokernel stack object; it can only be
* called from a fiber context
*
* If data is not available the calling fiber will pend until data is pushed
* onto the stack.
*
* RETURNS: the data popped from the stack
*
* INTERNAL
* There exists a separate nano_task_stack_pop_wait() implementation since a
* task context cannot pend on a nanokernel object. Instead tasks will poll the
* the stack object.
*/

uint32_t nano_fiber_stack_pop_wait(
	struct nano_stack *stack /* stack on which to interact */
	)
{
	uint32_t data;
	unsigned int imask;

	imask = irq_lock_inline();

	if (stack->next == stack->base) {
		stack->proc = _NanoKernel.current;
		data = (uint32_t)_Swap(imask);
	} else {
		stack->next--;
		data = *(stack->next);
		irq_unlock_inline(imask);
	}

	return data;
}

/*******************************************************************************
*
* nano_task_stack_pop_wait - pop data from a nanokernel stack, poll if empty
*
* Pop the first data word from a nanokernel stack; it can only be called
* from a task context.
*
* If data is not available the calling task will poll until data is pushed
* onto the stack.
*
* RETURNS: the data popped from the stack
*/

uint32_t nano_task_stack_pop_wait(
	struct nano_stack *stack /* stack on which to interact */
	)
{
	uint32_t data;
	unsigned int imask;

	/* spin until data is pushed onto the stack */

	while (1) {
		imask = irq_lock_inline();

		/*
		 * Predict that the branch will be taken to break out of the loop.
		 * There is little cost to a misprediction since that leads to idle.
		 */

		if (likely(stack->next > stack->base))
			break;

		/*
		 * Invoke nano_cpu_atomic_idle() with interrupts still disabled to
		 * prevent the scenario where an interrupt fires after re-enabling
		 * interrupts and before executing the "halt" instruction.  If the ISR
		 * performs a nano_isr_stack_push() on the same stack object, the
		 * subsequent execution of the "halt" instruction will result in the
		 * queued data being ignored until the next interrupt, if any.
		 *
		 * Thus it should be clear that an architectures implementation
		 * of nano_cpu_atomic_idle() must be able to atomically re-enable
		 * interrupts and enter a low-power mode.
		 *
		 * This explanation is valid for all nanokernel objects: stacks, FIFOs,
		 * LIFOs, and semaphores, for their nano_task_<object>_<get>_wait()
		 * routines.
		 */

		nano_cpu_atomic_idle(imask);
	}

	stack->next--;
	data = *(stack->next);

	irq_unlock_inline(imask);

	return data;
}

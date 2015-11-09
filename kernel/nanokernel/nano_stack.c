/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
 * @brief Nanokernel fixed-size stack object
 *
 * This module provides the nanokernel stack object implementation, including
 * the following APIs:
 *
 *  nano_stack_init
 *  nano_fiber_stack_push, nano_task_stack_push, nano_isr_stack_push
 *  nano_fiber_stack_pop, nano_task_stack_pop, nano_isr_stack_pop
 *  nano_fiber_stack_pop_wait, nano_task_stack_pop_wait
 *
 * @param stack the stack to initialize
 * @param data pointer to the container for the stack
 *
 * @internal
 * In some cases the compiler "alias" attribute is used to map two or more
 * APIs to the same function, since they have identical implementations.
 * @endinternal
 *
 */

#include <nano_private.h>
#include <toolchain.h>
#include <sections.h>


void nano_stack_init(struct nano_stack *stack, uint32_t *data)
{
	stack->next = stack->base = data;
	stack->fiber = (struct tcs *)0;
}

FUNC_ALIAS(_stack_push_non_preemptible, nano_isr_stack_push, void);
FUNC_ALIAS(_stack_push_non_preemptible, nano_fiber_stack_push, void);

/**
 *
 * @brief Push data onto a stack (no context switch)
 *
 * This routine pushes a data item onto a stack object; it may be called from
 * either a fiber or ISR context.  A fiber pending on the stack object will be
 * made ready, but will NOT be scheduled to execute.
 *
 * @param stack Stack on which to interact
 * @param data Data to push on stack
 * @return N/A
 *
 * @internal
 * This function is capable of supporting invocations from both a fiber and an
 * ISR context.  However, the nano_isr_stack_push and nano_fiber_stack_push
 * aliases are created to support any required implementation differences in
 * the future without introducing a source code migration issue.
 * @endinternal
 */
void _stack_push_non_preemptible(struct nano_stack *stack, uint32_t data)
{
	struct tcs *tcs;
	unsigned int imask;

	imask = irq_lock();

	tcs = stack->fiber;
	if (tcs) {
		stack->fiber = 0;
		fiberRtnValueSet(tcs, data);
		_nano_fiber_schedule(tcs);
	} else {
		*(stack->next) = data;
		stack->next++;
	}

	irq_unlock(imask);
}


void nano_task_stack_push(struct nano_stack *stack, uint32_t data)
{
	struct tcs *tcs;
	unsigned int imask;

	imask = irq_lock();

	tcs = stack->fiber;
	if (tcs) {
		stack->fiber = 0;
		fiberRtnValueSet(tcs, data);
		_nano_fiber_schedule(tcs);
		_Swap(imask);
		return;
	}

	*(stack->next) = data;
	stack->next++;

	irq_unlock(imask);
}

void nano_stack_push(struct nano_stack *stack, uint32_t data)
{
	static void (*func[3])(struct nano_stack *, uint32_t) = {
		nano_isr_stack_push,
		nano_fiber_stack_push,
		nano_task_stack_push
	};

	func[sys_execution_context_type_get()](stack, data);
}

FUNC_ALIAS(_stack_pop, nano_isr_stack_pop, int);
FUNC_ALIAS(_stack_pop, nano_fiber_stack_pop, int);
FUNC_ALIAS(_stack_pop, nano_task_stack_pop, int);
FUNC_ALIAS(_stack_pop, nano_stack_pop, int);

/**
 *
 * @brief Pop data from a nanokernel stack
 *
 * Pop the first data word from a nanokernel stack object; it may be called
 * from a fiber, task, or ISR context.
 *
 * If the stack is not empty, a data word is popped and copied to the provided
 * address <pData> and a non-zero value is returned. If the stack is empty,
 * zero is returned.
 *
 * @param stack Stack to operate on
 * @param pData Container for data to pop
 *
 * @return 1 if stack is not empty, 0 otherwise
 *
 * @internal
 * This function is capable of supporting invocations from fiber, task, and
 * ISR contexts.  However, the nano_isr_stack_pop, nano_task_stack_pop, and
 * nano_fiber_stack_pop aliases are created to support any required
 * implementation differences in the future without intoducing a source code
 * migration issue.
 * @endinternal
 */
int _stack_pop(struct nano_stack *stack, uint32_t *pData)
{
	unsigned int imask;
	int rv = 0;

	imask = irq_lock();

	if (stack->next > stack->base) {
		stack->next--;
		*pData = *(stack->next);
		rv = 1;
	}

	irq_unlock(imask);
	return rv;
}

/**
 * @brief Pop data from a nanokernel stack, wait if empty
 *
 * @internal
 * There exists a separate nano_task_stack_pop_wait() implementation since a
 * task cannot pend on a nanokernel object. Instead tasks will poll the
 * the stack object.
 * @endinternal
 */
uint32_t nano_fiber_stack_pop_wait(struct nano_stack *stack)
{
	uint32_t data;
	unsigned int imask;

	imask = irq_lock();

	if (stack->next == stack->base) {
		stack->fiber = _nanokernel.current;
		data = (uint32_t)_Swap(imask);
	} else {
		stack->next--;
		data = *(stack->next);
		irq_unlock(imask);
	}

	return data;
}


uint32_t nano_task_stack_pop_wait(struct nano_stack *stack)
{
	uint32_t data;
	unsigned int imask;

	/* spin until data is pushed onto the stack */

	while (1) {
		imask = irq_lock();

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

	irq_unlock(imask);

	return data;
}

uint32_t nano_stack_pop_wait(struct nano_stack *stack)
{
	static uint32_t (*func[3])(struct nano_stack *) = {
		NULL,
		nano_fiber_stack_pop_wait,
		nano_task_stack_pop_wait,
	};

	return func[sys_execution_context_type_get()](stack);
}

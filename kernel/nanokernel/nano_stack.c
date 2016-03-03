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
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>

void nano_stack_init(struct nano_stack *stack, uint32_t *data)
{
	stack->next = stack->base = data;
	stack->fiber = (struct tcs *)0;
	SYS_TRACING_OBJ_INIT(nano_stack, stack);
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
		_nano_fiber_ready(tcs);
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
		_nano_fiber_ready(tcs);
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

/**
 *
 * @brief Pop data from a nanokernel stack
 *
 * Pop the first data word from a nanokernel stack object; it may be called
 * from either a fiber or ISR context.
 *
 * If the stack is not empty, a data word is popped and copied to the provided
 * address <pData> and a non-zero value is returned. If the stack is empty,
 * it waits until data is ready.
 *
 * @param stack Stack to operate on
 * @param pData Container for data to pop
 * @param timeout_in_ticks Affects the action taken should the stack be empty.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. No other value is currently supported as this routine
 * does not support CONFIG_NANO_TIMEOUTS.
 *
 * @return 1 popped data from the stack; 0 otherwise
 */
int _stack_pop(struct nano_stack *stack, uint32_t *pData, int32_t timeout_in_ticks)
{
	unsigned int imask;

	imask = irq_lock();

	if (likely(stack->next > stack->base)) {
		stack->next--;
		*pData = *(stack->next);
		irq_unlock(imask);
		return 1;
	}

	if (timeout_in_ticks != TICKS_NONE) {
		stack->fiber = _nanokernel.current;
		*pData = (uint32_t) _Swap(imask);
		return 1;
	}

	irq_unlock(imask);
	return 0;
}

int nano_task_stack_pop(struct nano_stack *stack, uint32_t *pData, int32_t timeout_in_ticks)
{
	unsigned int imask;

	imask = irq_lock();

	while (1) {
		/*
		 * Predict that the branch will be taken to break out of the
		 * loop.  There is little cost to a misprediction since that
		 * leads to idle.
		 */

		if (likely(stack->next > stack->base)) {
			stack->next--;
			*pData = *(stack->next);
			irq_unlock(imask);
			return 1;
		}

		if (timeout_in_ticks == TICKS_NONE) {
			break;
		}

		/*
		 * Invoke nano_cpu_atomic_idle() with interrupts still disabled
		 * to prevent the scenario where an interrupt fires after
		 * re-enabling interrupts and before executing the "halt"
		 * instruction.  If the ISR performs a nano_isr_stack_push() on
		 * the same stack object, the subsequent execution of the "halt"
		 * instruction will result in the queued data being ignored
		 * until the next interrupt, if any.
		 *
		 * Thus it should be clear that an architectures implementation
		 * of nano_cpu_atomic_idle() must be able to atomically
		 * re-enable interrupts and enter a low-power mode.
		 *
		 * This explanation is valid for all nanokernel objects: stacks,
		 * FIFOs, LIFOs, and semaphores, for their
		 * nano_task_<object>_<get>() routines.
		 */

		nano_cpu_atomic_idle(imask);
		imask = irq_lock();
	}

	irq_unlock(imask);
	return 0;
}

int nano_stack_pop(struct nano_stack *stack, uint32_t *pData, int32_t timeout_in_ticks)
{
	static int (*func[3])(struct nano_stack *, uint32_t *, int32_t) = {
		nano_isr_stack_pop,
		nano_fiber_stack_pop,
		nano_task_stack_pop,
	};

	return func[sys_execution_context_type_get()](stack, pData, timeout_in_ticks);
}

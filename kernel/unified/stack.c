/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
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
 * @brief fixed-size stack object
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <misc/debug/object_tracing_common.h>
#include <toolchain.h>
#include <sections.h>
#include <ksched.h>
#include <wait_q.h>
#include <misc/__assert.h>
#include <init.h>

extern struct k_stack _k_stack_list_start[];
extern struct k_stack _k_stack_list_end[];

struct k_stack *_trace_list_k_stack;

#ifdef CONFIG_DEBUG_TRACING_KERNEL_OBJECTS

/*
 * Complete initialization of statically defined stacks.
 */
static int init_stack_module(struct device *dev)
{
	ARG_UNUSED(dev);

	struct k_stack *stack;

	for (stack = _k_stack_list_start; stack < _k_stack_list_end; stack++) {
		SYS_TRACING_OBJ_INIT(k_stack, stack);
	}
	return 0;
}

SYS_INIT(init_stack_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_DEBUG_TRACING_KERNEL_OBJECTS */

void k_stack_init(struct k_stack *stack, uint32_t *buffer, int num_entries)
{
	sys_dlist_init(&stack->wait_q);
	stack->next = stack->base = buffer;
	stack->top = stack->base + num_entries;

	SYS_TRACING_OBJ_INIT(k_stack, stack);
}

void k_stack_push(struct k_stack *stack, uint32_t data)
{
	struct k_thread *first_pending_thread;
	unsigned int key;

	__ASSERT(stack->next != stack->top, "stack is full");

	key = irq_lock();

	first_pending_thread = _unpend_first_thread(&stack->wait_q);

	if (first_pending_thread) {
		_abort_thread_timeout(first_pending_thread);
		_ready_thread(first_pending_thread);

		_set_thread_return_value_with_data(first_pending_thread,
						   0, (void *)data);

		if (!_is_in_isr() && _must_switch_threads()) {
			(void)_Swap(key);
			return;
		}
	} else {
		*(stack->next) = data;
		stack->next++;
	}

	irq_unlock(key);
}

int k_stack_pop(struct k_stack *stack, uint32_t *data, int32_t timeout)
{
	unsigned int key;
	int result;

	key = irq_lock();

	if (likely(stack->next > stack->base)) {
		stack->next--;
		*data = *(stack->next);
		irq_unlock(key);
		return 0;
	}

	if (timeout == K_NO_WAIT) {
		irq_unlock(key);
		return -EBUSY;
	}

	_pend_current_thread(&stack->wait_q, timeout);

	result = _Swap(key);
	if (result == 0) {
		*data = (uint32_t)_current->base.swap_data;
	}
	return result;
}

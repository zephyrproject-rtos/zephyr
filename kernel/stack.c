/*
 * Copyright (c) 2010-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief fixed-size stack object
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <debug/object_tracing_common.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <ksched.h>
#include <wait_q.h>
#include <misc/__assert.h>
#include <init.h>
#include <syscall_handler.h>

extern struct k_stack _k_stack_list_start[];
extern struct k_stack _k_stack_list_end[];

#ifdef CONFIG_OBJECT_TRACING

struct k_stack *_trace_list_k_stack;

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

#endif /* CONFIG_OBJECT_TRACING */

void k_stack_init(struct k_stack *stack, u32_t *buffer,
			unsigned int num_entries)
{
	_waitq_init(&stack->wait_q);
	stack->next = stack->base = buffer;
	stack->top = stack->base + num_entries;

	SYS_TRACING_OBJ_INIT(k_stack, stack);
	_k_object_init(stack);
}

int _impl_k_stack_alloc_init(struct k_stack *stack, unsigned int num_entries)
{
	void *buffer;
	int ret;

	buffer = z_thread_malloc(num_entries);
	if (buffer) {
		k_stack_init(stack, buffer, num_entries);
		stack->flags = K_STACK_FLAG_ALLOC;
		ret = 0;
	} else {
		ret = -ENOMEM;
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_stack_alloc_init, stack, num_entries)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(stack, K_OBJ_STACK));
	Z_OOPS(Z_SYSCALL_VERIFY(num_entries > 0));

	return _impl_k_stack_alloc_init((struct k_stack *)stack, num_entries);
}
#endif

void k_stack_cleanup(struct k_stack *stack)
{
	__ASSERT_NO_MSG(!_waitq_head(&stack->wait_q));

	if (stack->flags & K_STACK_FLAG_ALLOC) {
		k_free(stack->base);
		stack->base = NULL;
		stack->flags &= ~K_STACK_FLAG_ALLOC;
	}
}

void _impl_k_stack_push(struct k_stack *stack, u32_t data)
{
	struct k_thread *first_pending_thread;
	unsigned int key;

	__ASSERT(stack->next != stack->top, "stack is full");

	key = irq_lock();

	first_pending_thread = _unpend_first_thread(&stack->wait_q);

	if (first_pending_thread) {
		_ready_thread(first_pending_thread);

		_set_thread_return_value_with_data(first_pending_thread,
						   0, (void *)data);
		_reschedule(key);
		return;
	} else {
		*(stack->next) = data;
		stack->next++;
		irq_unlock(key);
	}

}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_stack_push, stack_p, data)
{
	struct k_stack *stack = (struct k_stack *)stack_p;

	Z_OOPS(Z_SYSCALL_OBJ(stack, K_OBJ_STACK));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(stack->next != stack->top,
				    "stack is full"));

	_impl_k_stack_push(stack, data);
	return 0;
}
#endif

int _impl_k_stack_pop(struct k_stack *stack, u32_t *data, s32_t timeout)
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

	result = _pend_current_thread(key, &stack->wait_q, timeout);
	if (result == -EAGAIN)
		return -EAGAIN;

	*data = (u32_t)_current->base.swap_data;
	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_stack_pop, stack, data, timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(stack, K_OBJ_STACK));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, sizeof(u32_t)));

	return _impl_k_stack_pop((struct k_stack *)stack, (u32_t *)data,
				 timeout);
}
#endif

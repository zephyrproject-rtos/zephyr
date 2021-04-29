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
#include <ksched.h>
#include <wait_q.h>
#include <sys/check.h>
#include <init.h>
#include <syscall_handler.h>
#include <kernel_internal.h>

#ifdef CONFIG_OBJECT_TRACING

struct k_stack *_trace_list_k_stack;

/*
 * Complete initialization of statically defined stacks.
 */
static int init_stack_module(const struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_stack, stack) {
		SYS_TRACING_OBJ_INIT(k_stack, stack);
	}
	return 0;
}

SYS_INIT(init_stack_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

void k_stack_init(struct k_stack *stack, stack_data_t *buffer,
		  uint32_t num_entries)
{
	z_waitq_init(&stack->wait_q);
	stack->lock = (struct k_spinlock) {};
	stack->next = stack->base = buffer;
	stack->top = stack->base + num_entries;

	SYS_TRACING_OBJ_INIT(k_stack, stack);
	z_object_init(stack);
}

int32_t z_impl_k_stack_alloc_init(struct k_stack *stack, uint32_t num_entries)
{
	void *buffer;
	int32_t ret;

	buffer = z_thread_malloc(num_entries * sizeof(stack_data_t));
	if (buffer != NULL) {
		k_stack_init(stack, buffer, num_entries);
		stack->flags = K_STACK_FLAG_ALLOC;
		ret = (int32_t)0;
	} else {
		ret = -ENOMEM;
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int32_t z_vrfy_k_stack_alloc_init(struct k_stack *stack,
					      uint32_t num_entries)
{
	Z_OOPS(Z_SYSCALL_OBJ_NEVER_INIT(stack, K_OBJ_STACK));
	Z_OOPS(Z_SYSCALL_VERIFY(num_entries > 0));
	return z_impl_k_stack_alloc_init(stack, num_entries);
}
#include <syscalls/k_stack_alloc_init_mrsh.c>
#endif

int k_stack_cleanup(struct k_stack *stack)
{
	CHECKIF(z_waitq_head(&stack->wait_q) != NULL) {
		return -EAGAIN;
	}

	if ((stack->flags & K_STACK_FLAG_ALLOC) != (uint8_t)0) {
		k_free(stack->base);
		stack->base = NULL;
		stack->flags &= ~K_STACK_FLAG_ALLOC;
	}
	return 0;
}

int z_impl_k_stack_push(struct k_stack *stack, stack_data_t data)
{
	struct k_thread *first_pending_thread;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&stack->lock);

	CHECKIF(stack->next == stack->top) {
		ret = -ENOMEM;
		goto out;
	}

	first_pending_thread = z_unpend_first_thread(&stack->wait_q);

	if (first_pending_thread != NULL) {
		z_thread_return_value_set_with_data(first_pending_thread,
						   0, (void *)data);

		z_ready_thread(first_pending_thread);
		z_reschedule(&stack->lock, key);
		goto end;
	} else {
		*(stack->next) = data;
		stack->next++;
		goto out;
	}

out:
	k_spin_unlock(&stack->lock, key);

end:
	return ret;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_stack_push(struct k_stack *stack, stack_data_t data)
{
	Z_OOPS(Z_SYSCALL_OBJ(stack, K_OBJ_STACK));

	return z_impl_k_stack_push(stack, data);
}
#include <syscalls/k_stack_push_mrsh.c>
#endif

int z_impl_k_stack_pop(struct k_stack *stack, stack_data_t *data,
		       k_timeout_t timeout)
{
	k_spinlock_key_t key;
	int result;

	key = k_spin_lock(&stack->lock);

	if (likely(stack->next > stack->base)) {
		stack->next--;
		*data = *(stack->next);
		k_spin_unlock(&stack->lock, key);
		return 0;
	}

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		k_spin_unlock(&stack->lock, key);
		return -EBUSY;
	}

	result = z_pend_curr(&stack->lock, key, &stack->wait_q, timeout);
	if (result == -EAGAIN) {
		return -EAGAIN;
	}

	*data = (stack_data_t)_current->base.swap_data;
	return 0;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_stack_pop(struct k_stack *stack,
				     stack_data_t *data, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(stack, K_OBJ_STACK));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, sizeof(stack_data_t)));
	return z_impl_k_stack_pop(stack, data, timeout);
}
#include <syscalls/k_stack_pop_mrsh.c>
#endif

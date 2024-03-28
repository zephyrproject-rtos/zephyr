/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel thread support
 *
 * This module provides general purpose thread support.
 */

#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys_clock.h>
#include <ksched.h>
#include <kthread.h>
#include <wait_q.h>
#include <zephyr/internal/syscall_handler.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <zephyr/init.h>
#include <zephyr/tracing/tracing.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/sys/check.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/iterable_sections.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_OBJ_CORE_THREAD
static struct k_obj_type  obj_type_thread;

#ifdef CONFIG_OBJ_CORE_STATS_THREAD
static struct k_obj_core_stats_desc  thread_stats_desc = {
	.raw_size = sizeof(struct k_cycle_stats),
	.query_size = sizeof(struct k_thread_runtime_stats),
	.raw   = z_thread_stats_raw,
	.query = z_thread_stats_query,
	.reset = z_thread_stats_reset,
	.disable = z_thread_stats_disable,
	.enable  = z_thread_stats_enable,
};
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */

static int init_thread_obj_core_list(void)
{
	/* Initialize mem_slab object type */

#ifdef CONFIG_OBJ_CORE_THREAD
	z_obj_type_init(&obj_type_thread, K_OBJ_TYPE_THREAD_ID,
			offsetof(struct k_thread, obj_core));
#endif /* CONFIG_OBJ_CORE_THREAD */

#ifdef CONFIG_OBJ_CORE_STATS_THREAD
	k_obj_type_stats_init(&obj_type_thread, &thread_stats_desc);
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */

	return 0;
}

SYS_INIT(init_thread_obj_core_list, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif /* CONFIG_OBJ_CORE_THREAD */


#define _FOREACH_STATIC_THREAD(thread_data)              \
	STRUCT_SECTION_FOREACH(_static_thread_data, thread_data)

bool k_is_in_isr(void)
{
	return arch_is_in_isr();
}
EXPORT_SYMBOL(k_is_in_isr);

#ifdef CONFIG_THREAD_CUSTOM_DATA
void z_impl_k_thread_custom_data_set(void *value)
{
	_current->custom_data = value;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_custom_data_set(void *data)
{
	z_impl_k_thread_custom_data_set(data);
}
#include <syscalls/k_thread_custom_data_set_mrsh.c>
#endif /* CONFIG_USERSPACE */

void *z_impl_k_thread_custom_data_get(void)
{
	return _current->custom_data;
}

#ifdef CONFIG_USERSPACE
static inline void *z_vrfy_k_thread_custom_data_get(void)
{
	return z_impl_k_thread_custom_data_get();
}
#include <syscalls/k_thread_custom_data_get_mrsh.c>

#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_THREAD_CUSTOM_DATA */

int z_impl_k_thread_name_set(struct k_thread *thread, const char *value)
{
#ifdef CONFIG_THREAD_NAME
	if (thread == NULL) {
		thread = _current;
	}

	strncpy(thread->name, value, CONFIG_THREAD_MAX_NAME_LEN - 1);
	thread->name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';

	SYS_PORT_TRACING_OBJ_FUNC(k_thread, name_set, thread, 0);

	return 0;
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(value);

	SYS_PORT_TRACING_OBJ_FUNC(k_thread, name_set, thread, -ENOSYS);

	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_name_set(struct k_thread *thread, const char *str)
{
#ifdef CONFIG_THREAD_NAME
	char name[CONFIG_THREAD_MAX_NAME_LEN];

	if (thread != NULL) {
		if (K_SYSCALL_OBJ(thread, K_OBJ_THREAD) != 0) {
			return -EINVAL;
		}
	}

	/* In theory we could copy directly into thread->name, but
	 * the current z_vrfy / z_impl split does not provide a
	 * means of doing so.
	 */
	if (k_usermode_string_copy(name, (char *)str, sizeof(name)) != 0) {
		return -EFAULT;
	}

	return z_impl_k_thread_name_set(thread, name);
#else
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}
#include <syscalls/k_thread_name_set_mrsh.c>
#endif /* CONFIG_USERSPACE */

const char *k_thread_name_get(k_tid_t thread)
{
#ifdef CONFIG_THREAD_NAME
	return (const char *)thread->name;
#else
	ARG_UNUSED(thread);
	return NULL;
#endif /* CONFIG_THREAD_NAME */
}

int z_impl_k_thread_name_copy(k_tid_t thread, char *buf, size_t size)
{
#ifdef CONFIG_THREAD_NAME
	strncpy(buf, thread->name, size);
	return 0;
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}

static size_t copy_bytes(char *dest, size_t dest_size, const char *src, size_t src_size)
{
	size_t  bytes_to_copy;

	bytes_to_copy = MIN(dest_size, src_size);
	memcpy(dest, src, bytes_to_copy);

	return bytes_to_copy;
}

#define Z_STATE_STR_DUMMY       "dummy"
#define Z_STATE_STR_PENDING     "pending"
#define Z_STATE_STR_PRESTART    "prestart"
#define Z_STATE_STR_DEAD        "dead"
#define Z_STATE_STR_SUSPENDED   "suspended"
#define Z_STATE_STR_ABORTING    "aborting"
#define Z_STATE_STR_SUSPENDING  "suspending"
#define Z_STATE_STR_QUEUED      "queued"

const char *k_thread_state_str(k_tid_t thread_id, char *buf, size_t buf_size)
{
	size_t      off = 0;
	uint8_t     bit;
	uint8_t     thread_state = thread_id->base.thread_state;
	static const struct {
		const char *str;
		size_t      len;
	} state_string[] = {
		{ Z_STATE_STR_DUMMY, sizeof(Z_STATE_STR_DUMMY) - 1},
		{ Z_STATE_STR_PENDING, sizeof(Z_STATE_STR_PENDING) - 1},
		{ Z_STATE_STR_PRESTART, sizeof(Z_STATE_STR_PRESTART) - 1},
		{ Z_STATE_STR_DEAD, sizeof(Z_STATE_STR_DEAD) - 1},
		{ Z_STATE_STR_SUSPENDED, sizeof(Z_STATE_STR_SUSPENDED) - 1},
		{ Z_STATE_STR_ABORTING, sizeof(Z_STATE_STR_ABORTING) - 1},
		{ Z_STATE_STR_SUSPENDING, sizeof(Z_STATE_STR_SUSPENDING) - 1},
		{ Z_STATE_STR_QUEUED, sizeof(Z_STATE_STR_QUEUED) - 1},
	};

	if ((buf == NULL) || (buf_size == 0)) {
		return "";
	}

	buf_size--;   /* Reserve 1 byte for end-of-string character */

	/*
	 * Loop through each bit in the thread_state. Stop once all have
	 * been processed. If more than one thread_state bit is set, then
	 * separate the descriptive strings with a '+'.
	 */


	for (unsigned int index = 0; thread_state != 0; index++) {
		bit = BIT(index);
		if ((thread_state & bit) == 0) {
			continue;
		}

		off += copy_bytes(buf + off, buf_size - off,
				  state_string[index].str,
				  state_string[index].len);

		thread_state &= ~bit;

		if (thread_state != 0) {
			off += copy_bytes(buf + off, buf_size - off, "+", 1);
		}
	}

	buf[off] = '\0';

	return (const char *)buf;
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_k_thread_name_copy(k_tid_t thread,
					    char *buf, size_t size)
{
#ifdef CONFIG_THREAD_NAME
	size_t len;
	struct k_object *ko = k_object_find(thread);

	/* Special case: we allow reading the names of initialized threads
	 * even if we don't have permission on them
	 */
	if (thread == NULL || ko->type != K_OBJ_THREAD ||
	    (ko->flags & K_OBJ_FLAG_INITIALIZED) == 0) {
		return -EINVAL;
	}
	if (K_SYSCALL_MEMORY_WRITE(buf, size) != 0) {
		return -EFAULT;
	}
	len = strlen(thread->name);
	if (len + 1 > size) {
		return -ENOSPC;
	}

	return k_usermode_to_copy((void *)buf, thread->name, len + 1);
#else
	ARG_UNUSED(thread);
	ARG_UNUSED(buf);
	ARG_UNUSED(size);
	return -ENOSYS;
#endif /* CONFIG_THREAD_NAME */
}
#include <syscalls/k_thread_name_copy_mrsh.c>
#endif /* CONFIG_USERSPACE */


#ifdef CONFIG_MULTITHREADING
#ifdef CONFIG_STACK_SENTINEL
/* Check that the stack sentinel is still present
 *
 * The stack sentinel feature writes a magic value to the lowest 4 bytes of
 * the thread's stack when the thread is initialized. This value gets checked
 * in a few places:
 *
 * 1) In k_yield() if the current thread is not swapped out
 * 2) After servicing a non-nested interrupt
 * 3) In z_swap(), check the sentinel in the outgoing thread
 *
 * Item 2 requires support in arch/ code.
 *
 * If the check fails, the thread will be terminated appropriately through
 * the system fatal error handler.
 */
void z_check_stack_sentinel(void)
{
	uint32_t *stack;

	if ((_current->base.thread_state & _THREAD_DUMMY) != 0) {
		return;
	}

	stack = (uint32_t *)_current->stack_info.start;
	if (*stack != STACK_SENTINEL) {
		/* Restore it so further checks don't trigger this same error */
		*stack = STACK_SENTINEL;
		z_except_reason(K_ERR_STACK_CHK_FAIL);
	}
}
#endif /* CONFIG_STACK_SENTINEL */

void z_impl_k_thread_start(struct k_thread *thread)
{
	SYS_PORT_TRACING_OBJ_FUNC(k_thread, start, thread);

	z_sched_start(thread);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_start(struct k_thread *thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_start(thread);
}
#include <syscalls/k_thread_start_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MULTITHREADING */


#if CONFIG_STACK_POINTER_RANDOM
int z_stack_adjust_initialized;

static size_t random_offset(size_t stack_size)
{
	size_t random_val;

	if (!z_stack_adjust_initialized) {
		z_early_rand_get((uint8_t *)&random_val, sizeof(random_val));
	} else {
		sys_rand_get((uint8_t *)&random_val, sizeof(random_val));
	}

	/* Don't need to worry about alignment of the size here,
	 * arch_new_thread() is required to do it.
	 *
	 * FIXME: Not the best way to get a random number in a range.
	 * See #6493
	 */
	const size_t fuzz = random_val % CONFIG_STACK_POINTER_RANDOM;

	if (unlikely(fuzz * 2 > stack_size)) {
		return 0;
	}

	return fuzz;
}
#if defined(CONFIG_STACK_GROWS_UP)
	/* This is so rare not bothering for now */
#error "Stack pointer randomization not implemented for upward growing stacks"
#endif /* CONFIG_STACK_GROWS_UP */
#endif /* CONFIG_STACK_POINTER_RANDOM */

static char *setup_thread_stack(struct k_thread *new_thread,
				k_thread_stack_t *stack, size_t stack_size)
{
	size_t stack_obj_size, stack_buf_size;
	char *stack_ptr, *stack_buf_start;
	size_t delta = 0;

#ifdef CONFIG_USERSPACE
	if (z_stack_is_user_capable(stack)) {
		stack_obj_size = K_THREAD_STACK_LEN(stack_size);
		stack_buf_start = K_THREAD_STACK_BUFFER(stack);
		stack_buf_size = stack_obj_size - K_THREAD_STACK_RESERVED;
	} else
#endif /* CONFIG_USERSPACE */
	{
		/* Object cannot host a user mode thread */
		stack_obj_size = K_KERNEL_STACK_LEN(stack_size);
		stack_buf_start = K_KERNEL_STACK_BUFFER(stack);
		stack_buf_size = stack_obj_size - K_KERNEL_STACK_RESERVED;

		/* Zephyr treats stack overflow as an app bug.  But
		 * this particular overflow can be seen by static
		 * analysis so needs to be handled somehow.
		 */
		if (K_KERNEL_STACK_RESERVED > stack_obj_size) {
			k_panic();
		}

	}

	/* Initial stack pointer at the high end of the stack object, may
	 * be reduced later in this function by TLS or random offset
	 */
	stack_ptr = (char *)stack + stack_obj_size;

	LOG_DBG("stack %p for thread %p: obj_size=%zu buf_start=%p "
		" buf_size %zu stack_ptr=%p",
		stack, new_thread, stack_obj_size, (void *)stack_buf_start,
		stack_buf_size, (void *)stack_ptr);

#ifdef CONFIG_INIT_STACKS
	memset(stack_buf_start, 0xaa, stack_buf_size);
#endif /* CONFIG_INIT_STACKS */
#ifdef CONFIG_STACK_SENTINEL
	/* Put the stack sentinel at the lowest 4 bytes of the stack area.
	 * We periodically check that it's still present and kill the thread
	 * if it isn't.
	 */
	*((uint32_t *)stack_buf_start) = STACK_SENTINEL;
#endif /* CONFIG_STACK_SENTINEL */
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* TLS is always last within the stack buffer */
	delta += arch_tls_stack_setup(new_thread, stack_ptr);
#endif /* CONFIG_THREAD_LOCAL_STORAGE */
#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
	size_t tls_size = sizeof(struct _thread_userspace_local_data);

	/* reserve space on highest memory of stack buffer for local data */
	delta += tls_size;
	new_thread->userspace_local_data =
		(struct _thread_userspace_local_data *)(stack_ptr - delta);
#endif /* CONFIG_THREAD_USERSPACE_LOCAL_DATA */
#if CONFIG_STACK_POINTER_RANDOM
	delta += random_offset(stack_buf_size);
#endif /* CONFIG_STACK_POINTER_RANDOM */
	delta = ROUND_UP(delta, ARCH_STACK_PTR_ALIGN);
#ifdef CONFIG_THREAD_STACK_INFO
	/* Initial values. Arches which implement MPU guards that "borrow"
	 * memory from the stack buffer (not tracked in K_THREAD_STACK_RESERVED)
	 * will need to appropriately update this.
	 *
	 * The bounds tracked here correspond to the area of the stack object
	 * that the thread can access, which includes TLS.
	 */
	new_thread->stack_info.start = (uintptr_t)stack_buf_start;
	new_thread->stack_info.size = stack_buf_size;
	new_thread->stack_info.delta = delta;
#endif /* CONFIG_THREAD_STACK_INFO */
	stack_ptr -= delta;

	return stack_ptr;
}

/*
 * The provided stack_size value is presumed to be either the result of
 * K_THREAD_STACK_SIZEOF(stack), or the size value passed to the instance
 * of K_THREAD_STACK_DEFINE() which defined 'stack'.
 */
char *z_setup_new_thread(struct k_thread *new_thread,
			 k_thread_stack_t *stack, size_t stack_size,
			 k_thread_entry_t entry,
			 void *p1, void *p2, void *p3,
			 int prio, uint32_t options, const char *name)
{
	char *stack_ptr;

	Z_ASSERT_VALID_PRIO(prio, entry);

#ifdef CONFIG_OBJ_CORE_THREAD
	k_obj_core_init_and_link(K_OBJ_CORE(new_thread), &obj_type_thread);
#ifdef CONFIG_OBJ_CORE_STATS_THREAD
	k_obj_core_stats_register(K_OBJ_CORE(new_thread),
				  &new_thread->base.usage,
				  sizeof(new_thread->base.usage));
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */
#endif /* CONFIG_OBJ_CORE_THREAD */

#ifdef CONFIG_USERSPACE
	__ASSERT((options & K_USER) == 0U || z_stack_is_user_capable(stack),
		 "user thread %p with kernel-only stack %p",
		 new_thread, stack);
	k_object_init(new_thread);
	k_object_init(stack);
	new_thread->stack_obj = stack;
	new_thread->syscall_frame = NULL;

	/* Any given thread has access to itself */
	k_object_access_grant(new_thread, new_thread);
#endif /* CONFIG_USERSPACE */
	z_waitq_init(&new_thread->join_queue);

	/* Initialize various struct k_thread members */
	z_init_thread_base(&new_thread->base, prio, _THREAD_PRESTART, options);
	stack_ptr = setup_thread_stack(new_thread, stack, stack_size);

#ifdef CONFIG_KERNEL_COHERENCE
	/* Check that the thread object is safe, but that the stack is
	 * still cached!
	 */
	__ASSERT_NO_MSG(arch_mem_coherent(new_thread));

	/* When dynamic thread stack is available, the stack may come from
	 * uncached area.
	 */
#ifndef CONFIG_DYNAMIC_THREAD
	__ASSERT_NO_MSG(!arch_mem_coherent(stack));
#endif  /* CONFIG_DYNAMIC_THREAD */

#endif /* CONFIG_KERNEL_COHERENCE */

	arch_new_thread(new_thread, stack, stack_ptr, entry, p1, p2, p3);

	/* static threads overwrite it afterwards with real value */
	new_thread->init_data = NULL;

#ifdef CONFIG_USE_SWITCH
	/* switch_handle must be non-null except when inside z_swap()
	 * for synchronization reasons.  Historically some notional
	 * USE_SWITCH architectures have actually ignored the field
	 */
	__ASSERT(new_thread->switch_handle != NULL,
		 "arch layer failed to initialize switch_handle");
#endif /* CONFIG_USE_SWITCH */
#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */
	new_thread->custom_data = NULL;
#endif /* CONFIG_THREAD_CUSTOM_DATA */
#ifdef CONFIG_EVENTS
	new_thread->no_wake_on_timeout = false;
#endif /* CONFIG_EVENTS */
#ifdef CONFIG_THREAD_MONITOR
	new_thread->entry.pEntry = entry;
	new_thread->entry.parameter1 = p1;
	new_thread->entry.parameter2 = p2;
	new_thread->entry.parameter3 = p3;

	k_spinlock_key_t key = k_spin_lock(&z_thread_monitor_lock);

	new_thread->next_thread = _kernel.threads;
	_kernel.threads = new_thread;
	k_spin_unlock(&z_thread_monitor_lock, key);
#endif /* CONFIG_THREAD_MONITOR */
#ifdef CONFIG_THREAD_NAME
	if (name != NULL) {
		strncpy(new_thread->name, name,
			CONFIG_THREAD_MAX_NAME_LEN - 1);
		/* Ensure NULL termination, truncate if longer */
		new_thread->name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';
	} else {
		new_thread->name[0] = '\0';
	}
#endif /* CONFIG_THREAD_NAME */
#ifdef CONFIG_SCHED_CPU_MASK
	if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY)) {
		new_thread->base.cpu_mask = 1; /* must specify only one cpu */
	} else {
		new_thread->base.cpu_mask = -1; /* allow all cpus */
	}
#endif /* CONFIG_SCHED_CPU_MASK */
#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
	/* _current may be null if the dummy thread is not used */
	if (!_current) {
		new_thread->resource_pool = NULL;
		return stack_ptr;
	}
#endif /* CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN */
#ifdef CONFIG_USERSPACE
	z_mem_domain_init_thread(new_thread);

	if ((options & K_INHERIT_PERMS) != 0U) {
		k_thread_perms_inherit(_current, new_thread);
	}
#endif /* CONFIG_USERSPACE */
#ifdef CONFIG_SCHED_DEADLINE
	new_thread->base.prio_deadline = 0;
#endif /* CONFIG_SCHED_DEADLINE */
	new_thread->resource_pool = _current->resource_pool;

#ifdef CONFIG_SMP
	z_waitq_init(&new_thread->halt_queue);
#endif /* CONFIG_SMP */

#ifdef CONFIG_SCHED_THREAD_USAGE
	new_thread->base.usage = (struct k_cycle_stats) {};
	new_thread->base.usage.track_usage =
		CONFIG_SCHED_THREAD_USAGE_AUTO_ENABLE;
#endif /* CONFIG_SCHED_THREAD_USAGE */

	SYS_PORT_TRACING_OBJ_FUNC(k_thread, create, new_thread);

	return stack_ptr;
}

#ifdef CONFIG_MULTITHREADING
k_tid_t z_impl_k_thread_create(struct k_thread *new_thread,
			      k_thread_stack_t *stack,
			      size_t stack_size, k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, uint32_t options, k_timeout_t delay)
{
	__ASSERT(!arch_is_in_isr(), "Threads may not be created in ISRs");

	z_setup_new_thread(new_thread, stack, stack_size, entry, p1, p2, p3,
			  prio, options, NULL);

	if (!K_TIMEOUT_EQ(delay, K_FOREVER)) {
		thread_schedule_new(new_thread, delay);
	}

	return new_thread;
}


#ifdef CONFIG_USERSPACE
bool z_stack_is_user_capable(k_thread_stack_t *stack)
{
	return k_object_find(stack) != NULL;
}

k_tid_t z_vrfy_k_thread_create(struct k_thread *new_thread,
			       k_thread_stack_t *stack,
			       size_t stack_size, k_thread_entry_t entry,
			       void *p1, void *p2, void *p3,
			       int prio, uint32_t options, k_timeout_t delay)
{
	size_t total_size, stack_obj_size;
	struct k_object *stack_object;

	/* The thread and stack objects *must* be in an uninitialized state */
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(new_thread, K_OBJ_THREAD));

	/* No need to check z_stack_is_user_capable(), it won't be in the
	 * object table if it isn't
	 */
	stack_object = k_object_find(stack);
	K_OOPS(K_SYSCALL_VERIFY_MSG(k_object_validation_check(stack_object, stack,
						K_OBJ_THREAD_STACK_ELEMENT,
						_OBJ_INIT_FALSE) == 0,
				    "bad stack object"));

	/* Verify that the stack size passed in is OK by computing the total
	 * size and comparing it with the size value in the object metadata
	 */
	K_OOPS(K_SYSCALL_VERIFY_MSG(!size_add_overflow(K_THREAD_STACK_RESERVED,
						       stack_size, &total_size),
				    "stack size overflow (%zu+%zu)",
				    stack_size,
				    K_THREAD_STACK_RESERVED));

	/* Testing less-than-or-equal since additional room may have been
	 * allocated for alignment constraints
	 */
#ifdef CONFIG_GEN_PRIV_STACKS
	stack_obj_size = stack_object->data.stack_data->size;
#else
	stack_obj_size = stack_object->data.stack_size;
#endif /* CONFIG_GEN_PRIV_STACKS */
	K_OOPS(K_SYSCALL_VERIFY_MSG(total_size <= stack_obj_size,
				    "stack size %zu is too big, max is %zu",
				    total_size, stack_obj_size));

	/* User threads may only create other user threads and they can't
	 * be marked as essential
	 */
	K_OOPS(K_SYSCALL_VERIFY(options & K_USER));
	K_OOPS(K_SYSCALL_VERIFY(!(options & K_ESSENTIAL)));

	/* Check validity of prio argument; must be the same or worse priority
	 * than the caller
	 */
	K_OOPS(K_SYSCALL_VERIFY(_is_valid_prio(prio, NULL)));
	K_OOPS(K_SYSCALL_VERIFY(z_is_prio_lower_or_equal(prio,
							_current->base.prio)));

	z_setup_new_thread(new_thread, stack, stack_size,
			   entry, p1, p2, p3, prio, options, NULL);

	if (!K_TIMEOUT_EQ(delay, K_FOREVER)) {
		thread_schedule_new(new_thread, delay);
	}

	return new_thread;
}
#include <syscalls/k_thread_create_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MULTITHREADING */


void z_init_thread_base(struct _thread_base *thread_base, int priority,
		       uint32_t initial_state, unsigned int options)
{
	/* k_q_node is initialized upon first insertion in a list */
	thread_base->pended_on = NULL;
	thread_base->user_options = (uint8_t)options;
	thread_base->thread_state = (uint8_t)initial_state;

	thread_base->prio = priority;

	thread_base->sched_locked = 0U;

#ifdef CONFIG_SMP
	thread_base->is_idle = 0;
#endif /* CONFIG_SMP */

#ifdef CONFIG_TIMESLICE_PER_THREAD
	thread_base->slice_ticks = 0;
	thread_base->slice_expired = NULL;
#endif /* CONFIG_TIMESLICE_PER_THREAD */

	/* swap_data does not need to be initialized */

	z_init_thread_timeout(thread_base);
}

FUNC_NORETURN void k_thread_user_mode_enter(k_thread_entry_t entry,
					    void *p1, void *p2, void *p3)
{
	SYS_PORT_TRACING_FUNC(k_thread, user_mode_enter);

	_current->base.user_options |= K_USER;
	z_thread_essential_clear(_current);
#ifdef CONFIG_THREAD_MONITOR
	_current->entry.pEntry = entry;
	_current->entry.parameter1 = p1;
	_current->entry.parameter2 = p2;
	_current->entry.parameter3 = p3;
#endif /* CONFIG_THREAD_MONITOR */
#ifdef CONFIG_USERSPACE
	__ASSERT(z_stack_is_user_capable(_current->stack_obj),
		 "dropping to user mode with kernel-only stack object");
#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
	memset(_current->userspace_local_data, 0,
	       sizeof(struct _thread_userspace_local_data));
#endif /* CONFIG_THREAD_USERSPACE_LOCAL_DATA */
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	arch_tls_stack_setup(_current,
			     (char *)(_current->stack_info.start +
				      _current->stack_info.size));
#endif /* CONFIG_THREAD_LOCAL_STORAGE */
	arch_user_mode_enter(entry, p1, p2, p3);
#else
	/* XXX In this case we do not reset the stack */
	z_thread_entry(entry, p1, p2, p3);
#endif /* CONFIG_USERSPACE */
}

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
#ifdef CONFIG_STACK_GROWS_UP
#error "Unsupported configuration for stack analysis"
#endif /* CONFIG_STACK_GROWS_UP */

int z_stack_space_get(const uint8_t *stack_start, size_t size, size_t *unused_ptr)
{
	size_t unused = 0;
	const uint8_t *checked_stack = stack_start;
	/* Take the address of any local variable as a shallow bound for the
	 * stack pointer.  Addresses above it are guaranteed to be
	 * accessible.
	 */
	const uint8_t *stack_pointer = (const uint8_t *)&stack_start;

	/* If we are currently running on the stack being analyzed, some
	 * memory management hardware will generate an exception if we
	 * read unused stack memory.
	 *
	 * This never happens when invoked from user mode, as user mode
	 * will always run this function on the privilege elevation stack.
	 */
	if ((stack_pointer > stack_start) && (stack_pointer <= (stack_start + size)) &&
	    IS_ENABLED(CONFIG_NO_UNUSED_STACK_INSPECTION)) {
		/* TODO: We could add an arch_ API call to temporarily
		 * disable the stack checking in the CPU, but this would
		 * need to be properly managed wrt context switches/interrupts
		 */
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_STACK_SENTINEL)) {
		/* First 4 bytes of the stack buffer reserved for the
		 * sentinel value, it won't be 0xAAAAAAAA for thread
		 * stacks.
		 *
		 * FIXME: thread->stack_info.start ought to reflect
		 * this!
		 */
		checked_stack += 4;
		size -= 4;
	}

	for (size_t i = 0; i < size; i++) {
		if ((checked_stack[i]) == 0xaaU) {
			unused++;
		} else {
			break;
		}
	}

	*unused_ptr = unused;

	return 0;
}

int z_impl_k_thread_stack_space_get(const struct k_thread *thread,
				    size_t *unused_ptr)
{
	return z_stack_space_get((const uint8_t *)thread->stack_info.start,
				 thread->stack_info.size, unused_ptr);
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_thread_stack_space_get(const struct k_thread *thread,
				    size_t *unused_ptr)
{
	size_t unused;
	int ret;

	ret = K_SYSCALL_OBJ(thread, K_OBJ_THREAD);
	CHECKIF(ret != 0) {
		return ret;
	}

	ret = z_impl_k_thread_stack_space_get(thread, &unused);
	CHECKIF(ret != 0) {
		return ret;
	}

	ret = k_usermode_to_copy(unused_ptr, &unused, sizeof(size_t));
	CHECKIF(ret != 0) {
		return ret;
	}

	return 0;
}
#include <syscalls/k_thread_stack_space_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_INIT_STACKS && CONFIG_THREAD_STACK_INFO */

#ifdef CONFIG_USERSPACE
static inline k_ticks_t z_vrfy_k_thread_timeout_remaining_ticks(
						    const struct k_thread *thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_timeout_remaining_ticks(thread);
}
#include <syscalls/k_thread_timeout_remaining_ticks_mrsh.c>

static inline k_ticks_t z_vrfy_k_thread_timeout_expires_ticks(
						  const struct k_thread *thread)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	return z_impl_k_thread_timeout_expires_ticks(thread);
}
#include <syscalls/k_thread_timeout_expires_ticks_mrsh.c>
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
void z_thread_mark_switched_in(void)
{
#if defined(CONFIG_SCHED_THREAD_USAGE) && !defined(CONFIG_USE_SWITCH)
	z_sched_usage_start(_current);
#endif /* CONFIG_SCHED_THREAD_USAGE && !CONFIG_USE_SWITCH */

#ifdef CONFIG_TRACING
	SYS_PORT_TRACING_FUNC(k_thread, switched_in);
#endif /* CONFIG_TRACING */
}

void z_thread_mark_switched_out(void)
{
#if defined(CONFIG_SCHED_THREAD_USAGE) && !defined(CONFIG_USE_SWITCH)
	z_sched_usage_stop();
#endif /*CONFIG_SCHED_THREAD_USAGE && !CONFIG_USE_SWITCH */

#ifdef CONFIG_TRACING
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Dummy thread won't have TLS set up to run arbitrary code */
	if (!_current_cpu->current ||
	    (_current_cpu->current->base.thread_state & _THREAD_DUMMY) != 0)
		return;
#endif /* CONFIG_THREAD_LOCAL_STORAGE */
	SYS_PORT_TRACING_FUNC(k_thread, switched_out);
#endif /* CONFIG_TRACING */
}
#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

int k_thread_runtime_stats_get(k_tid_t thread,
			       k_thread_runtime_stats_t *stats)
{
	if ((thread == NULL) || (stats == NULL)) {
		return -EINVAL;
	}

#ifdef CONFIG_SCHED_THREAD_USAGE
	z_sched_thread_usage(thread, stats);
#else
	*stats = (k_thread_runtime_stats_t) {};
#endif /* CONFIG_SCHED_THREAD_USAGE */

	return 0;
}

int k_thread_runtime_stats_all_get(k_thread_runtime_stats_t *stats)
{
#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	k_thread_runtime_stats_t  tmp_stats;
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */

	if (stats == NULL) {
		return -EINVAL;
	}

	*stats = (k_thread_runtime_stats_t) {};

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	/* Retrieve the usage stats for each core and amalgamate them. */

	unsigned int num_cpus = arch_num_cpus();

	for (uint8_t i = 0; i < num_cpus; i++) {
		z_sched_cpu_usage(i, &tmp_stats);

		stats->execution_cycles += tmp_stats.execution_cycles;
		stats->total_cycles     += tmp_stats.total_cycles;
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		stats->current_cycles   += tmp_stats.current_cycles;
		stats->peak_cycles      += tmp_stats.peak_cycles;
		stats->average_cycles   += tmp_stats.average_cycles;
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
		stats->idle_cycles      += tmp_stats.idle_cycles;
	}
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */

	return 0;
}

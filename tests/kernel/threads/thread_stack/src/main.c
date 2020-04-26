/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <syscall_handler.h>

#include "test_syscall.h"

/*
 * Stack testing
 */
struct k_thread test_thread;
#define NUM_STACKS	3
#define STEST_STACKSIZE	(512 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_DEFINE(user_stack, STEST_STACKSIZE);
K_THREAD_STACK_ARRAY_DEFINE(user_stack_array, NUM_STACKS, STEST_STACKSIZE);

struct foo {
	int bar;

	K_THREAD_STACK_MEMBER(stack, STEST_STACKSIZE);
	int baz;
};

struct foo stest_member_stack;

void z_impl_stack_info_get(char **start_addr, size_t *size)
{
	*start_addr = (char *)k_current_get()->stack_info.start;
	*size = k_current_get()->stack_info.size;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_stack_info_get(char **start_addr,
					 size_t *size)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(start_addr, sizeof(uintptr_t)));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(size, sizeof(size_t)));

	z_impl_stack_info_get(start_addr, size);
}
#include <syscalls/stack_info_get_mrsh.c>

int z_impl_check_perms(void *addr, size_t size, int write)
{
	return arch_buffer_validate(addr, size, write);
}

static inline int z_vrfy_check_perms(void *addr, size_t size, int write)
{
	return z_impl_check_perms((void *)addr, size, write);
}
#include <syscalls/check_perms_mrsh.c>
#endif /* CONFIG_USERSPACE */

void stack_buffer_scenarios(k_thread_stack_t *stack_obj, size_t obj_size)
{
	size_t stack_size, unused;
	uint8_t val;
	char *stack_start, *stack_ptr, *stack_end, *obj_start, *obj_end;
	volatile char *pos;
	int ret, expected;
	uintptr_t base = (uintptr_t)stack_obj;
	bool is_usermode;

#ifdef CONFIG_USERSPACE
	is_usermode = arch_is_user_context();
#else
	is_usermode = false;
#endif

	/* Dump interesting information */
	stack_info_get(&stack_start, &stack_size);
	printk("   - Thread reports buffer %p size %zu\n", stack_start,
	       stack_size);

	stack_end = stack_start + stack_size;
	obj_end = (char *)stack_obj + obj_size;
	obj_start = (char *)stack_obj;

	/* Assert that the created stack object, with the reserved data
	 * removed, can hold a thread buffer of STEST_STACKSIZE
	 */
	zassert_true(STEST_STACKSIZE <= (obj_size - K_THREAD_STACK_RESERVED),
		      "bad stack size in object");

	/* Check that the stack info in the thread marks a region
	 * completely contained within the stack object
	 */
	zassert_true(stack_end <= obj_end,
		     "stack size in thread struct out of bounds (overflow)");
	zassert_true(stack_start >= obj_start,
		     "stack size in thread struct out of bounds (underflow)");

	/* Check that the base of the stack is aligned properly. */
	zassert_true(base % Z_THREAD_STACK_OBJ_ALIGN(stack_size) == 0,
		     "stack base address %p not aligned to %zu",
		     stack_obj, Z_THREAD_STACK_OBJ_ALIGN(stack_size));

	/* Check that the entire stack buffer is read/writable */
	printk("   - check read/write to stack buffer\n");

	/* Address of this stack variable is guaranteed to part of
	 * the active stack, and close to the actual stack pointer.
	 * Some CPUs have hardware stack overflow detection which
	 * faults on memory access within the stack buffer but below
	 * the stack pointer.
	 *
	 * First test does direct read & write starting at the estimated
	 * stack pointer up to the highest addresses in the buffer
	 */
	stack_ptr = &val;
	for (pos = stack_ptr; pos < stack_end; pos++) {
		/* pos is volatile so this doesn't get optimized out */
		val = *pos;
		*pos = val;
	}

#ifdef CONFIG_USERSPACE
	if (is_usermode) {
		/* If we're in user mode, check every byte in the stack buffer
		 * to ensure that the thread has permissions on it.
		 */
		for (pos = stack_start; pos < stack_end; pos++) {
			zassert_false(check_perms((void *)pos, 1, 1),
				      "bad MPU/MMU permission on stack buffer at address %p",
				      pos);
		}

		/* Bounds check the user accessible area, it shouldn't extend
		 * before or after the stack. Because of memory protection HW
		 * alignment constraints, we test the end of the stack object
		 * and not the buffer.
		 */
		zassert_true(check_perms(stack_start - 1, 1, 0),
			     "user mode access to memory %p before start of stack object",
			     obj_start - 1);
		zassert_true(check_perms(stack_end, 1, 0),
			     "user mode access to memory %p past end of stack object",
			     obj_end);
		zassert_true(stack_size <= obj_size - K_THREAD_STACK_RESERVED,
			      "bad stack size %zu in thread struct",
			      stack_size);
	}
#endif

	/* This API is being removed just whine about it for now */
	if (Z_THREAD_STACK_BUFFER(stack_obj) != stack_start) {
		printk("WARNING: Z_THREAD_STACK_BUFFER() reports %p\n",
		       Z_THREAD_STACK_BUFFER(stack_obj));
	}

	ret = k_thread_stack_space_get(k_current_get(), &unused);
	if (!is_usermode && IS_ENABLED(CONFIG_NO_UNUSED_STACK_INSPECTION)) {
		expected = -ENOTSUP;
	} else {
		expected = 0;
	}

	zassert_equal(ret, expected, "unexpected return value %d", ret);
	if (ret == 0) {
		printk("self-reported unused stack space: %zu\n", unused);
	}
}

void stest_thread_entry(void *p1, void *p2, void *p3)
{
	bool drop = (bool)p3;

	if (drop) {
		k_thread_user_mode_enter(stest_thread_entry, p1, p2,
					 (void *)false);
	} else {
		stack_buffer_scenarios((k_thread_stack_t *)p1, (size_t)p2);
	}
}

void stest_thread_launch(void *stack_obj, size_t obj_size, uint32_t flags,
			 bool drop)
{
	int ret;
	size_t unused;

	k_thread_create(&test_thread, stack_obj, STEST_STACKSIZE,
			stest_thread_entry, stack_obj, (void *)obj_size,
			(void *)drop,
			-1, flags, K_NO_WAIT);
	k_thread_join(&test_thread, K_FOREVER);

	ret = k_thread_stack_space_get(&test_thread, &unused);
	zassert_equal(ret, 0, "failed to calculate unused stack space\n");
	printk("target thread unused stack space: %zu\n", unused);
}

void scenario_entry(void *stack_obj, size_t obj_size)
{
	printk("Stack object %p[%zu]\n", stack_obj, obj_size);
	printk(" - Testing supervisor mode\n");
	stest_thread_launch(stack_obj, obj_size, 0, false);
	printk(" - Testing user mode (direct launch)\n");
	stest_thread_launch(stack_obj, obj_size, K_USER | K_INHERIT_PERMS,
			    false);
	printk(" - Testing user mode (drop)\n");
	stest_thread_launch(stack_obj, obj_size, K_INHERIT_PERMS,
			    true);
}

/**
 * @brief Test kernel provides user thread read/write access to its own stack
 * memory buffer
 *
 * @details Thread can access its own stack memory buffer and perform
 * read/write operations.
 *
 * @ingroup kernel_memprotect_tests
 */
void test_stack_buffer(void)
{
	printk("Reserved space: %zu\n", K_THREAD_STACK_RESERVED);
	printk("Provided stack size: %u\n", STEST_STACKSIZE);
	scenario_entry(stest_stack, sizeof(stest_stack));

	for (int i = 0; i < NUM_STACKS; i++) {
		scenario_entry(stest_stack_array[i],
			       sizeof(stest_stack_array[i]));
	}

	scenario_entry(&stest_member_stack.stack,
		       sizeof(stest_member_stack.stack));

}

void test_main(void)
{
	ztest_test_suite(userspace,
			 ztest_1cpu_unit_test(test_stack_buffer)
			 );
	ztest_run_test_suite(userspace);
}

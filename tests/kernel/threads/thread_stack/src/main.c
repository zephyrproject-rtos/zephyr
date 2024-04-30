/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/internal/syscall_handler.h>
#include <kernel_internal.h>

#include "test_syscall.h"

/*
 * Stack testing
 */
struct k_thread test_thread;
#define NUM_STACKS	3
#define STEST_STACKSIZE	(512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(user_stack, STEST_STACKSIZE);
K_THREAD_STACK_ARRAY_DEFINE(user_stack_array, NUM_STACKS, STEST_STACKSIZE);
K_KERNEL_STACK_DEFINE(kern_stack, STEST_STACKSIZE);
K_KERNEL_STACK_ARRAY_DEFINE(kern_stack_array, NUM_STACKS, STEST_STACKSIZE);

struct foo {
	int bar;

	K_KERNEL_STACK_MEMBER(stack, STEST_STACKSIZE);
	int baz;
};

__kstackmem struct foo stest_member_stack;

void z_impl_stack_info_get(char **start_addr, size_t *size)
{
	*start_addr = (char *)k_current_get()->stack_info.start;
	*size = k_current_get()->stack_info.size;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_stack_info_get(char **start_addr,
					 size_t *size)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(start_addr, sizeof(uintptr_t)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(size, sizeof(size_t)));

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

/* Global data structure with object information, used by
 * stack_buffer_scenarios
 */
ZTEST_BMEM struct scenario_data {
	k_thread_stack_t *stack;

#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	k_thread_stack_t *stack_mapped;
#endif

	/* If this was declared with K_THREAD_STACK_DEFINE and not
	 * K_KERNEL_STACK_DEFINE
	 */
	bool is_user;

	/* Stack size stored in kernel object metadata if a user stack */
	size_t metadata_size;

	/* Return value of sizeof(stack) */
	size_t object_size;

	/* Return value of K_{THREAD|KERNEL}_STACK_SIZEOF(stack) */
	size_t reported_size;

	/* Original size argument passed to K_{THREAD|KERNEL}_STACK_DECLARE */
	size_t declared_size;

	/* Whether this stack is part of an array of thread stacks */
	bool is_array;
} scenario_data;

void stack_buffer_scenarios(void)
{
	k_thread_stack_t *stack_obj;
	size_t obj_size = scenario_data.object_size;
	size_t stack_size, unused, carveout, reserved, alignment, adjusted;
	uint8_t val = 0;
	char *stack_start, *stack_ptr, *stack_end, *obj_start, *obj_end;
	char *stack_buf;
	volatile char *pos;
	int ret, expected;
	uintptr_t base;
	bool is_usermode;
	long int end_space;

#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	stack_obj = scenario_data.stack_mapped;
#else
	stack_obj = scenario_data.stack;
#endif

	base = (uintptr_t)stack_obj;

#ifdef CONFIG_USERSPACE
	is_usermode = arch_is_user_context();
#else
	is_usermode = false;
#endif
	/* Dump interesting information */
	stack_info_get(&stack_start, &stack_size);
	printk("   - Thread reports buffer %p size %zu\n", stack_start,
	       stack_size);

#ifdef CONFIG_USERSPACE
	if (scenario_data.is_user) {
		reserved = K_THREAD_STACK_RESERVED;
		stack_buf = K_THREAD_STACK_BUFFER(stack_obj);
		/* always use the original size here */
		alignment = Z_THREAD_STACK_OBJ_ALIGN(STEST_STACKSIZE);
	} else
#endif
	{
		reserved = K_KERNEL_STACK_RESERVED;
		stack_buf = K_KERNEL_STACK_BUFFER(stack_obj);
		alignment = Z_KERNEL_STACK_OBJ_ALIGN;
	}

	stack_end = stack_start + stack_size;
	obj_end = (char *)stack_obj + obj_size;
	obj_start = (char *)stack_obj;



	/* Assert that the created stack object, with the reserved data
	 * removed, can hold a thread buffer of STEST_STACKSIZE
	 */
	zassert_true(STEST_STACKSIZE <= (obj_size - reserved),
		      "bad stack size in object");

	/* Check that the stack info in the thread marks a region
	 * completely contained within the stack object
	 */
	zassert_true(stack_end <= obj_end,
		     "stack size in thread struct out of bounds (overflow)");
	zassert_true(stack_start >= obj_start,
		     "stack size in thread struct out of bounds (underflow)");

	/* Check that the base of the stack is aligned properly. */
	zassert_true(base % alignment == 0,
		     "stack base address %p not aligned to %zu",
		     stack_obj, alignment);

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
	 * Starting from &val which is close enough to stack pointer
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

		/*
		 * The reserved area, when it exists, is dropped at run time
		 * when transitioning to user mode on RISC-V. Reinstate that
		 * reserved area here for the next tests to work properly
		 * with a static non-zero K_THREAD_STACK_RESERVED definition.
		 */
		if (IS_ENABLED(CONFIG_RISCV) &&
		    IS_ENABLED(CONFIG_GEN_PRIV_STACKS) &&
		    K_THREAD_STACK_RESERVED != 0) {
			stack_start += reserved;
			stack_size -= reserved;
		}

		zassert_true(stack_size <= obj_size - reserved,
			      "bad stack size %zu in thread struct",
			      stack_size);
	}
#endif

	carveout = stack_start - stack_buf;
	printk("   - Carved-out space in buffer: %zu\n", carveout);

	zassert_true(carveout < stack_size,
		     "Suspicious carve-out space reported");
	/* 0 unless this is a stack array */
	end_space = obj_end - stack_end;
	printk("   - Unused objects space: %ld\n", end_space);

	/* For all stacks, when k_thread_create() is called with a stack object,
	 * it is equivalent to pass either the original requested stack size, or
	 * the return value of K_*_STACK_SIZEOF() for that stack object.
	 *
	 * When the stack is actually instantiated, both expand to fill any space
	 * rounded up, except rounding space for array members.
	 */
	if (!scenario_data.is_array) {
		/* These should be exactly the same. We have an equivalence relation:
		 * For some stack declared with:
		 *
		 * K_THREAD_STACK_DEFINE(my_stack, X);
		 * K_THREAD_STACK_LEN(X) - K_THREAD_STACK_RESERVED ==
		 * 	K_THREAD_STACK_SIZEOF(my_stack)
		 *
		 * K_KERNEL_STACK_DEFINE(my_kern_stack, Y):
		 * K_KERNEL_STACK_LEN(Y) - K_KERNEL_STACK_RESERVED ==
		 *	K_KERNEL_STACK_SIZEOF(my_stack)
		 */
#ifdef CONFIG_USERSPACE
		/* Not defined if user mode disabled, all stacks are kernel stacks */
		if (scenario_data.is_user) {
			adjusted = K_THREAD_STACK_LEN(scenario_data.declared_size);
		} else
#endif
		{
			adjusted = K_KERNEL_STACK_LEN(scenario_data.declared_size);
		}
		adjusted -= reserved;

		zassert_equal(end_space, 0, "unexpected unused space\n");
	} else {
		/* For arrays there may be unused space per-object. This is because
		 * every single array member must be aligned to the value returned
		 * by Z_{KERNEL|THREAD}_STACK_OBJ_ALIGN.
		 *
		 * If we define:
		 *
		 * K_{THREAD|KERNEL}_STACK_ARRAY_DEFINE(my_stack_array, num_stacks, X);
		 *
		 * We do not auto-expand usable space to cover this unused area. Doing
		 * this would require some way for the kernel to know that a stack object
		 * pointer passed in is an array member, which is currently not possible.
		 *
		 * The equivalence here is computable with:
		 * K_THREAD_STACK_SIZEOF(my_stack_array[0]) ==
		 * 	K_THREAD_STACK_LEN(X) - K_THREAD_STACK_RESERVED;
		 */

		if (scenario_data.is_user) {
			adjusted = K_THREAD_STACK_LEN(scenario_data.declared_size);
		} else {
			adjusted = K_KERNEL_STACK_LEN(scenario_data.declared_size);
		}
		adjusted -= reserved;

		/* At least make sure it's not negative, that means stack_info isn't set
		 * right
		 */
		zassert_true(end_space >= 0, "bad stack bounds in stack_info");
	}

	zassert_true(adjusted == scenario_data.reported_size,
		     "size mismatch: adjusted %zu vs. reported %zu",
		     adjusted, scenario_data.reported_size);

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
	bool drop = (bool)p1;

	if (drop) {
		k_thread_user_mode_enter(stest_thread_entry, (void *)false,
					 p2, p3);
	} else {
		stack_buffer_scenarios();
	}
}

void stest_thread_launch(uint32_t flags, bool drop)
{
	int ret;
	size_t unused;

	k_thread_create(&test_thread, scenario_data.stack, STEST_STACKSIZE,
			stest_thread_entry,
			(void *)drop, NULL, NULL,
			-1, flags, K_FOREVER);

#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	scenario_data.stack_mapped = test_thread.stack_info.mapped.addr;

	printk("   - Memory mapped stack object %p\n", scenario_data.stack_mapped);
#endif /* CONFIG_THREAD_STACK_MEM_MAPPED */

	k_thread_start(&test_thread);
	k_thread_join(&test_thread, K_FOREVER);

	ret = k_thread_stack_space_get(&test_thread, &unused);

#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
	if (ret == -EINVAL) {
		printk("! cannot report unused stack space due to stack no longer mapped.\n");
	} else
#endif /* CONFIG_THREAD_STACK_MEM_MAPPED */
	{
		zassert_equal(ret, 0, "failed to calculate unused stack space\n");
		printk("target thread unused stack space: %zu\n", unused);
	}
}

void scenario_entry(void *stack_obj, size_t obj_size, size_t reported_size,
		    size_t declared_size, bool is_array)
{
	bool is_user;
	size_t metadata_size;

#ifdef CONFIG_USERSPACE
	struct k_object *zo;

	zo = k_object_find(stack_obj);
	if (zo != NULL) {
		is_user = true;
#ifdef CONFIG_GEN_PRIV_STACKS
		metadata_size = zo->data.stack_data->size;
#else
		metadata_size = zo->data.stack_size;
#endif /* CONFIG_GEN_PRIV_STACKS */
		printk("stack may host user thread, size in metadata is %zu\n",
		       metadata_size);
	} else
#endif /* CONFIG_USERSPACE */
	{
		metadata_size = 0;
		is_user = false;
	}

	scenario_data.stack = stack_obj;
	scenario_data.object_size = obj_size;
	scenario_data.is_user = is_user;
	scenario_data.metadata_size = metadata_size;
	scenario_data.reported_size = reported_size;
	scenario_data.declared_size = declared_size;
	scenario_data.is_array = is_array;

	printk("Stack object %p[%zu]\n", stack_obj, obj_size);
	printk(" - Testing supervisor mode\n");
	stest_thread_launch(0, false);

#ifdef CONFIG_USERSPACE
	if (is_user) {
		printk(" - Testing user mode (direct launch)\n");
		stest_thread_launch(K_USER | K_INHERIT_PERMS, false);
		printk(" - Testing user mode (drop)\n");
		stest_thread_launch(K_INHERIT_PERMS, true);
	}
#endif /* CONFIG_USERSPACE */
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
ZTEST(userspace_thread_stack, test_stack_buffer)
{
	printk("Reserved space (thread stacks): %zu\n",
	       K_THREAD_STACK_RESERVED);
	printk("Reserved space (kernel stacks): %zu\n",
	       K_KERNEL_STACK_RESERVED);

	printk("CONFIG_ISR_STACK_SIZE %zu\n", (size_t)CONFIG_ISR_STACK_SIZE);

	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		printk("irq stack %d: %p size %zu\n",
		       i, &z_interrupt_stacks[i],
		       sizeof(z_interrupt_stacks[i]));
	}

	printk("Provided stack size: %u\n", STEST_STACKSIZE);

	printk("\ntesting user_stack\n");
	scenario_entry(user_stack, sizeof(user_stack), K_THREAD_STACK_SIZEOF(user_stack),
		       STEST_STACKSIZE, false);

	for (int i = 0; i < NUM_STACKS; i++) {
		printk("\ntesting user_stack_array[%d]\n", i);
		scenario_entry(user_stack_array[i],
			       sizeof(user_stack_array[i]),
			       K_THREAD_STACK_SIZEOF(user_stack_array[i]),
			       STEST_STACKSIZE, true);
	}

	printk("\ntesting kern_stack\n");
	scenario_entry(kern_stack, sizeof(kern_stack), K_KERNEL_STACK_SIZEOF(kern_stack),
		       STEST_STACKSIZE, false);

	for (int i = 0; i < NUM_STACKS; i++) {
		printk("\ntesting kern_stack_array[%d]\n", i);
		scenario_entry(kern_stack_array[i],
			       sizeof(kern_stack_array[i]),
			       K_KERNEL_STACK_SIZEOF(kern_stack_array[i]),
			       STEST_STACKSIZE, true);
	}

	printk("\ntesting stest_member_stack\n");
	scenario_entry(&stest_member_stack.stack,
		       sizeof(stest_member_stack.stack),
		       K_KERNEL_STACK_SIZEOF(stest_member_stack.stack),
		       STEST_STACKSIZE, false);
}

void no_op_entry(void *p1, void *p2, void *p3)
{

	printk("hi! bye!\n");

#ifdef CONFIG_DYNAMIC_OBJECTS
	/* Allocate a dynamic kernel object, which gets freed on thread
	 * cleanup since this thread has the only reference.
	 */
	struct k_sem *dyn_sem = k_object_alloc(K_OBJ_SEM);
	k_sem_init(dyn_sem, 1, 1);
	printk("allocated semaphore %p\n", dyn_sem);
#endif
	/* thread self-aborts, triggering idle thread cleanup */
}

/**
 * @brief Show that the idle thread stack size is correct
 *
 * The idle thread has to occasionally clean up self-exiting threads.
 * Exercise this and show that we didn't overflow, reporting out stack
 * usage.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_thread_stack, test_idle_stack)
{
	if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		/* Stacks on coherence platforms aren't coherent, and
		 * the idle stack may have been initialized on a
		 * different CPU!
		 */
		ztest_test_skip();
	}

	int ret;
#ifdef CONFIG_SMP
	/* 1cpu test case, so all other CPUs are spinning with co-op
	 * threads blocking them. _current_cpu triggers an assertion.
	 */
	struct k_thread *idle = arch_curr_cpu()->idle_thread;
#else
	struct k_thread *idle = _current_cpu->idle_thread;
#endif
	size_t unused_bytes;

	/* Spwawn a child thread which self-exits */
	k_thread_create(&test_thread, kern_stack, STEST_STACKSIZE,
			no_op_entry,
			NULL, NULL, NULL,
			-1, 0, K_NO_WAIT);

	k_thread_join(&test_thread, K_FOREVER);

	/* Also sleep for a bit, which also exercises the idle thread
	 * in case some PM hooks will run
	 */
	k_sleep(K_MSEC(1));

	/* Now measure idle thread stack usage */
	ret = k_thread_stack_space_get(idle, &unused_bytes);
	zassert_true(ret == 0, "failed to obtain stack space");
	zassert_true(unused_bytes > 0, "idle thread stack size %d too low",
		     CONFIG_IDLE_STACK_SIZE);
	printk("unused idle thread stack size: %zu/%d (%zu used)\n",
	       unused_bytes, CONFIG_IDLE_STACK_SIZE,
	       CONFIG_IDLE_STACK_SIZE - unused_bytes);

}

void *thread_setup(void)
{
	k_thread_system_pool_assign(k_current_get());

	return NULL;
}

ZTEST_SUITE(userspace_thread_stack, NULL, thread_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

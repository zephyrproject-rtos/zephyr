/*
 * Parts derived from tests/kernel/fatal/src/main.c, which has the
 * following copyright and license:
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel_structs.h>
#include <string.h>
#include <stdlib.h>
#include <app_memory/app_memdomain.h>
#include <sys/util.h>
#include <debug/stack.h>
#include <syscall_handler.h>
#include "test_syscall.h"

#if defined(CONFIG_ARC)
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#endif

#if defined(CONFIG_ARM)
extern void arm_core_mpu_disable(void);
#endif

#define INFO(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define PIPE_LEN 1
#define BYTES_TO_READ_WRITE 1
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

K_SEM_DEFINE(uthread_start_sem, 0, 1);
K_SEM_DEFINE(uthread_end_sem, 0, 1);
K_SEM_DEFINE(test_revoke_sem, 0, 1);
K_SEM_DEFINE(expect_fault_sem, 0, 1);

/*
 * Create partitions. part0 is for all variables to run
 * ztest and this test suite. part1 is for
 * subsequent test specifically for this new implementation.
 */
FOR_EACH(K_APPMEM_PARTITION_DEFINE, part0, part1);

/*
 * Create memory domains. dom0 is for the ztest and this
 * test suite, specifically. dom1 is for a specific test
 * in this test suite.
 */
struct k_mem_domain dom0;
struct k_mem_domain dom1;

K_APP_DMEM(part0) static volatile bool give_uthread_end_sem;
K_APP_DMEM(part0) bool mem_access_check;

K_APP_BMEM(part0) static volatile bool expect_fault;

K_APP_BMEM(part0) static volatile unsigned int expected_reason;

/*
 * We need something that can act as a memory barrier
 * from usermode threads to ensure expect_fault and
 * expected_reason has been updated.  We'll just make an
 * arbitrary system call to force one.
 */
#define BARRIER() k_sem_give(&expect_fault_sem)

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	INFO("Caught system error -- reason %d\n", reason);

	if (expect_fault && expected_reason == reason) {
		/*
		 * If there is a user thread waiting for notification to exit,
		 * give it that notification.
		 */
		if (give_uthread_end_sem) {
			give_uthread_end_sem = false;
			k_sem_give(&uthread_end_sem);
		}
		expect_fault = false;
		expected_reason = 0;
		BARRIER();
		ztest_test_pass();
	} else {
		printk("Unexpected fault during test");
		k_fatal_halt(reason);
	}
}

/**
 * @brief Test to check if the thread is in user mode
 *
 * @ingroup kernel_memprotect_tests
 */
static void is_usermode(void)
{
	/* Confirm that we are in fact running in user mode. */
	expect_fault = false;
	BARRIER();
	zassert_true(_is_user_context(), "thread left in kernel mode");
}

/**
 * @brief Test to write to a control register
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_control(void)
{
	/* Try to write to a control register. */
#if defined(CONFIG_X86)
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0xfffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
	zassert_unreachable("Write to control register did not fault");
#elif defined(CONFIG_ARM)
	unsigned int msr_value;

	expect_fault = false;
	BARRIER();
	msr_value = __get_CONTROL();
	msr_value &= ~(CONTROL_nPRIV_Msk);
	__set_CONTROL(msr_value);
	__DSB();
	__ISB();
	msr_value = __get_CONTROL();
	zassert_true((msr_value & (CONTROL_nPRIV_Msk)),
		     "Write to control register was successful");
#elif defined(CONFIG_ARC)
	unsigned int er_status;

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	/* _ARC_V2_ERSTATUS is privilege aux reg */
	__asm__ volatile (
		"lr %0, [0x402]\n"
		: "=r" (er_status)::
	);
#else
#error "Not implemented for this architecture"
	zassert_unreachable("Write to control register did not fault");
#endif
}

/**
 * @brief Test to disable memory protection
 *
 * @ingroup kernel_memprotect_tests
 */
static void disable_mmu_mpu(void)
{
	/* Try to disable memory protections. */
#if defined(CONFIG_X86)
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0x7ffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
#elif defined(CONFIG_ARM)
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	arm_core_mpu_disable();
#elif defined(CONFIG_ARC)
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	arc_core_mpu_disable();
#else
#error "Not implemented for this architecture"
#endif
	zassert_unreachable("Disable MMU/MPU did not fault");
}

/**
 * @brief Test to read from kernel RAM
 *
 * @ingroup kernel_memprotect_tests
 */
static void read_kernram(void)
{
	/* Try to read from kernel RAM. */
	void *p;

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	p = _current->init_data;
	printk("%p\n", p);
	zassert_unreachable("Read from kernel RAM did not fault");
}

/**
 * @brief Test to write to kernel RAM
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_kernram(void)
{
	/* Try to write to kernel RAM. */
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	_current->init_data = NULL;
	zassert_unreachable("Write to kernel RAM did not fault");
}

extern int _k_neg_eagain;

#include <linker/linker-defs.h>

/**
 * @brief Test to write kernel RO
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_kernro(void)
{
	/* Try to write to kernel RO. */
	const char *const ptr = (const char *const)&_k_neg_eagain;

	zassert_true(ptr < _image_rodata_end &&
		     ptr >= _image_rodata_start,
		     "_k_neg_eagain is not in rodata");
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	_k_neg_eagain = -EINVAL;
	zassert_unreachable("Write to kernel RO did not fault");
}

/**
 * @brief Test to write to kernel text section
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_kerntext(void)
{
	/* Try to write to kernel text. */
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	memset(&z_is_thread_essential, 0, 4);
	zassert_unreachable("Write to kernel text did not fault");
}

static int kernel_data;

/**
 * @brief Testto read from kernel data section
 *
 * @ingroup kernel_memprotect_tests
 */
static void read_kernel_data(void)
{
	/* Try to read from embedded kernel data. */
	int value;

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	value = kernel_data;
	printk("%d\n", value);
	zassert_unreachable("Read from data did not fault");
}

/**
 * @brief Test to write to kernel data section
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_kernel_data(void)
{
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	kernel_data = 1;
	zassert_unreachable("Write to  data did not fault");
}

/*
 * volatile to avoid compiler mischief.
 */
K_APP_DMEM(part0) volatile int *priv_stack_ptr;
#if defined(CONFIG_X86)
/*
 * We can't inline this in the code or make it static
 * or local without triggering a warning on -Warray-bounds.
 */
K_APP_DMEM(part0) size_t size = MMU_PAGE_SIZE;
#elif defined(CONFIG_ARC)
K_APP_DMEM(part0) s32_t size = (0 - CONFIG_PRIVILEGED_STACK_SIZE -
			       STACK_GUARD_SIZE);
#endif

/**
 * @brief Test to read provileged stack
 *
 * @ingroup kernel_memprotect_tests
 */
static void read_priv_stack(void)
{
	/* Try to read from privileged stack. */
#if defined(CONFIG_X86) || defined(CONFIG_ARC)
	int s[1];

	s[0] = 0;
	priv_stack_ptr = &s[0];
	priv_stack_ptr = (int *)((unsigned char *)priv_stack_ptr - size);
#elif defined(CONFIG_ARM)
	/* priv_stack_ptr set by test_main() */
#else
#error "Not implemented for this architecture"
#endif
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	printk("%d\n", *priv_stack_ptr);
	zassert_unreachable("Read from privileged stack did not fault");
}

/**
 * @brief Test to write to privilege stack
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_priv_stack(void)
{
	/* Try to write to privileged stack. */
#if defined(CONFIG_X86) || defined(CONFIG_ARC)
	int s[1];

	s[0] = 0;
	priv_stack_ptr = &s[0];
	priv_stack_ptr = (int *)((unsigned char *)priv_stack_ptr - size);
#elif defined(CONFIG_ARM)
	/* priv_stack_ptr set by test_main() */
#else
#error "Not implemented for this architecture"
#endif
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	*priv_stack_ptr = 42;
	zassert_unreachable("Write to privileged stack did not fault");
}


K_APP_BMEM(part0) static struct k_sem sem;

/**
 * @brief Test to pass a user object to system call
 *
 * @ingroup kernel_memprotect_tests
 */
static void pass_user_object(void)
{
	/* Try to pass a user object to a system call. */
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_sem_init(&sem, 0, 1);
	zassert_unreachable("Pass a user object to a syscall did not fault");
}

static struct k_sem ksem;

/**
 * @brief Test to pass object to a system call without permissions
 *
 * @ingroup kernel_memprotect_tests
 */
static void pass_noperms_object(void)
{
	/* Try to pass a object to a system call w/o permissions. */
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_sem_init(&ksem, 0, 1);
	zassert_unreachable("Pass an unauthorized object to a "
			    "syscall did not fault");
}

struct k_thread kthread_thread;

K_THREAD_STACK_DEFINE(kthread_stack, STACKSIZE);

void thread_body(void)
{
}

/**
 * @brief Test to start kernel thread from usermode
 *
 * @ingroup kernel_memprotect_tests
 */
static void start_kernel_thread(void)
{
	/* Try to start a kernel thread from a usermode thread */
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_thread_create(&kthread_thread, kthread_stack, STACKSIZE,
			(k_thread_entry_t)thread_body, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_INHERIT_PERMS,
			K_NO_WAIT);
	zassert_unreachable("Create a kernel thread did not fault");
}

struct k_thread uthread_thread;
K_THREAD_STACK_DEFINE(uthread_stack, STACKSIZE);

static void uthread_body(void)
{
	/* Notify our creator that we are alive. */
	k_sem_give(&uthread_start_sem);
	/* Request notification of when we should exit. */
	give_uthread_end_sem = true;
	/* Wait until notified by the fault handler or by the creator. */
	k_sem_take(&uthread_end_sem, K_FOREVER);
}

/**
 * @brief Test to read from another thread's stack
 *
 * @ingroup kernel_memprotect_tests
 */
static void read_other_stack(void)
{
	/* Try to read from another thread's stack. */
	unsigned int *ptr;

	k_thread_create(&uthread_thread, uthread_stack, STACKSIZE,
			(k_thread_entry_t)uthread_body, NULL, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* Ensure that the other thread has begun. */
	k_sem_take(&uthread_start_sem, K_FOREVER);

	/* Try to directly read the stack of the other thread. */
	ptr = (unsigned int *)Z_THREAD_STACK_BUFFER(uthread_stack);
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	printk("%u\n", *ptr);

	/* Shouldn't be reached, but if so, let the other thread exit */
	if (give_uthread_end_sem) {
		give_uthread_end_sem = false;
		k_sem_give(&uthread_end_sem);
	}
	zassert_unreachable("Read from other thread stack did not fault");
}

/**
 * @brief Test to write to other thread's stack
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_other_stack(void)
{
	/* Try to write to another thread's stack. */
	unsigned int *ptr;

	k_thread_create(&uthread_thread, uthread_stack, STACKSIZE,
			(k_thread_entry_t)uthread_body, NULL, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* Ensure that the other thread has begun. */
	k_sem_take(&uthread_start_sem, K_FOREVER);

	/* Try to directly write the stack of the other thread. */
	ptr = (unsigned int *) Z_THREAD_STACK_BUFFER(uthread_stack);
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	*ptr = 0U;

	/* Shouldn't be reached, but if so, let the other thread exit */
	if (give_uthread_end_sem) {
		give_uthread_end_sem = false;
		k_sem_give(&uthread_end_sem);
	}
	zassert_unreachable("Write to other thread stack did not fault");
}

/**
 * @brief Test to revoke access to kobject without permission
 *
 * @ingroup kernel_memprotect_tests
 */
static void revoke_noperms_object(void)
{
	/* Attempt to revoke access to kobject w/o permissions*/
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_object_release(&ksem);

	zassert_unreachable("Revoke access to unauthorized object "
			    "did not fault");
}

/**
 * @brief Test to access object after revoking access
 *
 * @ingroup kernel_memprotect_tests
 */
static void access_after_revoke(void)
{
	k_object_release(&test_revoke_sem);

	/* Try to access an object after revoking access to it */
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_sem_take(&test_revoke_sem, K_NO_WAIT);

	zassert_unreachable("Using revoked object did not fault");
}

static void umode_enter_func(void)
{
	if (_is_user_context()) {
		/*
		 * Have to explicitly call ztest_test_pass() because
		 * k_thread_user_mode_enter() does not return.  We have
		 * to signal a pass status or else run_test() will hang
		 * forever waiting on test_end_signal semaphore.
		 */
		ztest_test_pass();
	} else {
		zassert_unreachable("Thread did not enter user mode");
	}
}

/**
 * @brief Test to check enter to usermode
 *
 * @ingroup kernel_memprotect_tests
 */
static void user_mode_enter(void)
{
	expect_fault = false;
	BARRIER();
	k_thread_user_mode_enter((k_thread_entry_t)umode_enter_func,
				 NULL, NULL, NULL);
}

/* Define and initialize pipe. */
K_PIPE_DEFINE(kpipe, PIPE_LEN, BYTES_TO_READ_WRITE);
K_APP_BMEM(part0) static size_t bytes_written_read;

/**
 * @brief Test to write to kobject using pipe
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_kobject_user_pipe(void)
{
	/*
	 * Attempt to use system call from k_pipe_get to write over
	 * a kernel object.
	 */
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_pipe_get(&kpipe, &uthread_start_sem, BYTES_TO_READ_WRITE,
		   &bytes_written_read, 1, K_NO_WAIT);

	zassert_unreachable("System call memory write validation "
			    "did not fault");
}

/**
 * @brief Test to read from kobject using pipe
 *
 * @ingroup kernel_memprotect_tests
 */
static void read_kobject_user_pipe(void)
{
	/*
	 * Attempt to use system call from k_pipe_put to read a
	 * kernel object.
	 */
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;
	BARRIER();
	k_pipe_put(&kpipe, &uthread_start_sem, BYTES_TO_READ_WRITE,
		   &bytes_written_read, 1, K_NO_WAIT);

	zassert_unreachable("System call memory read validation "
			    "did not fault");
}

/* Create bool in part1 partitions */
K_APP_DMEM(part1) bool thread_bool;

static void shared_mem_thread(void)
{
	/*
	 * Try to access thread_bool_1 in denied memory
	 * domain.
	 */
	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	BARRIER();
	thread_bool = false;
	zassert_unreachable("Thread accessed global in other "
			    "memory domain\n");
}

/**
 * @brief Test to access other memory domain
 *
 * @ingroup kernel_memprotect_tests
 */
static void access_other_memdomain(void)
{
	struct k_mem_partition *parts[] = {&part0};
	/*
	 * Following tests the ability for a thread to access data
	 * in a domain that it is denied.
	 */

	k_mem_domain_init(&dom1, ARRAY_SIZE(parts), parts);

	/* remove current thread from domain dom0 and add to dom1 */
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&dom1, k_current_get());

	/* Create user mode thread */
	k_thread_create(&uthread_thread, uthread_stack, STACKSIZE,
			(k_thread_entry_t)shared_mem_thread, NULL,
			NULL, NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_abort(k_current_get());

}


#if defined(CONFIG_ARM)
extern u8_t *z_priv_stack_find(void *obj);
extern k_thread_stack_t ztest_thread_stack[];
#endif

struct k_mem_domain add_thread_drop_dom;
struct k_mem_domain add_part_drop_dom;
struct k_mem_domain remove_thread_drop_dom;
struct k_mem_domain remove_part_drop_dom;

struct k_mem_domain add_thread_ctx_dom;
struct k_mem_domain add_part_ctx_dom;
struct k_mem_domain remove_thread_ctx_dom;
struct k_mem_domain remove_part_ctx_dom;

K_APPMEM_PARTITION_DEFINE(access_part);
K_APP_BMEM(access_part) volatile bool test_bool;

static void user_half(void *arg1, void *arg2, void *arg3)
{
	test_bool = 1;
	if (!expect_fault) {
		ztest_test_pass();
	} else {
		ztest_test_fail();
	}
}

/**
 * Show that changing between memory domains and dropping to user mode works
 * as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_add_thread_drop_to_user(void)
{
	struct k_mem_partition *parts[] = {&part0, &access_part,
					   &ztest_mem_partition};

	expect_fault = false;
	k_mem_domain_init(&add_thread_drop_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_add_thread(&add_thread_drop_dom, k_current_get());

	k_thread_user_mode_enter(user_half, NULL, NULL, NULL);
}

/* Show that adding a partition to a domain and then dropping to user mode
 * works as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_add_part_drop_to_user(void)
{
	struct k_mem_partition *parts[] = {&part0, &ztest_mem_partition};

	expect_fault = false;
	k_mem_domain_init(&add_part_drop_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&add_part_drop_dom, k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_add_partition(&add_part_drop_dom, &access_part);

	k_thread_user_mode_enter(user_half, NULL, NULL, NULL);
}

/* Show that self-removing from a memory domain and then dropping to user
 * mode faults as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_remove_thread_drop_to_user(void)
{
	struct k_mem_partition *parts[] = {&part0, &access_part,
					   &ztest_mem_partition};

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	k_mem_domain_init(&remove_thread_drop_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&remove_thread_drop_dom, k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_remove_thread(k_current_get());

	k_thread_user_mode_enter(user_half, NULL, NULL, NULL);
}

/**
 * Show that self-removing a partition from a domain we are a membed of,
 * and then dropping to user mode faults as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_remove_part_drop_to_user(void)
{
	struct k_mem_partition *parts[] = {&part0, &access_part,
					   &ztest_mem_partition};

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	k_mem_domain_init(&remove_part_drop_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&remove_part_drop_dom, k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_remove_partition(&remove_part_drop_dom, &access_part);

	k_thread_user_mode_enter(user_half, NULL, NULL, NULL);
}

static void user_ctx_switch_half(void *arg1, void *arg2, void *arg3)
{
	test_bool = 1;
	k_sem_give(&uthread_end_sem);
}

static void spawn_user(void)
{
	k_sem_reset(&uthread_end_sem);
	k_object_access_grant(&uthread_end_sem, k_current_get());

	k_thread_create(&kthread_thread, kthread_stack, STACKSIZE,
			user_ctx_switch_half, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_INHERIT_PERMS | K_USER,
			K_NO_WAIT);

	k_sem_take(&uthread_end_sem, K_FOREVER);
	if (expect_fault) {
		ztest_test_fail();
	}
}

/**
 * Show that changing between memory domains and then switching to another
 * thread in the same domain works as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_add_thread_context_switch(void)
{
	struct k_mem_partition *parts[] = {&part0, &access_part,
					   &ztest_mem_partition};

	expect_fault = false;
	k_mem_domain_init(&add_thread_ctx_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_add_thread(&add_thread_ctx_dom, k_current_get());

	spawn_user();
}

/* Show that adding a partition to a domain and then switching to another
 * user thread in the same domain works as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_add_part_context_switch(void)
{
	struct k_mem_partition *parts[] = {&part0, &ztest_mem_partition};

	expect_fault = false;
	k_mem_domain_init(&add_part_ctx_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&add_part_ctx_dom, k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_add_partition(&add_part_ctx_dom, &access_part);

	spawn_user();
}

/* Show that self-removing from a memory domain and then switching to another
 * user thread in the same domain faults as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_remove_thread_context_switch(void)
{
	struct k_mem_partition *parts[] = {&part0, &access_part,
					   &ztest_mem_partition};

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	k_mem_domain_init(&remove_thread_ctx_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&remove_thread_ctx_dom, k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_remove_thread(k_current_get());

	spawn_user();
}

/**
 * Show that self-removing a partition from a domain we are a member of,
 * and then switching to another user thread in the same domain faults as
 * expected.
 *
 * @ingroup kernel_memprotect_tests
 */
static void domain_remove_part_context_switch(void)
{
	struct k_mem_partition *parts[] = {&part0, &access_part,
					   &ztest_mem_partition};

	expect_fault = true;
	expected_reason = K_ERR_CPU_EXCEPTION;
	k_mem_domain_init(&remove_part_ctx_dom, ARRAY_SIZE(parts), parts);
	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_add_thread(&remove_part_ctx_dom, k_current_get());

	k_sleep(K_MSEC(1));
	k_mem_domain_remove_partition(&remove_part_ctx_dom, &access_part);

	spawn_user();
}

/*
 * Stack testing
 */

#define NUM_STACKS	3
#define STEST_STACKSIZE	(1024 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_DEFINE(stest_stack, STEST_STACKSIZE);
K_THREAD_STACK_ARRAY_DEFINE(stest_stack_array, NUM_STACKS, STEST_STACKSIZE);

struct foo {
	int bar;
	K_THREAD_STACK_MEMBER(stack, STEST_STACKSIZE);
	int baz;
};

struct foo stest_member_stack;

void z_impl_stack_info_get(u32_t *start_addr, u32_t *size)
{
	*start_addr = k_current_get()->stack_info.start;
	*size = k_current_get()->stack_info.size;
}

static inline void z_vrfy_stack_info_get(u32_t *start_addr, u32_t *size)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(start_addr, sizeof(u32_t)));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(size, sizeof(u32_t)));

	z_impl_stack_info_get((u32_t *)start_addr, (u32_t *)size);
}
#include <syscalls/stack_info_get_mrsh.c>

int z_impl_check_perms(void *addr, size_t size, int write)
{
	return z_arch_buffer_validate(addr, size, write);
}

static inline int z_vrfy_check_perms(void *addr, size_t size, int write)
{
	return z_impl_check_perms((void *)addr, size, write);
}
#include <syscalls/check_perms_mrsh.c>

void stack_buffer_scenarios(k_thread_stack_t *stack_obj, size_t obj_size)
{
	size_t stack_size;
	u8_t val;
	char *stack_start, *stack_ptr, *stack_end, *obj_start, *obj_end;
	volatile char *pos;

	expect_fault = false;


	/* Dump interesting information */

	stack_info_get((u32_t *)&stack_start, (u32_t *)&stack_size);
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

	if (z_arch_is_user_context()) {
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
		zassert_true(check_perms(obj_start - 1, 1, 0),
			     "user mode access to memory before start of stack object");
		zassert_true(check_perms(obj_end, 1, 0),
			     "user mode access past end of stack object");
	}


	/* This API is being removed just whine about it for now */
	if (Z_THREAD_STACK_BUFFER(stack_obj) != stack_start) {
		printk("WARNING: Z_THREAD_STACK_BUFFER() reports %p\n",
		       Z_THREAD_STACK_BUFFER(stack_obj));
	}

	if (z_arch_is_user_context()) {
		zassert_true(stack_size <= obj_size - K_THREAD_STACK_RESERVED,
			      "bad stack size in thread struct");
	}


	k_sem_give(&uthread_end_sem);
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

void stest_thread_launch(void *stack_obj, size_t obj_size, u32_t flags,
			 bool drop)
{
	k_thread_create(&uthread_thread, stack_obj, STEST_STACKSIZE,
			stest_thread_entry, stack_obj, (void *)obj_size,
			(void *)drop,
			-1, flags, K_NO_WAIT);
	k_sem_take(&uthread_end_sem, K_FOREVER);

	stack_analyze("test_thread", (char *)uthread_thread.stack_info.start,
		      uthread_thread.stack_info.size);
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

void test_stack_buffer(void)
{
	printk("Reserved space: %u\n", K_THREAD_STACK_RESERVED);
	printk("Provided stack size: %u\n", STEST_STACKSIZE);
	scenario_entry(stest_stack, sizeof(stest_stack));

	for (int i = 0; i < NUM_STACKS; i++) {
		scenario_entry(stest_stack_array[i],
			       sizeof(stest_stack_array[i]));
	}

	scenario_entry(&stest_member_stack.stack,
		       sizeof(stest_member_stack.stack));

}

void z_impl_missing_syscall(void)
{
	/* Shouldn't ever get here; no handler function compiled */
	k_panic();
}

void test_unimplemented_syscall(void)
{
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;

	missing_syscall();
}

void test_bad_syscall(void)
{
	expect_fault = true;
	expected_reason = K_ERR_KERNEL_OOPS;

	z_arch_syscall_invoke0(INT_MAX);

}

static struct k_sem recycle_sem;


void test_object_recycle(void)
{
	struct _k_object *ko;
	int perms_count = 0;

	ko = z_object_find(&recycle_sem);
	(void)memset(ko->perms, 0xFF, sizeof(ko->perms));

	z_object_recycle(&recycle_sem);
	zassert_true(ko != NULL, "kernel object not found");
	zassert_true(ko->flags & K_OBJ_FLAG_INITIALIZED,
		     "object wasn't marked as initialized");

	for (int i = 0; i < CONFIG_MAX_THREAD_BYTES; i++) {
		perms_count += popcount(ko->perms[i]);
	}

	zassert_true(perms_count == 1, "invalid number of thread permissions");
}

#define test_oops(provided, expected) do { \
	expect_fault = true; \
	expected_reason = expected; \
	z_except_reason(provided); \
} while (false)

void test_oops_panic(void)
{
	test_oops(K_ERR_KERNEL_PANIC, K_ERR_KERNEL_OOPS);
}

void test_oops_oops(void)
{
	test_oops(K_ERR_KERNEL_OOPS, K_ERR_KERNEL_OOPS);
}

void test_oops_exception(void)
{
	test_oops(K_ERR_CPU_EXCEPTION, K_ERR_KERNEL_OOPS);
}

void test_oops_maxint(void)
{
	test_oops(INT_MAX, K_ERR_KERNEL_OOPS);
}

void test_oops_stackcheck(void)
{
	test_oops(K_ERR_STACK_CHK_FAIL, K_ERR_STACK_CHK_FAIL);
}

void test_main(void)
{
	struct k_mem_partition *parts[] = {&part0, &part1,
		&ztest_mem_partition};

	k_mem_domain_remove_thread(k_current_get());
	k_mem_domain_init(&dom0, ARRAY_SIZE(parts), parts);
	k_mem_domain_add_thread(&dom0, k_current_get());

#if defined(CONFIG_ARM)
	priv_stack_ptr = (int *)z_priv_stack_find(ztest_thread_stack);

#endif
	k_thread_access_grant(k_current_get(),
			      &kthread_thread, &kthread_stack,
			      &uthread_thread, &uthread_stack,
			      &uthread_start_sem, &uthread_end_sem,
			      &test_revoke_sem, &kpipe, &expect_fault_sem);
	ztest_test_suite(userspace,
			 ztest_user_unit_test(is_usermode),
			 ztest_user_unit_test(write_control),
			 ztest_user_unit_test(disable_mmu_mpu),
			 ztest_user_unit_test(read_kernram),
			 ztest_user_unit_test(write_kernram),
			 ztest_user_unit_test(write_kernro),
			 ztest_user_unit_test(write_kerntext),
			 ztest_user_unit_test(read_kernel_data),
			 ztest_user_unit_test(write_kernel_data),
			 ztest_user_unit_test(read_priv_stack),
			 ztest_user_unit_test(write_priv_stack),
			 ztest_user_unit_test(pass_user_object),
			 ztest_user_unit_test(pass_noperms_object),
			 ztest_user_unit_test(start_kernel_thread),
			 ztest_user_unit_test(read_other_stack),
			 ztest_user_unit_test(write_other_stack),
			 ztest_user_unit_test(revoke_noperms_object),
			 ztest_user_unit_test(access_after_revoke),
			 ztest_unit_test(user_mode_enter),
			 ztest_user_unit_test(write_kobject_user_pipe),
			 ztest_user_unit_test(read_kobject_user_pipe),
			 ztest_unit_test(access_other_memdomain),
			 ztest_unit_test(domain_add_thread_drop_to_user),
			 ztest_unit_test(domain_add_part_drop_to_user),
			 ztest_unit_test(domain_remove_part_drop_to_user),
			 ztest_unit_test(domain_remove_thread_drop_to_user),
			 ztest_unit_test(domain_add_thread_context_switch),
			 ztest_unit_test(domain_add_part_context_switch),
			 ztest_unit_test(domain_remove_part_context_switch),
			 ztest_unit_test(domain_remove_thread_context_switch),
			 ztest_unit_test(test_stack_buffer),
			 ztest_user_unit_test(test_unimplemented_syscall),
			 ztest_user_unit_test(test_bad_syscall),
			 ztest_user_unit_test(test_oops_panic),
			 ztest_user_unit_test(test_oops_oops),
			 ztest_user_unit_test(test_oops_exception),
			 ztest_user_unit_test(test_oops_maxint),
			 ztest_user_unit_test(test_oops_stackcheck),
			 ztest_unit_test(test_object_recycle)
			 );
	ztest_run_test_suite(userspace);
}

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
#include <misc/util.h>

#if defined(CONFIG_ARC)
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#endif

#define INFO(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define PIPE_LEN 1
#define BYTES_TO_READ_WRITE 1

K_SEM_DEFINE(uthread_start_sem, 0, 1);
K_SEM_DEFINE(uthread_end_sem, 0, 1);
K_SEM_DEFINE(test_revoke_sem, 0, 1);
K_SEM_DEFINE(expect_fault_sem, 0, 1);

/*
 * Create partitions. part0 is for all variables to run
 * ztest and this test suite. part1 and part2 are for
 * subsequent test specifically for this new implementation.
 */
FOR_EACH(appmem_partition, part0, part1, part2);

/*
 * Create memory domains. dom0 is for the ztest and this
 * test suite, specifically. dom1 is for a specific test
 * in this test suite.
 */
FOR_EACH(appmem_domain, dom0, dom1);

_app_dmem(part0) static volatile bool give_uthread_end_sem;
_app_dmem(part0) bool mem_access_check;

_app_bmem(part0) static volatile bool expect_fault;

#if defined(CONFIG_X86)
#define REASON_HW_EXCEPTION _NANO_ERR_CPU_EXCEPTION
#define REASON_KERNEL_OOPS  _NANO_ERR_KERNEL_OOPS
#elif defined(CONFIG_ARM)
#define REASON_HW_EXCEPTION _NANO_ERR_HW_EXCEPTION
#define REASON_KERNEL_OOPS  _NANO_ERR_HW_EXCEPTION
#elif defined(CONFIG_ARC)
#define REASON_HW_EXCEPTION _NANO_ERR_HW_EXCEPTION
#define REASON_KERNEL_OOPS  _NANO_ERR_KERNEL_OOPS
#else
#error "Not implemented for this architecture"
#endif
_app_bmem(part0) static volatile unsigned int expected_reason;

/*
 * We need something that can act as a memory barrier
 * from usermode threads to ensure expect_fault and
 * expected_reason has been updated.  We'll just make an
 * arbitrary system call to force one.
 */
#define BARRIER() k_sem_give(&expect_fault_sem)

/* ARM is a special case, in that k_thread_abort() does indeed return
 * instead of calling _Swap() directly. The PendSV exception is queued
 * and immediately fires upon completing the exception path; the faulting
 * thread is never run again.
 */
#if !(defined(CONFIG_ARM) || defined(CONFIG_ARC))
FUNC_NORETURN
#endif
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	INFO("Caught system error -- reason %d\n", reason);
	/*
	 * If there is a user thread waiting for notification to exit,
	 * give it that notification.
	 */
	if (give_uthread_end_sem) {
		give_uthread_end_sem = false;
		k_sem_give(&uthread_end_sem);
	}

	if (expect_fault && expected_reason == reason) {
		expect_fault = false;
		expected_reason = 0;
		BARRIER();
		ztest_test_pass();
	} else {
		zassert_unreachable("Unexpected fault during test");
	}
#if !(defined(CONFIG_ARM) || defined(CONFIG_ARC))
	CODE_UNREACHABLE;
#endif
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
	expected_reason = REASON_HW_EXCEPTION;
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
	__asm__ volatile (
		"mrs %0, CONTROL;\n\t"
		"bic %0, #1;\n\t"
		"msr CONTROL, %0;\n\t"
		"mrs %0, CONTROL;\n\t"
		: "=r" (msr_value)::
	);
	zassert_true((msr_value & 1),
		     "Write to control register was successful");
#elif defined(CONFIG_ARC)
	unsigned int er_status;

	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
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
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0x7ffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
#elif defined(CONFIG_ARM)
	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	arm_core_mpu_disable();
#elif defined(CONFIG_ARC)
	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
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
	expected_reason = REASON_HW_EXCEPTION;
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
	expected_reason = REASON_HW_EXCEPTION;
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
	expected_reason = REASON_HW_EXCEPTION;
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
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	memcpy(&_is_thread_essential, 0, 4);
	zassert_unreachable("Write to kernel text did not fault");
}

__kernel static int kernel_data;

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
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	value = kernel_data;
	printk("%d\n", value);
	zassert_unreachable("Read from __kernel data did not fault");
}

/**
 * @brief Test to write to kernel data section
 *
 * @ingroup kernel_memprotect_tests
 */
static void write_kernel_data(void)
{
	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	kernel_data = 1;
	zassert_unreachable("Write to  __kernel data did not fault");
}

/*
 * volatile to avoid compiler mischief.
 */
_app_dmem(part0) volatile int *priv_stack_ptr;
#if defined(CONFIG_X86)
/*
 * We can't inline this in the code or make it static
 * or local without triggering a warning on -Warray-bounds.
 */
_app_dmem(part0) size_t size = MMU_PAGE_SIZE;
#elif defined(CONFIG_ARC)
_app_dmem(part0) s32_t size = (0 - CONFIG_PRIVILEGED_STACK_SIZE -
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
	expected_reason = REASON_HW_EXCEPTION;
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
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	*priv_stack_ptr = 42;
	zassert_unreachable("Write to privileged stack did not fault");
}


_app_bmem(part0) static struct k_sem sem;

/**
 * @brief Test to pass a user object to system call
 *
 * @ingroup kernel_memprotect_tests
 */
static void pass_user_object(void)
{
	/* Try to pass a user object to a system call. */
	expect_fault = true;
	expected_reason = REASON_KERNEL_OOPS;
	BARRIER();
	k_sem_init(&sem, 0, 1);
	zassert_unreachable("Pass a user object to a syscall did not fault");
}

__kernel static struct k_sem ksem;

/**
 * @brief Test to pass object to a system call without permissions
 *
 * @ingroup kernel_memprotect_tests
 */
static void pass_noperms_object(void)
{
	/* Try to pass a object to a system call w/o permissions. */
	expect_fault = true;
	expected_reason = REASON_KERNEL_OOPS;
	BARRIER();
	k_sem_init(&ksem, 0, 1);
	zassert_unreachable("Pass an unauthorized object to a "
			    "syscall did not fault");
}

__kernel struct k_thread kthread_thread;

#define STACKSIZE 512
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
	expected_reason = REASON_KERNEL_OOPS;
	BARRIER();
	k_thread_create(&kthread_thread, kthread_stack, STACKSIZE,
			(k_thread_entry_t)thread_body, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_INHERIT_PERMS,
			K_NO_WAIT);
	zassert_unreachable("Create a kernel thread did not fault");
}

__kernel struct k_thread uthread_thread;
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
	ptr = (unsigned int *)K_THREAD_STACK_BUFFER(uthread_stack);
	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
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
	ptr = (unsigned int *) K_THREAD_STACK_BUFFER(uthread_stack);
	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
	BARRIER();
	*ptr = 0;

	/* Shouldn't be reached, but if so, let the other thread exit */
	if (give_uthread_end_sem) {
		give_uthread_end_sem = false;
		k_sem_give(&uthread_end_sem);
	}
	zassert_unreachable("Write to other thread stack did not fault");
}

/**
 * @brief Test to revoke acess to kobject without permission
 *
 * @ingroup kernel_memprotect_tests
 */
static void revoke_noperms_object(void)
{
	/* Attempt to revoke access to kobject w/o permissions*/
	expect_fault = true;
	expected_reason = REASON_KERNEL_OOPS;
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
	expected_reason = REASON_KERNEL_OOPS;
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
_app_bmem(part0) static size_t bytes_written_read;

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
	expected_reason = REASON_KERNEL_OOPS;
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
	expected_reason = REASON_KERNEL_OOPS;
	BARRIER();
	k_pipe_put(&kpipe, &uthread_start_sem, BYTES_TO_READ_WRITE,
		   &bytes_written_read, 1, K_NO_WAIT);

	zassert_unreachable("System call memory read validation "
			    "did not fault");
}

/* Removed test for access_non_app_memory
 * due to the APPLICATION_MEMORY variable
 * defaulting to y, when enabled the
 * section app_bss is made available to
 * all threads breaking the test
 */

/* Create bool in part1 partitions */
_app_dmem(part1) bool thread_bool;

static void shared_mem_thread(void)
{
	/*
	 * Try to access thread_bool_1 in denied memory
	 * domain.
	 */
	expect_fault = true;
	expected_reason = REASON_HW_EXCEPTION;
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
	/*
	 * Following tests the ability for a thread to access data
	 * in a domain that it is denied.
	 */

	/* initialize domain dom1 with partition part2 */
	appmem_init_domain_dom1(part2);
	/* add partition part0 for test globals */
	appmem_add_part_dom1(part0);
	/* remove current thread from domain dom0 */
	appmem_rm_thread_dom0(k_current_get());
	/* initialize domain with current thread*/
	appmem_add_thread_dom1(k_current_get());

	/* Create user mode thread */
	k_thread_create(&uthread_thread, uthread_stack, STACKSIZE,
			(k_thread_entry_t)shared_mem_thread, NULL,
			NULL, NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_abort(k_current_get());

}


#if defined(CONFIG_ARM)
extern u8_t *_k_priv_stack_find(void *obj);
extern k_thread_stack_t ztest_thread_stack[];
#endif

void test_main(void)
{
	/* partitions must be initialized first */
	FOR_EACH(appmem_init_part, part0, part1, part2);
	/*
	 * Next, the app_memory must be initialized in order to
	 * calculate size of the dynamically created subsections.
	 */
#if defined(CONFIG_ARC)
	/*
	 * appmem_init_app_memory will accees all partitions
	 * For CONFIG_ARC_MPU_VER == 3, these partiontons are not added
	 * into MPU now, so need to disable mpu first to do app_bss_zero()
	 */
	arc_core_mpu_disable();
	appmem_init_app_memory();
	arc_core_mpu_enable();
#else
	appmem_init_app_memory();
#endif
	/* Domain is initialized with partition part0 */
	appmem_init_domain_dom0(part0);
	/* Next, the partition must be added to the domain */
	appmem_add_part_dom0(part1);
	/* Finally, the current thread is added to domain */
	appmem_add_thread_dom0(k_current_get());

#if defined(CONFIG_ARM)
	priv_stack_ptr = (int *)_k_priv_stack_find(ztest_thread_stack);
#endif
	k_thread_access_grant(k_current_get(),
			      &kthread_thread, &kthread_stack,
			      &uthread_thread, &uthread_stack,
			      &uthread_start_sem, &uthread_end_sem,
			      &test_revoke_sem, &kpipe, &expect_fault_sem,
			      NULL);
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
			 ztest_unit_test(access_other_memdomain)
			 );
	ztest_run_test_suite(userspace);
}

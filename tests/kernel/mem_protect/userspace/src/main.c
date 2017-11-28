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

#define INFO(fmt, ...) printk(fmt, ##__VA_ARGS__)

/* ARM is a special case, in that k_thread_abort() does indeed return
 * instead of calling _Swap() directly. The PendSV exception is queued
 * and immediately fires upon completing the exception path; the faulting
 * thread is never run again.
 */
#ifndef CONFIG_ARM
FUNC_NORETURN
#endif
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	INFO("Caught system error -- reason %d\n", reason);
	ztest_test_pass();
#ifndef CONFIG_ARM
	CODE_UNREACHABLE;
#endif
}

static void is_usermode(void)
{
	/* Confirm that we are in fact running in user mode. */
	zassert_true(_is_user_context(), "thread left in kernel mode\n");
}

static void priv_insn(void)
{
	/* Try to invoke what should require a privileged instruction. */
	unsigned int key = irq_lock();

	irq_unlock(key); /* just in case we succeeded */
	zassert_unreachable("irq_lock did not fault\n");
}

static void write_control(void)
{
	/* Try to write to a control register. */
#if defined(CONFIG_X86)
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0xfffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
#elif defined(CONFIG_ARM)
	__asm__ volatile (
		"mrs r2, CONTROL;\n\t"
		"bic r2, #1;\n\t"
		"msr CONTROL, r2;\n\t"
		);
#else
#error "Not implemented for this architecture"
#endif
	zassert_unreachable("Write to control register did not fault\n");
}

static void disable_mmu_mpu(void)
{
	/* Try to disable memory protections. */
#if defined(CONFIG_X86)
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0x7ffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
#elif defined(CONFIG_ARM)
	arm_core_mpu_disable();
#else
#error "Not implemented for this architecture"
#endif
	zassert_unreachable("Disable MMU/MPU did not fault\n");
}

static void read_kernram(void)
{
	/* Try to read from kernel RAM. */
	void *p = _current->init_data;

	printk("%p\n", p);
	zassert_unreachable("Read from kernel RAM did not fault\n");
}

static void write_kernram(void)
{
	/* Try to write to kernel RAM. */
	_current->init_data = NULL;
	zassert_unreachable("Write to kernel RAM did not fault\n");
}

extern int _k_neg_eagain;

#include <linker/linker-defs.h>

static void write_kernro(void)
{
	/* Try to write to kernel RO. */
	const char *const ptr = (const char *const)&_k_neg_eagain;

	zassert_true(ptr < _image_rodata_end &&
		     ptr >= _image_rodata_start,
		     "_k_neg_eagain is not in rodata\n");
	_k_neg_eagain = -EINVAL;
	zassert_unreachable("Write to kernel RO did not fault\n");
}

static void write_kerntext(void)
{
	/* Try to write to kernel text. */
	memcpy(&_is_thread_essential, 0, 4);
	zassert_unreachable("Write to kernel text did not fault\n");
}

__kernel static int kernel_data;

static void read_kernel_data(void)
{
	/* Try to read from embedded kernel data. */
	int value = kernel_data;

	printk("%d\n", value);
	zassert_unreachable("Read from __kernel data did not fault\n");
}

static void write_kernel_data(void)
{
	kernel_data = 1;
	zassert_unreachable("Write to  __kernel data did not fault\n");
}

/*
 * volatile to avoid compiler mischief.
 */
volatile int *ptr = NULL;
#if defined(CONFIG_X86)
volatile size_t size = MMU_PAGE_SIZE;
#elif defined(CONFIG_ARM) && defined(CONFIG_PRIVILEGED_STACK_SIZE)
volatile size_t size = CONFIG_PRIVILEGED_STACK_SIZE;
#else
volatile size_t size = 512;
#endif

static void read_kernel_stack(void)
{
	/* Try to read from kernel stack. */
	int s[1];

	s[0] = 0;
	ptr = &s[0];
	ptr = (int *)((unsigned char *)ptr - size);
	printk("%d\n", *ptr);
	zassert_unreachable("Read from kernel stack did not fault\n");
}

static void write_kernel_stack(void)
{
	/* Try to write to kernel stack. */
	int s[1];

	s[0] = 0;
	ptr = &s[0];
	ptr = (int *)((unsigned char *)ptr - size);
	*ptr = 42;
	zassert_unreachable("Write to kernel stack did not fault\n");
}


static struct k_sem sem;

static void pass_user_object(void)
{
	/* Try to pass a user object to a system call. */
	k_sem_init(&sem, 0, 1);
	zassert_unreachable("Pass a user object to a syscall did not fault\n");
}

__kernel static struct k_sem ksem;

static void pass_noperms_object(void)
{
	/* Try to pass a object to a system call w/o permissions. */
	k_sem_init(&ksem, 0, 1);
	zassert_unreachable("Pass an unauthorized object to a "
			    "syscall did not fault\n");
}

__kernel struct k_thread kthread_thread;

#define STACKSIZE 512
K_THREAD_STACK_DEFINE(kthread_stack, STACKSIZE);

void thread_body(void)
{
}

static void start_kernel_thread(void)
{
	/* Try to start a kernel thread from a usermode thread */
	k_thread_create(&kthread_thread, kthread_stack, STACKSIZE,
			(k_thread_entry_t)thread_body, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_INHERIT_PERMS,
			K_NO_WAIT);
	zassert_unreachable("Create a kernel thread did not fault\n");
}

void test_main(void)
{
	k_thread_access_grant(k_current_get(),
			      &kthread_thread, &kthread_stack,
			      NULL);
	ztest_test_suite(test_userspace,
			 ztest_user_unit_test(is_usermode),
			 ztest_user_unit_test(priv_insn),
			 ztest_user_unit_test(write_control),
			 ztest_user_unit_test(disable_mmu_mpu),
			 ztest_user_unit_test(read_kernram),
			 ztest_user_unit_test(write_kernram),
			 ztest_user_unit_test(write_kernro),
			 ztest_user_unit_test(write_kerntext),
			 ztest_user_unit_test(read_kernel_data),
			 ztest_user_unit_test(write_kernel_data),
			 ztest_user_unit_test(read_kernel_stack),
			 ztest_user_unit_test(write_kernel_stack),
			 ztest_user_unit_test(pass_user_object),
			 ztest_user_unit_test(pass_noperms_object),
			 ztest_user_unit_test(start_kernel_thread)
		);
	ztest_run_test_suite(test_userspace);
}

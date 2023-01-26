/*
 * Parts derived from tests/kernel/fatal/src/main.c, which has the
 * following copyright and license:
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/util.h>
#include <zephyr/debug/stack.h>
#include <zephyr/syscall_handler.h>
#include "test_syscall.h"
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */

#if defined(CONFIG_ARC)
#include <zephyr/arch/arc/v2/mpu/arc_core_mpu.h>
#endif

#if defined(CONFIG_ARM)
extern void arm_core_mpu_disable(void);
#endif

#define INFO(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define PIPE_LEN 1
#define BYTES_TO_READ_WRITE 1
#define STACKSIZE (256 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_SEM_DEFINE(test_revoke_sem, 0, 1);

/* Used for tests that switch between domains, we will switch between the
 * default domain and this one.
 */
struct k_mem_domain alternate_domain;

ZTEST_BMEM static volatile bool expect_fault;
ZTEST_BMEM static volatile unsigned int expected_reason;

/* Partition unique to default domain */
K_APPMEM_PARTITION_DEFINE(default_part);
K_APP_BMEM(default_part) volatile bool default_bool;
/* Partition unique to alternate domain */
K_APPMEM_PARTITION_DEFINE(alt_part);
K_APP_BMEM(alt_part) volatile bool alt_bool;

static struct k_thread test_thread;
static K_THREAD_STACK_DEFINE(test_stack, STACKSIZE);

static void clear_fault(void)
{
	expect_fault = false;
	compiler_barrier();
}

static void set_fault(unsigned int reason)
{
	expect_fault = true;
	expected_reason = reason;
	compiler_barrier();
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	INFO("Caught system error -- reason %d\n", reason);

	if (expect_fault) {
		if (expected_reason == reason) {
			printk("System error was expected\n");
			clear_fault();
		} else {
			printk("Wrong fault reason, expecting %d\n",
			       expected_reason);
			k_fatal_halt(reason);
		}
	} else {
		printk("Unexpected fault during test\n");
		k_fatal_halt(reason);
	}
}

/**
 * @brief Test to check if the thread is in user mode
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_is_usermode)
{
	/* Confirm that we are in fact running in user mode. */
	clear_fault();

	zassert_true(k_is_user_context(), "thread left in kernel mode");
}

/**
 * @brief Test to write to a control register
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_control)
{
	/* Try to write to a control register. */
#if defined(CONFIG_X86)
	set_fault(K_ERR_CPU_EXCEPTION);

#ifdef CONFIG_X86_64
	__asm__ volatile (
		"movq $0xFFFFFFFF, %rax;\n\t"
		"movq %rax, %cr0;\n\t"
		);
#else
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0xfffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
#endif
	zassert_unreachable("Write to control register did not fault");

#elif defined(CONFIG_ARM64)
	uint64_t val = SPSR_MODE_EL1T;

	set_fault(K_ERR_CPU_EXCEPTION);

	__asm__ volatile("msr spsr_el1, %0"
			:
			: "r" (val)
			: "memory", "cc");

	zassert_unreachable("Write to control register did not fault");

#elif defined(CONFIG_ARM)
#if defined(CONFIG_CPU_CORTEX_M)
	unsigned int msr_value;

	clear_fault();

	msr_value = __get_CONTROL();
	msr_value &= ~(CONTROL_nPRIV_Msk);
	__set_CONTROL(msr_value);
	__DSB();
	__ISB();
	msr_value = __get_CONTROL();
	zassert_true((msr_value & (CONTROL_nPRIV_Msk)),
		     "Write to control register was successful");
#else
	uint32_t val;

	set_fault(K_ERR_CPU_EXCEPTION);

	val = __get_SCTLR();
	val |= SCTLR_DZ_Msk;
	__set_SCTLR(val);

	zassert_unreachable("Write to control register did not fault");
#endif
#elif defined(CONFIG_ARC)
	unsigned int er_status;

	set_fault(K_ERR_CPU_EXCEPTION);

	/* _ARC_V2_ERSTATUS is privilege aux reg */
	__asm__ volatile (
		"lr %0, [0x402]\n"
		: "=r" (er_status)::
	);
#elif defined(CONFIG_RISCV)
	unsigned int status;

	set_fault(K_ERR_CPU_EXCEPTION);

	__asm__ volatile("csrr %0, mstatus" : "=r" (status));
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
ZTEST_USER(userspace, test_disable_mmu_mpu)
{
	/* Try to disable memory protections. */
#if defined(CONFIG_X86)
	set_fault(K_ERR_CPU_EXCEPTION);

#ifdef CONFIG_X86_64
	__asm__ volatile (
		"movq %cr0, %rax;\n\t"
		"andq $0x7ffeffff, %rax;\n\t"
		"movq %rax, %cr0;\n\t"
		);
#else
	__asm__ volatile (
		"mov %cr0, %eax;\n\t"
		"and $0x7ffeffff, %eax;\n\t"
		"mov %eax, %cr0;\n\t"
		);
#endif
#elif defined(CONFIG_ARM64)
	uint64_t val;

	set_fault(K_ERR_CPU_EXCEPTION);

	__asm__ volatile("mrs %0, sctlr_el1" : "=r" (val));
	__asm__ volatile("msr sctlr_el1, %0"
			:
			: "r" (val & ~(SCTLR_M_BIT | SCTLR_C_BIT))
			: "memory", "cc");

#elif defined(CONFIG_ARM)
#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
	set_fault(K_ERR_CPU_EXCEPTION);

	arm_core_mpu_disable();
#else
	/* Disabling MPU from unprivileged code
	 * generates BusFault which is not banked
	 * between Security states. Do not execute
	 * this scenario for Non-Secure Cortex-M.
	 */
	return;
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */
#elif defined(CONFIG_ARC)
	set_fault(K_ERR_CPU_EXCEPTION);

	arc_core_mpu_disable();
#elif defined(CONFIG_RISCV)
	set_fault(K_ERR_CPU_EXCEPTION);

	/*
	 * Try to make everything accessible through PMP slot 3
	 * which should not be locked.
	 */
	csr_write(pmpaddr3, LLONG_MAX);
	csr_write(pmpcfg0, (PMP_R|PMP_W|PMP_X|PMP_NAPOT) << 24);
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
ZTEST_USER(userspace, test_read_kernram)
{
	/* Try to read from kernel RAM. */
	void *p;

	set_fault(K_ERR_CPU_EXCEPTION);

	p = _current->init_data;
	printk("%p\n", p);
	zassert_unreachable("Read from kernel RAM did not fault");
}

/**
 * @brief Test to write to kernel RAM
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_kernram)
{
	/* Try to write to kernel RAM. */
	set_fault(K_ERR_CPU_EXCEPTION);

	_current->init_data = NULL;
	zassert_unreachable("Write to kernel RAM did not fault");
}

extern int _k_neg_eagain;

#include <zephyr/linker/linker-defs.h>

/**
 * @brief Test to write kernel RO
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_kernro)
{
	bool in_rodata;

	/* Try to write to kernel RO. */
	const char *const ptr = (const char *const)&_k_neg_eagain;

	in_rodata = ptr < __rodata_region_end &&
		    ptr >= __rodata_region_start;

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
	if (!in_rodata) {
		in_rodata = ptr < lnkr_pinned_rodata_end &&
			    ptr >= lnkr_pinned_rodata_start;
	}
#endif

	zassert_true(in_rodata,
		     "_k_neg_eagain is not in rodata");

	set_fault(K_ERR_CPU_EXCEPTION);

	_k_neg_eagain = -EINVAL;
	zassert_unreachable("Write to kernel RO did not fault");
}

/**
 * @brief Test to write to kernel text section
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_kerntext)
{
	/* Try to write to kernel text. */
	set_fault(K_ERR_CPU_EXCEPTION);

	memset(&z_is_thread_essential, 0, 4);
	zassert_unreachable("Write to kernel text did not fault");
}

static int kernel_data;

/**
 * @brief Test to read from kernel data section
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_read_kernel_data)
{
	set_fault(K_ERR_CPU_EXCEPTION);

	printk("%d\n", kernel_data);
	zassert_unreachable("Read from data did not fault");
}

/**
 * @brief Test to write to kernel data section
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_kernel_data)
{
	set_fault(K_ERR_CPU_EXCEPTION);

	kernel_data = 1;
	zassert_unreachable("Write to  data did not fault");
}

/*
 * volatile to avoid compiler mischief.
 */
K_APP_DMEM(default_part) volatile char *priv_stack_ptr;
#if defined(CONFIG_ARC)
K_APP_DMEM(default_part) int32_t size = (0 - CONFIG_PRIVILEGED_STACK_SIZE -
				 Z_ARC_STACK_GUARD_SIZE);
#endif

/**
 * @brief Test to read privileged stack
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_read_priv_stack)
{
	/* Try to read from privileged stack. */
#if defined(CONFIG_ARC)
	int s[1];

	s[0] = 0;
	priv_stack_ptr = (char *)&s[0] - size;
#elif defined(CONFIG_ARM) || defined(CONFIG_X86) || defined(CONFIG_RISCV) || defined(CONFIG_ARM64)
	/* priv_stack_ptr set by test_main() */
#else
#error "Not implemented for this architecture"
#endif
	set_fault(K_ERR_CPU_EXCEPTION);

	printk("%c\n", *priv_stack_ptr);
	zassert_unreachable("Read from privileged stack did not fault");
}

/**
 * @brief Test to write to privilege stack
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_priv_stack)
{
	/* Try to write to privileged stack. */
#if defined(CONFIG_ARC)
	int s[1];

	s[0] = 0;
	priv_stack_ptr = (char *)&s[0] - size;
#elif defined(CONFIG_ARM) || defined(CONFIG_X86) || defined(CONFIG_RISCV) || defined(CONFIG_ARM64)
	/* priv_stack_ptr set by test_main() */
#else
#error "Not implemented for this architecture"
#endif
	set_fault(K_ERR_CPU_EXCEPTION);

	*priv_stack_ptr = 42;
	zassert_unreachable("Write to privileged stack did not fault");
}


K_APP_BMEM(default_part) static struct k_sem sem;

/**
 * @brief Test to pass a user object to system call
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_pass_user_object)
{
	/* Try to pass a user object to a system call. */
	set_fault(K_ERR_KERNEL_OOPS);

	k_sem_init(&sem, 0, 1);
	zassert_unreachable("Pass a user object to a syscall did not fault");
}

static struct k_sem ksem;

/**
 * @brief Test to pass object to a system call without permissions
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_pass_noperms_object)
{
	/* Try to pass a object to a system call w/o permissions. */
	set_fault(K_ERR_KERNEL_OOPS);

	k_sem_init(&ksem, 0, 1);
	zassert_unreachable("Pass an unauthorized object to a "
			    "syscall did not fault");
}


void thread_body(void)
{
}

/**
 * @brief Test to start kernel thread from usermode
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_start_kernel_thread)
{
	/* Try to start a kernel thread from a usermode thread */
	set_fault(K_ERR_KERNEL_OOPS);
	k_thread_create(&test_thread, test_stack, STACKSIZE,
			(k_thread_entry_t)thread_body, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_INHERIT_PERMS,
			K_NO_WAIT);
	zassert_unreachable("Create a kernel thread did not fault");
}

static void uthread_read_body(void *p1, void *p2, void *p3)
{
	unsigned int *vptr = p1;

	set_fault(K_ERR_CPU_EXCEPTION);
	printk("%u\n", *vptr);
	zassert_unreachable("Read from other thread stack did not fault");
}

static void uthread_write_body(void *p1, void *p2, void *p3)
{
	unsigned int *vptr = p1;

	set_fault(K_ERR_CPU_EXCEPTION);
	*vptr = 2U;
	zassert_unreachable("Write to other thread stack did not fault");
}

/**
 * @brief Test to read from another thread's stack
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_read_other_stack)
{
	/* Try to read from another thread's stack. */
	unsigned int val;

#ifdef CONFIG_MMU
	ztest_test_skip();
#endif
	k_thread_create(&test_thread, test_stack, STACKSIZE,
			uthread_read_body, &val, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_join(&test_thread, K_FOREVER);
}


/**
 * @brief Test to write to other thread's stack
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_other_stack)
{
	/* Try to write to another thread's stack. */
	unsigned int val;

#ifdef CONFIG_MMU
	ztest_test_skip();
#endif
	k_thread_create(&test_thread, test_stack, STACKSIZE,
			uthread_write_body, &val, NULL, NULL,
			-1, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);
	k_thread_join(&test_thread, K_FOREVER);
}

/**
 * @brief Test to revoke access to kobject without permission
 *
 * @details User thread can only revoke their own access to an object.
 * In that test user thread to revokes access to unathorized object, as a result
 * the system will assert.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_revoke_noperms_object)
{
	/* Attempt to revoke access to kobject w/o permissions*/
	set_fault(K_ERR_KERNEL_OOPS);

	k_object_release(&ksem);

	zassert_unreachable("Revoke access to unauthorized object "
			    "did not fault");
}

/**
 * @brief Test to access object after revoking access
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_access_after_revoke)
{
	k_object_release(&test_revoke_sem);

	/* Try to access an object after revoking access to it */
	set_fault(K_ERR_KERNEL_OOPS);

	k_sem_take(&test_revoke_sem, K_NO_WAIT);

	zassert_unreachable("Using revoked object did not fault");
}

static void umode_enter_func(void)
{
	zassert_true(k_is_user_context(),
		     "Thread did not enter user mode");
}

/**
* @brief Test to check supervisor thread enter one-way to usermode
*
* @details A thread running in supervisor mode must have one-way operation
* ability to drop privileges to user mode.
*
* @ingroup kernel_memprotect_tests
*/
ZTEST(userspace, test_user_mode_enter)
{
	clear_fault();

	k_thread_user_mode_enter((k_thread_entry_t)umode_enter_func,
				 NULL, NULL, NULL);
}

/* Define and initialize pipe. */
K_PIPE_DEFINE(kpipe, PIPE_LEN, BYTES_TO_READ_WRITE);
K_APP_BMEM(default_part) static size_t bytes_written_read;

/**
 * @brief Test to write to kobject using pipe
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_write_kobject_user_pipe)
{
	/*
	 * Attempt to use system call from k_pipe_get to write over
	 * a kernel object.
	 */
	set_fault(K_ERR_KERNEL_OOPS);

	k_pipe_get(&kpipe, &test_revoke_sem, BYTES_TO_READ_WRITE,
		   &bytes_written_read, 1, K_NO_WAIT);

	zassert_unreachable("System call memory write validation "
			    "did not fault");
}

/**
 * @brief Test to read from kobject using pipe
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_read_kobject_user_pipe)
{
	/*
	 * Attempt to use system call from k_pipe_put to read a
	 * kernel object.
	 */
	set_fault(K_ERR_KERNEL_OOPS);

	k_pipe_put(&kpipe, &test_revoke_sem, BYTES_TO_READ_WRITE,
		   &bytes_written_read, 1, K_NO_WAIT);

	zassert_unreachable("System call memory read validation "
			    "did not fault");
}

static void user_half(void *arg1, void *arg2, void *arg3)
{
	volatile bool *bool_ptr = arg1;

	*bool_ptr = true;
	compiler_barrier();
	if (expect_fault) {
		printk("Expecting a fatal error %d but succeeded instead\n",
		       expected_reason);
		ztest_test_fail();
	}
}


static void spawn_user(volatile bool *to_modify)
{
	k_thread_create(&test_thread, test_stack, STACKSIZE, user_half,
			(void *)to_modify, NULL, NULL,
			-1, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&test_thread, K_FOREVER);
}

static void drop_user(volatile bool *to_modify)
{
	k_sleep(K_MSEC(1)); /* Force a context switch */
	k_thread_user_mode_enter(user_half, (void *)to_modify, NULL, NULL);
}

/**
 * @brief Test creation of new memory domains
 *
 * We initialize a new memory domain and show that its partition configuration
 * is correct. This new domain has "alt_part" in it, but not "default_part".
 * We then try to modify data in "default_part" and show it produces an
 * exception since that partition is not in the new domain.
 *
 * This caught a bug once where an MMU system copied page tables for the new
 * domain and accidentally copied memory partition permissions from the source
 * page tables, allowing the write to "default_part" to work.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain, test_1st_init_and_access_other_memdomain)
{
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&ztest_mem_partition, &alt_part
	};

	zassert_equal(
		k_mem_domain_init(&alternate_domain, ARRAY_SIZE(parts), parts),
		0, "failed to initialize memory domain");

	/* Switch to alternate_domain which does not have default_part that
	 * contains default_bool. This should fault when we try to write it.
	 */
	k_mem_domain_add_thread(&alternate_domain, k_current_get());
	set_fault(K_ERR_CPU_EXCEPTION);
	spawn_user(&default_bool);
}

#if (defined(CONFIG_ARM) || (defined(CONFIG_GEN_PRIV_STACKS) && defined(CONFIG_RISCV)))
extern uint8_t *z_priv_stack_find(void *obj);
#endif
extern k_thread_stack_t ztest_thread_stack[];

/**
 * Show that changing between memory domains and dropping to user mode works
 * as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain, test_domain_add_thread_drop_to_user)
{
	clear_fault();
	k_mem_domain_add_thread(&alternate_domain, k_current_get());
	drop_user(&alt_bool);
}

/* @brief Test adding application memory partition to memory domain
 *
 * @details Show that adding a partition to a domain and then dropping to user
 * mode works as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain, test_domain_add_part_drop_to_user)
{
	clear_fault();

	zassert_equal(
		k_mem_domain_add_partition(&k_mem_domain_default, &alt_part),
		0, "failed to add memory partition");

	drop_user(&alt_bool);
}

/**
 * Show that self-removing a partition from a domain we are a member of,
 * and then dropping to user mode faults as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain, test_domain_remove_part_drop_to_user)
{
	/* We added alt_part to the default domain in the previous test,
	 * remove it, and then try to access again.
	 */
	set_fault(K_ERR_CPU_EXCEPTION);

	zassert_equal(
		k_mem_domain_remove_partition(&k_mem_domain_default, &alt_part),
		0, "failed to remove partition");

	drop_user(&alt_bool);
}

/**
 * Show that changing between memory domains and then switching to another
 * thread in the same domain works as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain_ctx, test_domain_add_thread_context_switch)
{
	clear_fault();
	k_mem_domain_add_thread(&alternate_domain, k_current_get());
	spawn_user(&alt_bool);
}

/* Show that adding a partition to a domain and then switching to another
 * user thread in the same domain works as expected.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain_ctx, test_domain_add_part_context_switch)
{
	clear_fault();

	zassert_equal(
		k_mem_domain_add_partition(&k_mem_domain_default, &alt_part),
		0, "failed to add memory partition");

	spawn_user(&alt_bool);
}

/**
 * Show that self-removing a partition from a domain we are a member of,
 * and then switching to another user thread in the same domain faults as
 * expected.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace_domain_ctx, test_domain_remove_part_context_switch)
{
	/* We added alt_part to the default domain in the previous test,
	 * remove it, and then try to access again.
	 */
	set_fault(K_ERR_CPU_EXCEPTION);

	zassert_equal(
		k_mem_domain_remove_partition(&k_mem_domain_default, &alt_part),
		0, "failed to remove memory partition");

	spawn_user(&alt_bool);
}

void z_impl_missing_syscall(void)
{
	/* Shouldn't ever get here; no handler function compiled */
	k_panic();
}

/**
 * @brief Test unimplemented system call
 *
 * @details Created a syscall with name missing_syscall() without a verification
 * function. The kernel shall safety handle invocations of unimplemented system
 * calls.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_unimplemented_syscall)
{
	set_fault(K_ERR_KERNEL_OOPS);

	missing_syscall();
}

/**
 * @brief Test bad syscall handler
 *
 * @details When a system call handler decides to terminate the calling thread,
 * the kernel will produce error which indicates the context, where the faulting
 * system call was made from user code.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(userspace, test_bad_syscall)
{
	set_fault(K_ERR_KERNEL_OOPS);

	arch_syscall_invoke0(INT_MAX);

	set_fault(K_ERR_KERNEL_OOPS);

	arch_syscall_invoke0(UINT_MAX);
}

static struct k_sem recycle_sem;

/**
 * @brief Test recycle object
 *
 * @details Test recycle valid/invalid kernel object, see if
 * perms_count changes as expected.
 *
 * @see z_object_recycle(), z_object_find()
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST(userspace, test_object_recycle)
{
	struct z_object *ko;
	int perms_count = 0;
	int dummy = 0;

	/* Validate recycle invalid objects, after recycling this invalid
	 * object, perms_count should finally still be 1.
	 */
	ko = z_object_find(&dummy);
	zassert_true(ko == NULL, "not an invalid object");

	z_object_recycle(&dummy);

	ko = z_object_find(&recycle_sem);
	(void)memset(ko->perms, 0xFF, sizeof(ko->perms));

	z_object_recycle(&recycle_sem);
	zassert_true(ko != NULL, "kernel object not found");
	zassert_true(ko->flags & K_OBJ_FLAG_INITIALIZED,
		     "object wasn't marked as initialized");

	for (int i = 0; i < CONFIG_MAX_THREAD_BYTES; i++) {
		perms_count += POPCOUNT(ko->perms[i]);
	}

	zassert_true(perms_count == 1, "invalid number of thread permissions");
}

#define test_oops(provided, expected) do { \
	expect_fault = true; \
	expected_reason = expected; \
	z_except_reason(provided); \
} while (false)

ZTEST_USER(userspace, test_oops_panic)
{
	test_oops(K_ERR_KERNEL_PANIC, K_ERR_KERNEL_OOPS);
}

ZTEST_USER(userspace, test_oops_oops)
{
	test_oops(K_ERR_KERNEL_OOPS, K_ERR_KERNEL_OOPS);
}

ZTEST_USER(userspace, test_oops_exception)
{
	test_oops(K_ERR_CPU_EXCEPTION, K_ERR_KERNEL_OOPS);
}

ZTEST_USER(userspace, test_oops_maxint)
{
	test_oops(INT_MAX, K_ERR_KERNEL_OOPS);
}

ZTEST_USER(userspace, test_oops_stackcheck)
{
	test_oops(K_ERR_STACK_CHK_FAIL, K_ERR_STACK_CHK_FAIL);
}

void z_impl_check_syscall_context(void)
{
	unsigned int key = irq_lock();

	irq_unlock(key);

	/* Make sure that interrupts aren't locked when handling system calls;
	 * key has the previous locking state before the above irq_lock() call.
	 */
	zassert_true(arch_irq_unlocked(key), "irqs locked during syscall");

	/* The kernel should not think we are in ISR context either */
	zassert_false(k_is_in_isr(), "kernel reports irq context");
}

static inline void z_vrfy_check_syscall_context(void)
{
	return z_impl_check_syscall_context();
}
#include <syscalls/check_syscall_context_mrsh.c>

ZTEST_USER(userspace, test_syscall_context)
{
	check_syscall_context();
}

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
static void tls_leakage_user_part(void *p1, void *p2, void *p3)
{
	char *tls_area = p1;

	for (int i = 0; i < sizeof(struct _thread_userspace_local_data); i++) {
		zassert_false(tls_area[i] == 0xff,
			      "TLS data leakage to user mode");
	}
}
#endif

ZTEST(userspace, test_tls_leakage)
{
#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
	/* Tests two assertions:
	 *
	 * - That a user thread has full access to its TLS area
	 * - That dropping to user mode doesn't allow any TLS data set in
	 * supervisor mode to be leaked
	 */

	memset(_current->userspace_local_data, 0xff,
	       sizeof(struct _thread_userspace_local_data));

	k_thread_user_mode_enter(tls_leakage_user_part,
				 _current->userspace_local_data, NULL, NULL);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
void tls_entry(void *p1, void *p2, void *p3)
{
	printk("tls_entry\n");
}
#endif

ZTEST(userspace, test_tls_pointer)
{
#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
	k_thread_create(&test_thread, test_stack, STACKSIZE, tls_entry,
			NULL, NULL, NULL, 1, K_USER, K_FOREVER);

	printk("tls pointer for thread %p: %p\n",
	       &test_thread, (void *)test_thread.userspace_local_data);

	printk("stack buffer reported bounds: [%p, %p)\n",
	       (void *)test_thread.stack_info.start,
	       (void *)(test_thread.stack_info.start +
			test_thread.stack_info.size));

	printk("stack object bounds: [%p, %p)\n",
	       test_stack, test_stack + sizeof(test_stack));

	uintptr_t tls_start = (uintptr_t)test_thread.userspace_local_data;
	uintptr_t tls_end = tls_start +
		sizeof(struct _thread_userspace_local_data);

	if ((tls_start < (uintptr_t)test_stack) ||
	    (tls_end > (uintptr_t)test_stack + sizeof(test_stack))) {
		printk("tls area out of bounds\n");
		ztest_test_fail();
	}

	k_thread_abort(&test_thread);
#else
	ztest_test_skip();
#endif
}


void *userspace_setup(void)
{
	int ret;

	/* Most of these scenarios use the default domain */
	ret = k_mem_domain_add_partition(&k_mem_domain_default, &default_part);
	if (ret != 0) {
		printk("Failed to add default memory partition (%d)\n", ret);
		k_oops();
	}

#if defined(CONFIG_ARM64)
	struct z_arm64_thread_stack_header *hdr;
	void *vhdr = ((struct z_arm64_thread_stack_header *)ztest_thread_stack);

	hdr = vhdr;
	priv_stack_ptr = (((char *)&hdr->privilege_stack) +
			  (sizeof(hdr->privilege_stack) - 1));
#elif defined(CONFIG_ARM)
	priv_stack_ptr = (char *)z_priv_stack_find(ztest_thread_stack);
#elif defined(CONFIG_X86)
	struct z_x86_thread_stack_header *hdr;
	void *vhdr = ((struct z_x86_thread_stack_header *)ztest_thread_stack);

	hdr = vhdr;
	priv_stack_ptr = (((char *)&hdr->privilege_stack) +
			  (sizeof(hdr->privilege_stack) - 1));
#elif defined(CONFIG_RISCV)
#if defined(CONFIG_GEN_PRIV_STACKS)
	priv_stack_ptr = (char *)z_priv_stack_find(ztest_thread_stack);
#else
	priv_stack_ptr = (char *)((uintptr_t)ztest_thread_stack +
				  Z_RISCV_STACK_GUARD_SIZE);
#endif
#endif
	k_thread_access_grant(k_current_get(),
			      &test_thread, &test_stack,
			      &test_revoke_sem, &kpipe);
	return NULL;
}

ZTEST_SUITE(userspace, NULL, userspace_setup, NULL, NULL, NULL);

ZTEST_SUITE(userspace_domain, NULL, NULL, NULL, NULL, NULL);

ZTEST_SUITE(userspace_domain_ctx, NULL, NULL, NULL, NULL, NULL);

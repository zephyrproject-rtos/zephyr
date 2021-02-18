/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <syscall_handler.h>
#include <ztest.h>
#include <linker/linker-defs.h>
#include "test_syscalls.h"
#include <mmu.h>

#define BUF_SIZE	32
#define SLEEP_MS_LONG	15000

#if defined(CONFIG_BOARD_NUCLEO_F429ZI) || defined(CONFIG_BOARD_NUCLEO_F207ZG) \
	|| defined(CONFIG_BOARD_NUCLEO_L073RZ) \
	|| defined(CONFIG_BOARD_RONOTH_LODEV)
#define FAULTY_ADDRESS 0x0FFFFFFF
#elif CONFIG_MMU
/* Just past the zephyr image mapping should be a non-present page */
#define FAULTY_ADDRESS Z_FREE_VM_START
#else
#define FAULTY_ADDRESS 0xFFFFFFF0
#endif

char kernel_string[BUF_SIZE];
char kernel_buf[BUF_SIZE];
ZTEST_BMEM char user_string[BUF_SIZE];

size_t z_impl_string_nlen(char *src, size_t maxlen, int *err)
{
	return z_user_string_nlen(src, maxlen, err);
}

static inline size_t z_vrfy_string_nlen(char *src, size_t maxlen, int *err)
{
	int err_copy;
	size_t ret;

	ret = z_impl_string_nlen((char *)src, maxlen, &err_copy);
	if (!err_copy && Z_SYSCALL_MEMORY_READ(src, ret + 1)) {
		err_copy = -1;
	}

	Z_OOPS(z_user_to_copy((int *)err, &err_copy, sizeof(err_copy)));

	return ret;
}
#include <syscalls/string_nlen_mrsh.c>

int z_impl_string_alloc_copy(char *src)
{
	if (!strcmp(src, kernel_string)) {
		return 0;
	} else {
		return -2;
	}
}

static inline int z_vrfy_string_alloc_copy(char *src)
{
	char *src_copy;
	int ret;

	src_copy = z_user_string_alloc_copy((char *)src, BUF_SIZE);
	if (!src_copy) {
		return -1;
	}

	ret = z_impl_string_alloc_copy(src_copy);
	k_free(src_copy);

	return ret;
}
#include <syscalls/string_alloc_copy_mrsh.c>

int z_impl_string_copy(char *src)
{
	if (!strcmp(src, kernel_string)) {
		return 0;
	} else {
		return ESRCH;
	}
}

static inline int z_vrfy_string_copy(char *src)
{
	int ret = z_user_string_copy(kernel_buf, (char *)src, BUF_SIZE);

	if (ret) {
		return ret;
	}

	return z_impl_string_copy(kernel_buf);
}
#include <syscalls/string_copy_mrsh.c>

/* Not actually used, but will copy wrong string if called by mistake instead
 * of the handler
 */
int z_impl_to_copy(char *dest)
{
	memcpy(dest, kernel_string, BUF_SIZE);
	return 0;
}

static inline int z_vrfy_to_copy(char *dest)
{
	return z_user_to_copy((char *)dest, user_string, BUF_SIZE);
}
#include <syscalls/to_copy_mrsh.c>

int z_impl_syscall_arg64(uint64_t arg)
{
	/* "Hash" (heh) the return to avoid accidental false positives
	 * due to using common/predictable values.
	 */
	return (int)(arg + 0x8c32a9eda4ca2621ULL + (size_t)&kernel_string);
}

static inline int z_vrfy_syscall_arg64(uint64_t arg)
{
	return z_impl_syscall_arg64(arg);
}
#include <syscalls/syscall_arg64_mrsh.c>

/* Bigger 64 bit arg syscall to exercise marshalling 7+ words of
 * arguments (this one happens to need 9), and to test generation of
 * 64 bit return values.
 */
uint64_t z_impl_syscall_arg64_big(uint32_t arg1, uint32_t arg2,
			       uint64_t arg3, uint32_t arg4,
			       uint32_t arg5, uint64_t arg6)
{
	uint64_t args[] = { arg1, arg2, arg3, arg4, arg5, arg6 };
	uint64_t ret = 0xae751a24ef464cc0ULL;

	for (int i = 0; i < ARRAY_SIZE(args); i++) {
		ret += args[i];
		ret = (ret << 11) | (ret >> 53);
	}

	return ret;
}

static inline uint64_t z_vrfy_syscall_arg64_big(uint32_t arg1, uint32_t arg2,
					     uint64_t arg3, uint32_t arg4,
					     uint32_t arg5, uint64_t arg6)
{
	return z_impl_syscall_arg64_big(arg1, arg2, arg3, arg4, arg5, arg6);
}
#include <syscalls/syscall_arg64_big_mrsh.c>

uint32_t z_impl_more_args(uint32_t arg1, uint32_t arg2, uint32_t arg3,
			  uint32_t arg4, uint32_t arg5, uint32_t arg6,
			  uint32_t arg7)
{
	uint32_t ret = 0x4ef464cc;
	uint32_t args[] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };

	for (int i = 0; i < ARRAY_SIZE(args); i++) {
		ret += args[i];
		ret = (ret << 11) | (ret >> 5);
	}

	return ret;
}

static inline uint32_t z_vrfy_more_args(uint32_t arg1, uint32_t arg2,
					uint32_t arg3, uint32_t arg4,
					uint32_t arg5, uint32_t arg6,
					uint32_t arg7)
{
	return z_impl_more_args(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}
#include <syscalls/more_args_mrsh.c>

/**
 * @brief Test to demonstrate usage of z_user_string_nlen()
 *
 * @details The test will be called from user mode and kernel
 * mode to check the behavior of z_user_string_nlen()
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_user_string_nlen()
 */
void test_string_nlen(void)
{
	int err;
	size_t ret;

	ret = string_nlen(kernel_string, BUF_SIZE, &err);
	if (arch_is_user_context()) {
		zassert_equal(err, -1,
			      "kernel string did not fault on user access");
	} else {
		zassert_equal(err, 0, "kernel string faulted in kernel mode");
		zassert_equal(ret, strlen(kernel_string),
			      "incorrect length returned");
	}

	/* Valid usage */
	ret = string_nlen(user_string, BUF_SIZE, &err);
	zassert_equal(err, 0, "user string faulted");
	zassert_equal(ret, strlen(user_string), "incorrect length returned");

	/* Skip this scenario for nsim_sem emulated board, unfortunately
	 * the emulator doesn't set up memory as specified in DTS and poking
	 * this address doesn't fault
	 * Also skip this scenario for em_starterkit_7d, which won't generate
	 * exceptions when unmapped address is accessed.
	 *
	 * In addition to the above, skip the scenario for Non-Secure Cortex-M
	 * builds; Zephyr running in Non-Secure mode will generate SecureFault
	 * if it attempts to access any address outside the image Flash or RAM
	 * boundaries, and the program will hang.
	 */
#if !((defined(CONFIG_BOARD_NSIM) && defined(CONFIG_SOC_NSIM_SEM)) || \
	defined(CONFIG_SOC_EMSK_EM7D) || \
	(defined(CONFIG_CPU_CORTEX_M) && \
		defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)))
	/* Try to blow up the kernel */
	ret = string_nlen((char *)FAULTY_ADDRESS, BUF_SIZE, &err);
	zassert_equal(err, -1, "nonsense string address did not fault");
#endif
}

/**
 * @brief Test to verify syscall for string alloc copy
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_user_string_alloc_copy(), strcmp()
 */
void test_user_string_alloc_copy(void)
{
	int ret;

	ret = string_alloc_copy("asdkajshdazskjdh");
	zassert_equal(ret, -2, "got %d", ret);

	ret = string_alloc_copy(
	    "asdkajshdazskjdhikfsdjhfskdjfhsdkfjhskdfjhdskfjhs");
	zassert_equal(ret, -1, "got %d", ret);

	ret = string_alloc_copy(kernel_string);
	zassert_equal(ret, -1, "got %d", ret);

	ret = string_alloc_copy("this is a kernel string");
	zassert_equal(ret, 0, "string should have matched");
}

/**
 * @brief Test sys_call for string copy
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_user_string_copy(), strcmp()
 */
void test_user_string_copy(void)
{
	int ret;

	ret = string_copy("asdkajshdazskjdh");
	zassert_equal(ret, ESRCH, "got %d", ret);

	ret = string_copy("asdkajshdazskjdhikfsdjhfskdjfhsdkfjhskdfjhdskfjhs");
	zassert_equal(ret, EINVAL, "got %d", ret);

	ret = string_copy(kernel_string);
	zassert_equal(ret, EFAULT, "got %d", ret);

	ret = string_copy("this is a kernel string");
	zassert_equal(ret, 0, "string should have matched");
}

/**
 * @brief Test to demonstrate system call for copy
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see memcpy(), z_user_to_copy()
 */
void test_to_copy(void)
{
	char buf[BUF_SIZE];
	int ret;

	ret = to_copy(kernel_buf);
	zassert_equal(ret, EFAULT, "should have faulted");

	ret = to_copy(buf);
	zassert_equal(ret, 0, "copy should have been a success");
	ret = strcmp(buf, user_string);
	zassert_equal(ret, 0, "string should have matched");
}

void test_arg64(void)
{
	zassert_equal(syscall_arg64(54321),
		      z_impl_syscall_arg64(54321),
		      "syscall didn't match impl");

	zassert_equal(syscall_arg64_big(1, 2, 3, 4, 5, 6),
		      z_impl_syscall_arg64_big(1, 2, 3, 4, 5, 6),
		      "syscall didn't match impl");
}

void test_more_args(void)
{
	zassert_equal(more_args(1, 2, 3, 4, 5, 6, 7),
		      z_impl_more_args(1, 2, 3, 4, 5, 6, 7),
		      "syscall didn't match impl");
}

#define NR_THREADS	(CONFIG_MP_NUM_CPUS * 4)
#define STACK_SZ	(1024 + CONFIG_TEST_EXTRA_STACKSIZE)

struct k_thread torture_threads[NR_THREADS];
K_THREAD_STACK_ARRAY_DEFINE(torture_stacks, NR_THREADS, STACK_SZ);

void syscall_torture(void *arg1, void *arg2, void *arg3)
{
	int count = 0;
	uintptr_t id = (uintptr_t)arg1;
	int ret, err;
	char buf[BUF_SIZE];

	for (;; ) {
		/* Run a bunch of our test syscalls in scenarios that are
		 * expected to succeed in a tight loop to look
		 * for concurrency problems.
		 */
		ret = string_nlen(user_string, BUF_SIZE, &err);
		zassert_equal(err, 0, "user string faulted");
		zassert_equal(ret, strlen(user_string),
			      "incorrect length returned");

		ret = string_alloc_copy("this is a kernel string");
		zassert_equal(ret, 0, "string should have matched");

		ret = string_copy("this is a kernel string");
		zassert_equal(ret, 0, "string should have matched");

		ret = to_copy(buf);
		zassert_equal(ret, 0, "copy should have been a success");
		ret = strcmp(buf, user_string);
		zassert_equal(ret, 0, "string should have matched");

		test_arg64();

		if (count++ == 30000) {
			printk("%ld", id);
			count = 0;
		}
	}
}

void test_syscall_torture(void)
{
	uintptr_t i;

	printk("Running syscall torture test with %d threads on %d cpu(s)\n",
	       NR_THREADS, CONFIG_MP_NUM_CPUS);

	for (i = 0; i < NR_THREADS; i++) {
		k_thread_create(&torture_threads[i], torture_stacks[i],
				STACK_SZ, syscall_torture,
				(void *)i, NULL, NULL,
				2, K_INHERIT_PERMS | K_USER, K_NO_WAIT);
	}

	/* Let the torture threads hog the system for 15 seconds before we
	 * abort them.
	 * They will all be hammering the cpu(s) with system calls,
	 * hopefully smoking out any issues and causing a crash.
	 */
	k_sleep(K_MSEC(SLEEP_MS_LONG));

	for (i = 0; i < NR_THREADS; i++) {
		k_thread_abort(&torture_threads[i]);
	}

	printk("\n");
}

bool z_impl_syscall_context(void)
{
	return z_is_in_user_syscall();
}

static inline bool z_vrfy_syscall_context(void)
{
	return z_impl_syscall_context();
}
#include <syscalls/syscall_context_mrsh.c>

void test_syscall_context_user(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(syscall_context(),
		     "not reported in user syscall");
}

/* Show that z_is_in_syscall() works properly */
void test_syscall_context(void)
{
	/* We're a regular supervisor thread. */
	zassert_false(z_is_in_user_syscall(),
		      "reported in user syscall when in supv. thread ctx");

	/* Make a system call from supervisor mode. The check in the
	 * implementation function should return false.
	 */
	zassert_false(syscall_context(),
		      "reported in user syscall when called from supervisor");

	/* Remainder of the test in user mode */
	k_thread_user_mode_enter(test_syscall_context_user, NULL, NULL, NULL);
}

K_HEAP_DEFINE(test_heap, BUF_SIZE * (4 * NR_THREADS));

void test_main(void)
{
	sprintf(kernel_string, "this is a kernel string");
	sprintf(user_string, "this is a user string");
	k_thread_heap_assign(k_current_get(), &test_heap);

	ztest_test_suite(syscalls,
			 ztest_unit_test(test_string_nlen),
			 ztest_user_unit_test(test_string_nlen),
			 ztest_user_unit_test(test_to_copy),
			 ztest_user_unit_test(test_user_string_copy),
			 ztest_user_unit_test(test_user_string_alloc_copy),
			 ztest_user_unit_test(test_arg64),
			 ztest_user_unit_test(test_more_args),
			 ztest_unit_test(test_syscall_torture),
			 ztest_unit_test(test_syscall_context)
			 );
	ztest_run_test_suite(syscalls);
}

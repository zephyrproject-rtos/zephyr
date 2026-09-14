/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/ztest.h>
#include <zephyr/linker/linker-defs.h>
#include "test_syscalls.h"
#include <mmu.h>

#define BUF_SIZE	32

#if defined(CONFIG_BOARD_FVP_BASE_REVC_2XAEM)
#define SLEEP_MS_LONG	30000
#elif defined(CONFIG_WHISPER_TARGET) || defined(CONFIG_INTEL_ADSP_SIM)
#define SLEEP_MS_LONG	300
#elif defined(CONFIG_QEMU_TARGET)
/* Emulators are much slower than real hardware, especially in the kyield
 * scenario where every syscall forces a userspace context switch.
 */
#define SLEEP_MS_LONG	300
#else
#define SLEEP_MS_LONG	15000
#endif

#if defined(CONFIG_BOARD_NUCLEO_F429ZI) || defined(CONFIG_BOARD_NUCLEO_F207ZG) \
	|| defined(CONFIG_BOARD_NUCLEO_L073RZ) \
	|| defined(CONFIG_BOARD_RONOTH_LODEV)
#define FAULTY_ADDRESS 0x0FFFFFFF
#elif defined(CONFIG_BOARD_QEMU_CORTEX_R5) || defined(CONFIG_BOARD_VERSAL_RPU)
#define FAULTY_ADDRESS 0x3FFFFFFF
#elif CONFIG_MMU
/* Just past the zephyr image mapping should be a non-present page */
#define FAULTY_ADDRESS K_MEM_VM_FREE_START
#else
#define FAULTY_ADDRESS 0xFFFFFFF0
#endif

#if !defined(CONFIG_TIMESLICING) || (CONFIG_TIMESLICE_SIZE == 0)
#define YIELD_KERNEL		z_impl_k_yield()
#define YIELD_USER		k_yield()
#else
#define YIELD_KERNEL
#define YIELD_USER
#endif

#define NR_THREADS	(arch_num_cpus() * 4)
#define MAX_NR_THREADS	(CONFIG_MP_MAX_NUM_CPUS * 4)
#define STACK_SZ	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

struct k_thread stress_threads[MAX_NR_THREADS];
K_THREAD_STACK_ARRAY_DEFINE(stress_stacks, MAX_NR_THREADS, STACK_SZ);

char kernel_string[BUF_SIZE];
char kernel_buf[MAX_NR_THREADS][BUF_SIZE];
ZTEST_BMEM char user_string[BUF_SIZE];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	printk("Unexpected fault during test\n");
	TC_END_REPORT(TC_FAIL);
	k_fatal_halt(reason);
}

size_t z_impl_string_nlen(char *src, size_t maxlen, int *err)
{
	YIELD_KERNEL;

	return k_usermode_string_nlen(src, maxlen, err);
}

static inline size_t z_vrfy_string_nlen(char *src, size_t maxlen, int *err)
{
	int err_copy;
	size_t ret;

	ret = z_impl_string_nlen((char *)src, maxlen, &err_copy);

	YIELD_KERNEL;

	if (!err_copy && K_SYSCALL_MEMORY_READ(src, ret + 1)) {
		err_copy = -1;
	}

	YIELD_KERNEL;

	K_OOPS(k_usermode_to_copy((int *)err, &err_copy, sizeof(err_copy)));

	return ret;
}
#include <zephyr/syscalls/string_nlen_mrsh.c>

int z_impl_string_alloc_copy(char *src)
{
	YIELD_KERNEL;

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

	YIELD_KERNEL;

	src_copy = k_usermode_string_alloc_copy((char *)src, BUF_SIZE);
	if (!src_copy) {
		return -1;
	}

	YIELD_KERNEL;

	ret = z_impl_string_alloc_copy(src_copy);

	YIELD_KERNEL;

	k_free(src_copy);

	return ret;
}
#include <zephyr/syscalls/string_alloc_copy_mrsh.c>

int z_impl_string_copy(char *src, int id)
{
	ARG_UNUSED(id);

	YIELD_KERNEL;

	if (!strcmp(src, kernel_string)) {
		return 0;
	} else {
		return ESRCH;
	}
}

static inline int z_vrfy_string_copy(char *src, int id)
{
	YIELD_KERNEL;

	int ret = k_usermode_string_copy(kernel_buf[id], (char *)src, BUF_SIZE);

	YIELD_KERNEL;

	if (ret) {
		return ret;
	}

	return z_impl_string_copy(kernel_buf[id], id);
}
#include <zephyr/syscalls/string_copy_mrsh.c>

/* Not actually used, but will copy wrong string if called by mistake instead
 * of the handler
 */
int z_impl_to_copy(char *dest)
{
	YIELD_KERNEL;

	memcpy(dest, kernel_string, BUF_SIZE);
	return 0;
}

static inline int z_vrfy_to_copy(char *dest)
{
	YIELD_KERNEL;

	return k_usermode_to_copy((char *)dest, user_string, BUF_SIZE);
}
#include <zephyr/syscalls/to_copy_mrsh.c>

int z_impl_syscall_arg64(uint64_t arg)
{
	YIELD_USER;

	/* "Hash" (heh) the return to avoid accidental false positives
	 * due to using common/predictable values.
	 */
	return (int)(arg + 0x8c32a9eda4ca2621ULL + (size_t)&kernel_string);
}

static inline int z_vrfy_syscall_arg64(uint64_t arg)
{
	return z_impl_syscall_arg64(arg);
}
#include <zephyr/syscalls/syscall_arg64_mrsh.c>

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

	YIELD_USER;

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
#include <zephyr/syscalls/syscall_arg64_big_mrsh.c>

uint32_t z_impl_more_args(uint32_t arg1, uint32_t arg2, uint32_t arg3,
			  uint32_t arg4, uint32_t arg5, uint32_t arg6,
			  uint32_t arg7)
{
	uint32_t ret = 0x4ef464cc;
	uint32_t args[] = { arg1, arg2, arg3, arg4, arg5, arg6, arg7 };

	YIELD_USER;

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
#include <zephyr/syscalls/more_args_mrsh.c>

/**
 * @brief Verify that k_usermode_string_nlen() measures only memory the caller
 *        may read.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * String lengths handed to the kernel come from untrusted callers, so the
 * measuring helper has to respect the caller's permissions: from user mode a
 * kernel-owned string or a wild address must fault into the error argument
 * rather than being read, while a string the caller owns measures normally.
 * The same call from supervisor mode may read everything. The case runs in
 * both modes via ZTEST_USER, so both sides of that contract are covered by
 * one body.
 *
 * Test steps:
 * - Measure a kernel-owned string; expect a fault in user mode and a correct
 *   length in supervisor mode.
 * - Measure a user-owned string; expect the correct length in both modes.
 * - Measure a known-faulty address; expect a fault (skipped on platforms
 *   whose emulation cannot fault it).
 *
 * Expected result:
 * - Lengths are returned only for memory the caller is allowed to read, and
 *   every forbidden access is reported through the error argument.
 *
 * @see k_usermode_string_nlen()
 */
ZTEST_USER(syscalls, test_syscall_string_nlen)
{
	int err;
	size_t ret;

	ret = string_nlen(kernel_string, BUF_SIZE, &err);
	if (arch_is_user_context()) {
		zassert_equal(err, -1,
			      "kernel string did not fault on user access (%d)", err);
	} else {
		zassert_equal(err, 0, "kernel string faulted in kernel mode (%d)", err);
		zassert_equal(ret, strlen(kernel_string),
			      "incorrect length returned (%zu)", ret);
	}

	/* Valid usage */
	ret = string_nlen(user_string, BUF_SIZE, &err);
	zassert_equal(err, 0, "user string faulted (%d)", err);
	zassert_equal(ret, strlen(user_string), "incorrect length returned (%zu)", ret);

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
 * @brief Test that string_nlen with maxlen 0 does not touch the string
 *
 * @details strnlen() semantics require examining at most maxlen bytes,
 * so with maxlen == 0 the source pointer must not be dereferenced at
 * all: even an inaccessible address must yield length 0 and no error.
 * Regression test for arch implementations that loaded the string
 * byte before checking the length limit.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_usermode_string_nlen()
 */
ZTEST(syscalls, test_syscall_string_nlen_maxsize_zero)
{
	int err = 0;
	size_t ret;
	char buf[8] = "abcdefg";

	ret = k_usermode_string_nlen((const char *)FAULTY_ADDRESS, 0, &err);
	zassert_equal(ret, 0, "maxlen=0 returned %zu", ret);
	zassert_equal(err, 0, "maxlen=0 dereferenced the pointer (err %d)", err);

	/* Capped length: no NUL within maxlen must return maxlen. */
	err = 0;
	ret = k_usermode_string_nlen(buf, 4, &err);
	zassert_equal(ret, 4, "capped length returned %zu", ret);
	zassert_equal(err, 0, "capped length faulted (err %d)", err);
}

/**
 * @brief Verify that k_usermode_string_alloc_copy() copies only readable,
 *        fitting strings.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The allocating copy helper brings a user string into kernel memory before
 * the kernel looks at it, so it must reject a string that does not match the
 * expectation, one that exceeds the destination size, and one the caller has
 * no right to read -- the kernel_string case, where the copy itself must
 * fault. Only a readable string of acceptable length may be copied and
 * compared successfully.
 *
 * Test steps:
 * - Pass a valid but unexpected string and check the comparison fails.
 * - Pass an over-long string and check the copy is rejected.
 * - Pass a kernel-owned string and check the copy faults.
 * - Pass the expected string and check it copies and matches.
 *
 * Expected result:
 * - Each invalid input is rejected with its own error and only the readable,
 *   fitting string succeeds.
 *
 * @see k_usermode_string_alloc_copy()
 */
ZTEST_USER(syscalls, test_syscall_string_alloc_copy)
{
	int ret;

	ret = string_alloc_copy("asdkajshdazskjdh");
	zassert_equal(ret, -2, "string_alloc_copy: 1: got %d", ret);

	ret = string_alloc_copy(
	    "asdkajshdazskjdhikfsdjhfskdjfhsdkfjhskdfjhdskfjhs");
	zassert_equal(ret, -1, "string_alloc_copy: 2: got %d", ret);

	ret = string_alloc_copy(kernel_string);
	zassert_equal(ret, -1, "string_alloc_copy: 3: got %d", ret);

	ret = string_alloc_copy("this is a kernel string");
	zassert_equal(ret, 0, "string_alloc_copy: string should have matched (%d)", ret);
}

/**
 * @brief Verify that k_usermode_string_copy() copies only readable, fitting
 *        strings.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The fixed-buffer counterpart of the allocating copy: same contract, but
 * into a caller-provided buffer, with EINVAL for a string that does not fit
 * and EFAULT for one the caller cannot read.
 *
 * Test steps:
 * - Pass a valid but unexpected string and check the comparison fails.
 * - Pass an over-long string and check EINVAL is returned.
 * - Pass a kernel-owned string and check EFAULT is returned.
 * - Pass the expected string and check it copies and matches.
 *
 * Expected result:
 * - Each invalid input is rejected with its own error and only the readable,
 *   fitting string succeeds.
 *
 * @see k_usermode_string_copy()
 */
ZTEST_USER(syscalls, test_syscall_string_copy)
{
	int ret;

	ret = string_copy("asdkajshdazskjdh", 0);
	zassert_equal(ret, ESRCH, "string_copy: 1: got %d", ret);

	ret = string_copy("asdkajshdazskjdhikfsdjhfskdjfhsdkfjhskdfjhdskfjhs", 0);
	zassert_equal(ret, EINVAL, "string_copy: 2: got %d", ret);

	ret = string_copy(kernel_string, 0);
	zassert_equal(ret, EFAULT, "string_copy: 3: got %d", ret);

	ret = string_copy("this is a kernel string", 0);
	zassert_equal(ret, 0, "string_copy: string should have matched (%d)", ret);
}

/**
 * @brief Verify that k_usermode_to_copy() writes only to caller-writable
 *        memory.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Copying data out of the kernel back to the caller must validate the
 * destination the same way inbound copies validate the source: a buffer the
 * user thread cannot write has to be refused with EFAULT, and a writable
 * buffer receives the data intact.
 *
 * Test steps:
 * - Ask the kernel to copy into a kernel-owned buffer; expect EFAULT.
 * - Ask it to copy into a stack buffer of the calling user thread.
 * - Compare the received data with what the kernel was asked to send.
 *
 * Expected result:
 * - The unwritable destination is rejected and the writable one receives the
 *   exact data.
 *
 * @see k_usermode_to_copy()
 */
ZTEST_USER(syscalls, test_syscall_to_copy)
{
	char buf[BUF_SIZE];
	int ret;

	ret = to_copy(kernel_buf[0]);
	zassert_equal(ret, EFAULT, "to_copy: should have faulted (%d)", ret);

	ret = to_copy(buf);
	zassert_equal(ret, 0, "to_copy: copy should have been a success (%d)", ret);
	ret = strcmp(buf, user_string);
	zassert_equal(ret, 0, "to_copy: string should have matched (%d)", ret);
}

void run_test_arg64(void)
{
	zassert_equal(syscall_arg64(54321),
		      z_impl_syscall_arg64(54321),
		      "syscall (arg64) didn't match impl");

	zassert_equal(syscall_arg64_big(1, 2, 3, 4, 5, 6),
		      z_impl_syscall_arg64_big(1, 2, 3, 4, 5, 6),
		      "syscall (arg64_big) didn't match impl");
}

/**
 * @brief Test passing 64-bit arguments through a system call
 *
 * @details Invoke system calls that take 64-bit arguments from user mode and
 * verify the values received by the implementation match those passed in,
 * confirming user threads can perform privileged operations through system
 * calls with wide arguments.
 *
 * @ingroup kernel_memprotect_tests
 * @see syscall_arg64()
 */
ZTEST_USER(syscalls, test_syscall_arg64)
{
	run_test_arg64();
}

/**
 * @brief Test passing the maximum number of system call arguments
 *
 * @details Invoke a system call from user mode that takes more arguments than
 * fit in registers and verify the implementation receives all of them intact,
 * confirming user threads can perform privileged operations through system
 * calls with many arguments.
 *
 * @ingroup kernel_memprotect_tests
 */
ZTEST_USER(syscalls, test_syscall_more_args)
{
	zassert_equal(more_args(1, 2, 3, 4, 5, 6, 7),
		      z_impl_more_args(1, 2, 3, 4, 5, 6, 7),
		      "syscall (more_args) didn't match impl");
}

void syscall_switch_stress(void *arg1, void *arg2, void *arg3)
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
		zassert_equal(err, 0, "stress: user string faulted (%d)", err);
		zassert_equal(ret, strlen(user_string),
			      "stress: incorrect length returned (%d)", ret);

		YIELD_USER;

		ret = string_alloc_copy("this is a kernel string");
		zassert_equal(ret, 0,
			      "stress: string_alloc_copy: string should have matched (%d)", ret);

		YIELD_USER;

		ret = string_copy("this is a kernel string", (int)id);
		zassert_equal(ret, 0, "stress: string_copy: string should have matched (%d)", ret);

		YIELD_USER;

		ret = to_copy(buf);
		zassert_equal(ret, 0,
			      "stress: to_copy: copy should have been a success (%d)", ret);

		YIELD_USER;

		ret = strcmp(buf, user_string);
		zassert_equal(ret, 0, "stress: strcmp: string should have matched (%d)", ret);

		YIELD_USER;

		run_test_arg64();

		if (count++ == 30000) {
			printk("%ld", id);
			count = 0;
		}

		YIELD_USER;
	}
}

/**
 * @brief Stress the system call path with many concurrent user threads.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The entry and exit paths of a system call touch per-CPU and per-thread
 * state that has to hold up when every CPU is crossing the privilege boundary
 * at once. Four user threads per CPU hammer the string-handling test calls in
 * a tight loop, yielding periodically so the schedulers on all CPUs keep
 * moving threads around; corruption anywhere in the path shows up as a failed
 * validation inside one of the calls.
 *
 * Test steps:
 * - Spawn four user threads per CPU running the string system calls in a
 *   loop.
 * - Let them run for the configured iteration count and join them all.
 *
 * Expected result:
 * - Every thread completes every iteration without a validation failure or
 *   fault.
 */
ZTEST(syscalls_extended, test_syscall_switch_stress)
{
	uintptr_t i;

	printk("Running syscall switch stress test with %d threads on %d cpu(s)\n",
	       NR_THREADS, arch_num_cpus());

	for (i = 0; i < NR_THREADS; i++) {
		k_thread_create(&stress_threads[i], stress_stacks[i],
				STACK_SZ, syscall_switch_stress,
				(void *)i, NULL, NULL,
				2, K_INHERIT_PERMS | K_USER, K_NO_WAIT);
	}

	/* Let the stress threads hog the system for several seconds before
	 * we abort them.
	 * They will all be hammering the cpu(s) with system calls,
	 * hopefully smoking out any issues and causing a crash.
	 */
	k_sleep(K_MSEC(SLEEP_MS_LONG));

	for (i = 0; i < NR_THREADS; i++) {
		k_thread_abort(&stress_threads[i]);
	}

	for (i = 0; i < NR_THREADS; i++) {
		k_thread_join(&stress_threads[i], K_FOREVER);
	}

	printk("\n");
}

bool z_impl_syscall_context(void)
{
	return k_is_in_user_syscall();
}

static inline bool z_vrfy_syscall_context(void)
{
	return z_impl_syscall_context();
}
#include <zephyr/syscalls/syscall_context_mrsh.c>

void test_syscall_context_user(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(syscall_context(),
		     "not reported in user syscall");
}

/**
 * @brief Verify that k_is_in_user_syscall() identifies user-invoked calls.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Some kernel code behaves differently depending on whether it was entered
 * through a system call from user mode, so the predicate has to be true
 * exactly then: not in an ordinary supervisor thread, not when the same
 * function is invoked directly in supervisor mode, only inside a system call
 * made by a user thread.
 *
 * Test steps:
 * - Check the predicate is false in the supervisor-mode test thread.
 * - Invoke the test system call from supervisor mode and check it reports
 *   false inside.
 * - Enter user mode and invoke the same call, checking it reports true.
 *
 * Expected result:
 * - The predicate is true only inside a system call invoked from user mode.
 *
 * @see k_is_in_user_syscall()
 */
ZTEST(syscalls, test_syscall_context)
{
	/* We're a regular supervisor thread. */
	zassert_false(k_is_in_user_syscall(),
		      "reported in user syscall when in supv. thread ctx");

	/* Make a system call from supervisor mode. The check in the
	 * implementation function should return false.
	 */
	zassert_false(syscall_context(),
		      "reported in user syscall when called from supervisor");

	/* Remainder of the test in user mode */
	k_thread_user_mode_enter(test_syscall_context_user, NULL, NULL, NULL);
}

K_HEAP_DEFINE(test_heap, BUF_SIZE * (4 * MAX_NR_THREADS));

void *syscalls_setup(void)
{
	sprintf(kernel_string, "this is a kernel string");
	sprintf(user_string, "this is a user string");
	k_thread_heap_assign(k_current_get(), &test_heap);

	return NULL;
}

ZTEST_SUITE(syscalls, NULL, syscalls_setup, NULL, NULL, NULL);
ZTEST_SUITE(syscalls_extended, NULL, syscalls_setup, NULL, NULL, NULL);

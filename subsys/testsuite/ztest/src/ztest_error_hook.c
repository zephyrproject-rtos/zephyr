/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>


#if defined(CONFIG_ZTEST_FATAL_HOOK)
/* This is a flag indicate if treating fatal error as expected, then take
 * action dealing with it. It's SMP-safe.
 */
ZTEST_BMEM volatile bool fault_in_isr;
ZTEST_BMEM volatile k_tid_t valid_fault_tid;

static inline void reset_stored_fault_status(void)
{
	valid_fault_tid = NULL;
	fault_in_isr = false;
}

void z_impl_ztest_set_fault_valid(bool valid)
{
	if (valid) {
		if (k_is_in_isr()) {
			fault_in_isr = true;
		} else {
			valid_fault_tid = k_current_get();
		}
	} else {
		reset_stored_fault_status();
	}
}

#if defined(CONFIG_USERSPACE)
static inline void z_vrfy_ztest_set_fault_valid(bool valid)
{
	z_impl_ztest_set_fault_valid(valid);
}
#include <syscalls/ztest_set_fault_valid_mrsh.c>
#endif

__weak void ztest_post_fatal_error_hook(unsigned int reason,
		const z_arch_esf_t *pEsf)
{
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	k_tid_t curr_tid = k_current_get();
	bool valid_fault = (curr_tid == valid_fault_tid) || fault_in_isr;

	printk("Caught system error -- reason %d %d\n", reason, valid_fault);

	if (valid_fault) {
		printk("Fatal error expected as part of test case.\n");

		/* reset back to normal */
		reset_stored_fault_status();

		/* do some action after expected fatal error happened */
		ztest_post_fatal_error_hook(reason, pEsf);
	} else {
		printk("Fatal error was unexpected, aborting...\n");
		k_fatal_halt(reason);
	}
}
#endif


#if defined(CONFIG_ZTEST_ASSERT_HOOK)
/* This is a flag indicate if treating assert fail as expected, then take
 * action dealing with it. It's SMP-safe.
 */
ZTEST_BMEM volatile bool assert_in_isr;
ZTEST_BMEM volatile k_tid_t valid_assert_tid;

static inline void reset_stored_assert_status(void)
{
	valid_assert_tid = NULL;
	assert_in_isr = 0;
}

void z_impl_ztest_set_assert_valid(bool valid)
{
	if (valid) {
		if (k_is_in_isr()) {
			assert_in_isr = true;
		} else {
			valid_assert_tid = k_current_get();
		}
	} else {
		reset_stored_assert_status();
	}
}

#if defined(CONFIG_USERSPACE)
static inline void z_vrfy_ztest_set_assert_valid(bool valid)
{
	z_impl_ztest_set_assert_valid(valid);
}
#include <syscalls/ztest_set_assert_valid_mrsh.c>
#endif

__weak void ztest_post_assert_fail_hook(void)
{
	k_thread_abort(k_current_get());
}

#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
	ARG_UNUSED(file);
	ARG_UNUSED(line);
#endif

	printk("Caught assert failed\n");

	if ((k_current_get() == valid_assert_tid) || assert_in_isr) {
		printk("Assert error expected as part of test case.\n");

		/* reset back to normal */
		reset_stored_assert_status();

		/* It won't go back to caller when assert failed, and it
		 * will terminate the thread.
		 */
		ztest_post_assert_fail_hook();
	} else {
		printk("Assert failed was unexpected, aborting...\n");
#ifdef CONFIG_USERSPACE
	/* User threads aren't allowed to induce kernel panics; generate
	 * an oops instead.
	 */
		if (_is_user_context()) {
			k_oops();
		}
#endif
		k_panic();
	}
}
#endif

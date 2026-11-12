/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/cortex_a_r/cpu.h>
#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#if defined(CONFIG_IRQ_OFFLOAD)
#include <zephyr/irq_offload.h>

static atomic_t offload_calls;

static void offload_probe(const void *parameter)
{
	ARG_UNUSED(parameter);
	atomic_inc(&offload_calls);
}
#endif

static K_THREAD_STACK_ARRAY_DEFINE(user_stacks, 3, 2048);
static struct k_thread user_threads[3];
static K_THREAD_STACK_DEFINE(kernel_stack, 2048);
static struct k_thread kernel_thread;
static volatile uint32_t fault_reason;
static volatile uint32_t fault_mode;
static volatile bool fault_seen;

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	fault_reason = reason;
	fault_mode = (esf != NULL) ? (esf->basic.xpsr & MODE_MASK) : 0U;
	fault_seen = true;
}

static void user_svc(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	ztest_set_fault_valid(true);

	switch ((uintptr_t)arg1) {
	case 0U:
		__asm__ volatile ("svc #0" ::: "memory");
		break;
	case 1U:
		__asm__ volatile ("svc #1" ::: "memory");
		break;
	default:
		__asm__ volatile ("svc #42" ::: "memory");
		break;
	}

	/* An invalid SVC must not return to the calling thread. */
	k_sleep(K_FOREVER);
}

static void run_user_svc(uintptr_t number, size_t index)
{
	k_tid_t tid;
	int ret;

	fault_seen = false;
	tid = k_thread_create(&user_threads[index], user_stacks[index],
			      K_THREAD_STACK_SIZEOF(user_stacks[index]), user_svc,
			      (void *)number, NULL, NULL, K_PRIO_PREEMPT(0), K_USER, K_NO_WAIT);
	ret = k_thread_join(tid, K_SECONDS(5));
	if (ret != 0) {
		k_thread_abort(tid);
	}

	zassert_ok(ret, "user SVC did not terminate the thread");
	zassert_true(fault_seen, "user SVC did not raise an oops");
	zassert_equal(fault_reason, K_ERR_KERNEL_OOPS, "unexpected fatal reason");
	zassert_equal(fault_mode, MODE_USR, "fault did not originate in user mode");
}

static void kernel_svc_unknown(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	ztest_set_fault_valid(true);
	__asm__ volatile ("mov r0, %[reason]\n\tsvc #42"
			  :
			  : [reason] "i" (K_ERR_KERNEL_OOPS)
			  : "r0", "memory");
	k_sleep(K_FOREVER);
}

ZTEST(arm_cortex_r_svc, test_user_cannot_invoke_privileged_svcs)
{
#if defined(CONFIG_IRQ_OFFLOAD)
	atomic_set(&offload_calls, 0);
	irq_offload(offload_probe, NULL);
	zassert_equal(atomic_get(&offload_calls), 1, "privileged offload failed");
#endif

	run_user_svc(0U, 0U);
	run_user_svc(1U, 1U);
	run_user_svc(42U, 2U);

#if defined(CONFIG_IRQ_OFFLOAD)
	zassert_equal(atomic_get(&offload_calls), 1, "user SVC replayed the offload");
	irq_offload(offload_probe, NULL);
	zassert_equal(atomic_get(&offload_calls), 2, "privileged offload failed after user SVC");
#endif
}

ZTEST(arm_cortex_r_svc, test_unknown_privileged_svc_rejected)
{
	k_tid_t tid;
	int ret;

#if defined(CONFIG_IRQ_OFFLOAD)
	atomic_set(&offload_calls, 0);
	irq_offload(offload_probe, NULL);
	zassert_equal(atomic_get(&offload_calls), 1, "privileged offload failed");
#endif

	fault_seen = false;
	tid = k_thread_create(&kernel_thread, kernel_stack, K_THREAD_STACK_SIZEOF(kernel_stack),
			      kernel_svc_unknown, NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0,
			      K_NO_WAIT);
	ret = k_thread_join(tid, K_SECONDS(5));
	if (ret != 0) {
		k_thread_abort(tid);
	}

	zassert_ok(ret, "unknown SVC did not terminate the thread");
	zassert_true(fault_seen, "unknown SVC did not raise an oops");
	zassert_equal(fault_reason, K_ERR_KERNEL_OOPS, "unexpected fatal reason");
	zassert_equal(fault_mode, MODE_SYS, "fault did not originate in system mode");

#if defined(CONFIG_IRQ_OFFLOAD)
	zassert_equal(atomic_get(&offload_calls), 1, "unknown SVC replayed the offload");
#endif
}

ZTEST_SUITE(arm_cortex_r_svc, NULL, NULL, NULL, NULL, NULL);

/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(STACK_UNWIND_BUSY_LOG_TEST) || defined(STACK_UNWIND_BLOCKED_LOG_TEST)
LOG_MODULE_REGISTER(stack_unwind, LOG_LEVEL_INF);
#endif

#if defined(STACK_UNWIND_BLOCKED_LOG_TEST)
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/atomic.h>

static atomic_t block_log_thread;
static atomic_t log_thread_blocked;

/* Once told to, blocks the logging thread for good in the middle of a message */
static void blocking_backend_process(const struct log_backend *const backend,
				     union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	ARG_UNUSED(msg);

	if (atomic_cas(&block_log_thread, 1, 0)) {
		atomic_set(&log_thread_blocked, 1);
		k_sleep(K_FOREVER);
	}
}

static void blocking_backend_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api blocking_backend_api = {
	.process = blocking_backend_process,
	.panic = blocking_backend_panic,
};

LOG_BACKEND_DEFINE(blocking_backend, blocking_backend_api, true);
#endif

#if defined(STACK_UNWIND_LEAF_TEST)
extern void leaf_fault(void);

static void (*volatile leaf_fault_call)(void) = leaf_fault;

static void __noinline leaf_caller(void)
{
	leaf_fault_call();
	printf("unexpected return\n");
}
#else
static void func1(int a);
static void func2(int a);

static void __noinline func2(int a)
{
	printf("%d: %s\n", a, __func__);

	if (a >= 5) {
#if defined(STACK_UNWIND_NONLEAF_TEST)
		__asm__ volatile(".word 0");
#else
		k_oops();
#endif
	}

	func1(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}

static void __noinline func1(int a)
{
	printf("%d: %s\n", a, __func__);
	func2(a + 1);
	printf("bottom %d: %s\n", a, __func__);
}
#endif

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);

#if defined(STACK_UNWIND_BUSY_LOG_TEST)
	/* Keep the logging thread printing on another CPU while the fault is reported */
	for (int i = 0; i < 20; i++) {
		LOG_INF("message %d before the fault", i);
	}
#endif

#if defined(STACK_UNWIND_BLOCKED_LOG_TEST)
	/*
	 * Leave the logging thread blocked with a message claimed. This thread
	 * keeps its CPU meanwhile, so the logging thread runs on another one.
	 */
	atomic_set(&block_log_thread, 1);
	LOG_INF("message before the fault");
	log_thread_trigger();
	while (atomic_get(&log_thread_blocked) == 0) {
		k_busy_wait(USEC_PER_MSEC);
	}
#endif

#if defined(STACK_UNWIND_LEAF_TEST)
	leaf_caller();
#else
	func1(1);
#endif

	return 0;
}

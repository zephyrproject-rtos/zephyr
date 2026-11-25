/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/kernel.h>
#include <zephyr/call_offloader/call_offloader.h>

#define WORKER_STACK_SZ 1024
#define CALLER_STACK_SZ 1024

CALL_OFFLOADER_ENGINE_DEFINE(fault_eng, 1, WORKER_STACK_SZ);

static K_THREAD_STACK_DEFINE(caller_stack, CALLER_STACK_SZ);
static struct k_thread caller_thread;

static volatile bool dispatch_returned;
static k_tid_t faulting_tid;

static void faulting_work(void *args)
{
	ARG_UNUSED(args);

	faulting_tid = k_current_get();

	ztest_set_fault_valid(true);
	k_oops();
}

static void caller_entry(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	call_offloader_dispatch(&fault_eng, faulting_work, NULL);

	dispatch_returned = true;
}

static void store_value(void *args)
{
	*(int *)args = 42;
}

ZTEST(call_offloader_fault, test_worker_fault_aborts_the_caller_and_respawns_the_worker)
{
	k_tid_t caller;
	int value = 0;

	caller = k_thread_create(&caller_thread, caller_stack, CALLER_STACK_SZ, caller_entry,
				 NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	zassert_ok(k_thread_join(caller, K_SECONDS(1)), "the caller did not terminate");
	zassert_false(dispatch_returned, "the dispatch returned after the worker faulted");
	zassert_true(faulting_tid != caller, "the work did not run on a worker");

	call_offloader_dispatch(&fault_eng, store_value, &value);
	zassert_equal(value, 42, "the engine did not serve a call after the worker faulted");
}

ZTEST_SUITE(call_offloader_fault, NULL, NULL, NULL, NULL, NULL);

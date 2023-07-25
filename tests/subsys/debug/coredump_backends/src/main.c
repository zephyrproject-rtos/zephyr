/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/ztest.h>
#include <assert.h>
#include <zephyr/tc_util.h>

#include <zephyr/debug/coredump.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static struct k_thread dump_thread;
static K_THREAD_STACK_DEFINE(dump_stack, STACK_SIZE);

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(pEsf);
}

void dump_entry(void *p1, void *p2, void *p3)
{
	unsigned int key;

	key = irq_lock();
	k_oops();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	irq_unlock(key);
}

static void check_errors(void)
{
	int ret;

	/* Check backend error if backend supports this query */
	ret = coredump_query(COREDUMP_QUERY_GET_ERROR, NULL);
	if (ret != -ENOTSUP) {
		zassert_equal(ret, 0, "Error encountered! (%d)", ret);
	}
}

void test_coredump(void)
{
	k_tid_t tid;

	/* Create a thread that crashes */
	tid = k_thread_create(&dump_thread, dump_stack,
			      K_THREAD_STACK_SIZEOF(dump_stack),
			      dump_entry, NULL, NULL, NULL,
			      0, 0, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	check_errors();
}

void test_query_stored_dump(void)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_errors();

	/* There should be a stored coredump now if backend has storage */
	ret = coredump_query(COREDUMP_QUERY_HAS_STORED_DUMP, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	} else if (ret == 1) {
		check_errors();
		ztest_test_pass();
	} else if (ret == 0) {
		TC_PRINT("Should have stored dump!\n");
		ztest_test_fail();
	} else {
		TC_PRINT("Error reading stored dump! (%d)\n", ret);
		ztest_test_fail();
	}
}

void test_verify_stored_dump(void)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_errors();

	/* There should be a stored coredump now if backend has storage */
	ret = coredump_cmd(COREDUMP_CMD_VERIFY_STORED_DUMP, NULL);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	} else if (ret == 1) {
		check_errors();
		ztest_test_pass();
	} else if (ret == 0) {
		TC_PRINT("Verification of stored dump failed!\n");
		ztest_test_fail();
	} else {
		TC_PRINT("Error reading stored dump! (%d)\n", ret);
		ztest_test_fail();
	}
}

ZTEST(coredump_backends, test_coredump_backend) {
	test_coredump();
	test_query_stored_dump();
	test_verify_stored_dump();
}

ZTEST_SUITE(coredump_backends, NULL, NULL, NULL, NULL, NULL);

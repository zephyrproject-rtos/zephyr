/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020-2023, Intel Corporation.
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

void check_error(void)
{
	int ret;

	/* Check backend error if backend supports this query */
	ret = coredump_query(COREDUMP_QUERY_GET_ERROR, NULL);
	if (ret != -ENOTSUP) {
		zassert_equal(ret, 0, "Error encountered! (%d)", ret);
	}
}

void clear_error(void)
{
	int ret;

	/* Clear backend error if backend supports this query */
	ret = coredump_cmd(COREDUMP_CMD_CLEAR_ERROR, NULL);
	if (ret != -ENOTSUP) {
		zassert_equal(ret, 0, "Error encountered! (%d)", ret);
	}
}

static void *raise_coredump(void)
{
	k_tid_t tid;

	clear_error();

	/* Create a thread that crashes */
	tid = k_thread_create(&dump_thread, dump_stack,
			      K_THREAD_STACK_SIZEOF(dump_stack),
			      dump_entry, NULL, NULL, NULL,
			      0, 0, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);

	return &dump_thread;
}

void test_has_stored_dump(bool is_expected)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_error();

	/* There should be a stored coredump now if backend has storage */
	ret = coredump_query(COREDUMP_QUERY_HAS_STORED_DUMP, NULL);
	if (ret == -ENOTSUP) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		TC_ERROR("Can't query stored dump: unexpectedly not supported.\n");
		ztest_test_fail();
#else
		TC_PRINT("Can't query stored dump: expectedly not supported.\n");
		ztest_test_skip();
#endif
	} else if (ret == 1) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		check_error();
		zassert_true(is_expected, "Unexpected coredump found.\n");
		ztest_test_pass();
#else
		TC_ERROR("Can't have a stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else if (ret == 0) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		check_error();
		zassert_false(is_expected, "Should have stored dump!\n");
		ztest_test_pass();
#else
		TC_ERROR("Can't have an empty stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else {
		TC_ERROR("Error reading stored dump! (%d)\n", ret);
		ztest_test_fail();
	}
}

void test_verify_stored_dump(void)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_error();

	/* There should be a stored coredump now if backend has storage */
	ret = coredump_cmd(COREDUMP_CMD_VERIFY_STORED_DUMP, NULL);
	if (ret == -ENOTSUP) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		TC_ERROR("Can't verify stored dump: unexpectedly not supported.\n");
		ztest_test_fail();
#else
		TC_PRINT("Can't verify stored dump: expectedly not supported.\n");
		ztest_test_skip();
#endif
	} else if (ret == 1) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		check_error();
		ztest_test_pass();
#else
		TC_ERROR("Can't have a stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else if (ret == 0) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		TC_ERROR("Verification of stored dump failed!\n");
		ztest_test_fail();
#else
		TC_ERROR("Can't have a stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else {
		TC_ERROR("Error reading stored dump! (%d)\n", ret);
		ztest_test_fail();
	}
}

void test_invalidate_stored_dump(void)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_error();

	/* There should be a stored coredump now if backend has storage */
	ret = coredump_cmd(COREDUMP_CMD_INVALIDATE_STORED_DUMP, NULL);
	if (ret == -ENOTSUP) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		TC_ERROR("Can't invalidate stored dump: unexpectedly not supported.\n");
		ztest_test_fail();
#else
		TC_PRINT("Can't invalidate stored dump: expectedly not supported.\n");
		ztest_test_skip();
#endif
	} else if (ret == 0) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		check_error();
		ztest_test_pass();
#else
		TC_ERROR("Can't invalidate the stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else {
		TC_ERROR("Error invalidating stored dump! (%d)\n", ret);
		ztest_test_fail();
	}
}

void test_erase_stored_dump(void)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_error();

	/* There should be a stored coredump now if backend has storage */
	ret = coredump_cmd(COREDUMP_CMD_ERASE_STORED_DUMP, NULL);
	if (ret == -ENOTSUP) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		TC_ERROR("Can't erase stored dump: unexpectedly not supported.\n");
		ztest_test_fail();
#else
		TC_PRINT("Can't erase stored dump: expectedly not supported.\n");
		ztest_test_skip();
#endif
	} else if (ret == 0) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		check_error();
		ztest_test_pass();
#else
		TC_ERROR("Can't erase the stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else {
		TC_ERROR("Error erasing stored dump! (%d)\n", ret);
		ztest_test_fail();
	}
}

void test_get_stored_dump_size(int size_expected)
{
	int ret;

	/* Cannot proceed with previous errors */
	check_error();

	ret = coredump_query(COREDUMP_QUERY_GET_STORED_DUMP_SIZE, NULL);
	if (ret == -ENOTSUP) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		TC_ERROR("Can't query stored dump size: unexpectedly not supported.\n");
		ztest_test_fail();
#else
		TC_PRINT("Can't query stored dump size: expectedly not supported.\n");
		ztest_test_skip();
#endif
	} else if (ret >= 0) {
#ifdef CONFIG_TEST_STORED_COREDUMP
		check_error();
		if (size_expected > 0) {
			zassert_equal(ret, size_expected,
				      "Coredump size %d != %d size expected.\n",
				      ret, size_expected);
		}
		ztest_test_pass();
#else
		TC_ERROR("Can't have a stored dump: not supported.\n");
		ztest_test_fail();
#endif
	} else {
		TC_ERROR("Error reading stored dump size! (%d)\n", ret);
		ztest_test_fail();
	}
}

/* Excecute tests in exact sequence with the stored core dump. */

ZTEST(coredump_backends, test_coredump_0_ready) {
	check_error();
	ztest_test_pass();
}

ZTEST(coredump_backends, test_coredump_1_stored) {
	test_has_stored_dump(true);
}

ZTEST(coredump_backends, test_coredump_2_size) {
	test_get_stored_dump_size(CONFIG_TEST_STORED_DUMP_SIZE);
}

ZTEST(coredump_backends, test_coredump_3_verify) {
	test_verify_stored_dump();
}

ZTEST(coredump_backends, test_coredump_4_invalidate) {
	test_invalidate_stored_dump();
}

ZTEST(coredump_backends, test_coredump_5_erase) {
	test_erase_stored_dump();
}

ZTEST_SUITE(coredump_backends, NULL, raise_coredump, NULL, NULL, NULL);

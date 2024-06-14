/*
 * Copyright (c) 2024, Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/ztest.h>

#ifndef CONFIG_PICOLIBC

K_THREAD_STACK_DEFINE(test_stack, 1024);

#define LOCK_SHOULD_PASS (void *)true
#define LOCK_SHOULD_FAIL (void *)false
#define UNLOCK_FILE      (void *)true
#define NO_UNLOCK_FILE   (void *)false

void ftrylockfile_thread(void *p1, void *p2, void *p3)
{
	int ret;
	FILE *file = p1;
	bool success = (bool)p2;
	bool unlock = (bool)p3;

	if (success) {
		ret = ftrylockfile(file);
		zassert_ok(ret, "Expected ftrylockfile to succeed but it failed: %d", ret);
		if (unlock) {
			funlockfile(file);
		}
	} else {
		zassert_not_ok(ftrylockfile(file),
			       "Expected ftrylockfile to failed but it succeeded");
	}
}

void flockfile_thread(void *p1, void *p2, void *p3)
{
	FILE *file = p1;
	bool success = (bool)p2;
	bool unlock = (bool)p3;

	flockfile(file);

	if (!success) {
		/* Shouldn't be here if it supposed to fail */
		ztest_test_fail();
	}

	if (unlock) {
		funlockfile(file);
	}
}

ZTEST(file_locking, test_file_locking)
{
	FILE *file = INT_TO_POINTER(z_alloc_fd(NULL, NULL));
	int priority = k_thread_priority_get(k_current_get());
	struct k_thread test_thread;

	/* Lock 5 times with flockfile */
	for (int i = 0; i < 5; i++) {
		flockfile(file);
	}

	/* Lock 5 times with ftrylockfile */
	for (int i = 0; i < 5; i++) {
		zassert_ok(ftrylockfile(file));
	}

	/* Spawn a thread that uses ftrylockfile(), it should fail immediately */
	k_thread_create(&test_thread, test_stack, K_THREAD_STACK_SIZEOF(test_stack),
			ftrylockfile_thread, file, LOCK_SHOULD_FAIL, NO_UNLOCK_FILE, priority, 0,
			K_NO_WAIT);
	/* The thread should terminate immediately */
	zassert_ok(k_thread_join(&test_thread, K_MSEC(100)));

	/* Try agian with flockfile(), it should block forever */
	k_thread_create(&test_thread, test_stack, K_THREAD_STACK_SIZEOF(test_stack),
			flockfile_thread, file, LOCK_SHOULD_FAIL, NO_UNLOCK_FILE, priority, 0,
			K_NO_WAIT);
	/* We expect the flockfile() call to block forever, so this will timeout */
	zassert_equal(k_thread_join(&test_thread, K_MSEC(500)), -EAGAIN);
	/* Abort the test thread */
	k_thread_abort(&test_thread);

	/* Unlock the file completely in this thread */
	for (int i = 0; i < 10; i++) {
		funlockfile(file);
	}

	/* Spawn the thread again, which should be able to lock the file now with ftrylockfile() */
	k_thread_create(&test_thread, test_stack, K_THREAD_STACK_SIZEOF(test_stack),
			ftrylockfile_thread, file, LOCK_SHOULD_PASS, UNLOCK_FILE, priority, 0,
			K_NO_WAIT);
	zassert_ok(k_thread_join(&test_thread, K_MSEC(100)));

	z_free_fd(POINTER_TO_INT(file));
}

void put_thread(void *p1, void *p2, void *p3)
{
	FILE *file = p1;

	/* Lock held in main thread */
	zassert_not_ok(ftrylockfile(file));

	/* Wait for the lock */
	flockfile(file);
	zassert_equal(putc_unlocked('S', file), 'S');
	zassert_equal(putchar('T'), 'T');
	funlockfile(file);
}

ZTEST(file_locking, test_stdio)
{
	FILE *file = INT_TO_POINTER(z_alloc_fd(NULL, NULL));
	struct k_thread test_thread;
	int priority = k_thread_priority_get(k_current_get());

	/* Lock the file before creating the test thread */
	flockfile(file);

	k_thread_create(&test_thread, test_stack, K_THREAD_STACK_SIZEOF(test_stack),
			put_thread, file, INT_TO_POINTER(true), NULL, priority, 0,
			K_NO_WAIT);

	/* Allow the test thread to run */
	k_msleep(100);
	/* The test thread should be waiting for the lock */
	zassert_equal(k_thread_join(&test_thread, K_MSEC(10)), -EAGAIN);

	/* Main thread has the lock, either version should work */
	zassert_equal(putc('T', file), 'T');
	zassert_equal(putchar_unlocked('E'), 'E');

	/* We are done with the file here, unlock it so that test thread can run */
	funlockfile(file);
	zassert_equal(k_thread_join(&test_thread, K_MSEC(100)), 0);

	z_free_fd(POINTER_TO_INT(file));
}

#else
/**
 * PicoLIBC doesn't support these functions in its header
 * Skip the tests for now.
 */
ZTEST(file_locking, test_file_locking)
{
	ztest_test_skip();
}
#endif /* CONFIG_PICOLIBC */

ZTEST_SUITE(file_locking, NULL, NULL, NULL, NULL, NULL);

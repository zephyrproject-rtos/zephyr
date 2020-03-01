/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>

#define TIMEOUT 500
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

/**TESTPOINT: init via K_MUTEX_DEFINE*/
K_MUTEX_DEFINE(kmutex);
static struct k_mutex mutex;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static void tThread_entry_lock_forever(void *p1, void *p2, void *p3)
{
	zassert_false(k_mutex_lock((struct k_mutex *)p1, K_FOREVER) == 0,
		      "access locked resource from spawn thread");
	/* should not hit here */
}

static void tThread_entry_lock_no_wait(void *p1, void *p2, void *p3)
{
	zassert_true(k_mutex_lock((struct k_mutex *)p1, K_NO_WAIT) != 0, NULL);
	TC_PRINT("bypass locked resource from spawn thread\n");
}

static void tThread_entry_lock_timeout_fail(void *p1, void *p2, void *p3)
{
	zassert_true(k_mutex_lock((struct k_mutex *)p1, TIMEOUT - 100) != 0, NULL);
	TC_PRINT("bypass locked resource from spawn thread\n");
}

static void tThread_entry_lock_timeout_pass(void *p1, void *p2, void *p3)
{
	zassert_true(k_mutex_lock((struct k_mutex *)p1, TIMEOUT + 100) == 0, NULL);
	TC_PRINT("access resource from spawn thread\n");
	k_mutex_unlock((struct k_mutex *)p1);
}

static void tmutex_test_lock(struct k_mutex *pmutex,
			     void (*entry_fn)(void *, void *, void *))
{
	k_mutex_init(pmutex);
	k_thread_create(&tdata, tstack, STACK_SIZE,
			entry_fn, pmutex, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_mutex_lock(pmutex, K_FOREVER);
	TC_PRINT("access resource from main thread\n");

	/* wait for spawn thread to take action */
	k_sleep(TIMEOUT);
}

static void tmutex_test_lock_timeout(struct k_mutex *pmutex,
				     void (*entry_fn)(void *, void *, void *))
{
	/**TESTPOINT: test k_mutex_init mutex*/
	k_mutex_init(pmutex);
	k_thread_create(&tdata, tstack, STACK_SIZE,
			entry_fn, pmutex, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_mutex_lock(pmutex, K_FOREVER);
	TC_PRINT("access resource from main thread\n");

	/* wait for spawn thread to take action */
	k_sleep(TIMEOUT);
	k_mutex_unlock(pmutex);
	k_sleep(TIMEOUT);

}

static void tmutex_test_lock_unlock(struct k_mutex *pmutex)
{
	k_mutex_init(pmutex);
	zassert_true(k_mutex_lock(pmutex, K_FOREVER) == 0,
		     "fail to lock K_FOREVER");
	k_mutex_unlock(pmutex);
	zassert_true(k_mutex_lock(pmutex, K_NO_WAIT) == 0,
		     "fail to lock K_NO_WAIT");
	k_mutex_unlock(pmutex);
	zassert_true(k_mutex_lock(pmutex, TIMEOUT) == 0,
		     "fail to lock TIMEOUT");
	k_mutex_unlock(pmutex);
}

/*test cases*/
void test_mutex_reent_lock_forever(void)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	k_mutex_init(&mutex);
	tmutex_test_lock(&mutex, tThread_entry_lock_forever);
	k_thread_abort(&tdata);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock(&kmutex, tThread_entry_lock_forever);
	k_thread_abort(&tdata);
}

void test_mutex_reent_lock_no_wait(void)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock(&mutex, tThread_entry_lock_no_wait);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock(&kmutex, tThread_entry_lock_no_wait);
}

void test_mutex_reent_lock_timeout_fail(void)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock_timeout(&mutex, tThread_entry_lock_timeout_fail);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock_timeout(&kmutex, tThread_entry_lock_no_wait);
}

void test_mutex_reent_lock_timeout_pass(void)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock_timeout(&mutex, tThread_entry_lock_timeout_pass);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock_timeout(&kmutex, tThread_entry_lock_no_wait);
}

void test_mutex_lock_unlock(void)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock_unlock(&mutex);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock_unlock(&kmutex);
}

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, &tstack, &kmutex,
			      &mutex);

	ztest_test_suite(mutex_api,
			 ztest_1cpu_user_unit_test(test_mutex_lock_unlock),
			 ztest_1cpu_user_unit_test(test_mutex_reent_lock_forever),
			 ztest_user_unit_test(test_mutex_reent_lock_no_wait),
			 ztest_user_unit_test(test_mutex_reent_lock_timeout_fail),
			 ztest_1cpu_user_unit_test(test_mutex_reent_lock_timeout_pass)
			 );
	ztest_run_test_suite(mutex_api);
}

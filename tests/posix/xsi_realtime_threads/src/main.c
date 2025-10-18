/*
 * Copyright (c) 2025 Marvin Ouma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#define BIOS_FOOD       0xB105F00D
#define PRIO_INVALID    -1
#define PTHREAD_INVALID -1

static pthread_attr_t attr;
static const pthread_attr_t uninit_attr;
static bool detached_thread_has_finished;

static bool xsi_realtime_threads_predicate(const void *global_state)
{
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1024);
	return true;
}

static void xsi_realtime_threads_teardown(void *fixture)
{
	pthread_attr_destroy(&attr);
}

static void *thread_entry(void *arg)
{
	bool joinable = (bool)POINTER_TO_UINT(arg);

	if (!joinable) {
		detached_thread_has_finished = true;
	}

	return NULL;
}

static void *inheritsched_entry(void *arg)
{
	int prio;
	int inheritsched;
	int pprio = POINTER_TO_INT(arg);

	zassert_ok(pthread_attr_getinheritsched(&attr, &inheritsched));

	prio = k_thread_priority_get(k_current_get());

	if (inheritsched == PTHREAD_INHERIT_SCHED) {
		/*
		 * There will be numerical overlap between posix priorities in different scheduler
		 * policies so only check the Zephyr priority here. The posix policy and posix
		 * priority are derived from the Zephyr priority in any case.
		 */
		zassert_equal(prio, pprio, "actual priority: %d, expected priority: %d", prio,
			      pprio);
		return NULL;
	}

	/* inheritsched == PTHREAD_EXPLICIT_SCHED */
	int act_prio;
	int exp_prio;
	int act_policy;
	int exp_policy;
	struct sched_param param;

	/* get the actual policy, param, etc */
	zassert_ok(pthread_getschedparam(pthread_self(), &act_policy, &param));
	act_prio = param.sched_priority;

	/* get the expected policy, param, etc */
	zassert_ok(pthread_attr_getschedpolicy(&attr, &exp_policy));
	zassert_ok(pthread_attr_getschedparam(&attr, &param));
	exp_prio = param.sched_priority;

	/* compare actual vs expected */
	zassert_equal(act_policy, exp_policy, "actual policy: %d, expected policy: %d", act_policy,
		      exp_policy);
	zassert_equal(act_prio, exp_prio, "actual priority: %d, expected priority: %d", act_prio,
		      exp_prio);

	return NULL;
}

static void create_thread_common_entry(const pthread_attr_t *attrp, bool expect_success,
				       bool joinable, void *(*entry)(void *arg), void *arg)
{
	pthread_t th;

	if (!joinable) {
		detached_thread_has_finished = false;
	}

	if (expect_success) {
		zassert_ok(pthread_create(&th, attrp, entry, arg));
	} else {
		zassert_not_ok(pthread_create(&th, attrp, entry, arg));
		return;
	}

	if (joinable) {
		zassert_ok(pthread_join(th, NULL), "failed to join joinable thread");
		return;
	}

	/* should not be able to join detached thread */
	zassert_not_ok(pthread_join(th, NULL));

	for (size_t i = 0; i < 10; ++i) {
		k_msleep(2 * CONFIG_PTHREAD_RECYCLER_DELAY_MS);
		if (detached_thread_has_finished) {
			break;
		}
	}

	zassert_true(detached_thread_has_finished, "detached thread did not seem to finish");
}

static void create_thread_common(const pthread_attr_t *attrp, bool expect_success, bool joinable)
{
	create_thread_common_entry(attrp, expect_success, joinable, thread_entry,
				   UINT_TO_POINTER(joinable));
}

static inline void can_create_thread(const pthread_attr_t *attrp)
{
	create_thread_common(attrp, true, true);
}

static void test_pthread_attr_setinheritsched_common(bool inheritsched)
{
	int prio;
	int policy;
	struct sched_param param;

	extern int zephyr_to_posix_priority(int priority, int *policy);

	prio = k_thread_priority_get(k_current_get());
	zassert_not_equal(prio, K_LOWEST_APPLICATION_THREAD_PRIO);

	/*
	 * values affected by inheritsched are policy / priority / contentionscope
	 *
	 * we only support PTHREAD_SCOPE_SYSTEM, so no need to set contentionscope
	 */
	prio = K_LOWEST_APPLICATION_THREAD_PRIO;
	param.sched_priority = zephyr_to_posix_priority(prio, &policy);

	zassert_ok(pthread_attr_setschedpolicy(&attr, policy));
	zassert_ok(pthread_attr_setschedparam(&attr, &param));
	zassert_ok(pthread_attr_setinheritsched(&attr, inheritsched));
	create_thread_common_entry(&attr, true, true, inheritsched_entry,
				   UINT_TO_POINTER(k_thread_priority_get(k_current_get())));
}

ZTEST(xsi_realtime_threads, test_pthread_attr_setinheritsched)
{
	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_setinheritsched(NULL, PTHREAD_EXPLICIT_SCHED),
				      EINVAL);
			zassert_equal(pthread_attr_setinheritsched(NULL, PTHREAD_INHERIT_SCHED),
				      EINVAL);
			zassert_equal(pthread_attr_setinheritsched((pthread_attr_t *)&uninit_attr,
								   PTHREAD_INHERIT_SCHED),
				      EINVAL);
		}
		zassert_equal(pthread_attr_setinheritsched(&attr, 3), EINVAL);
	}

	/* valid cases */
	test_pthread_attr_setinheritsched_common(PTHREAD_INHERIT_SCHED);
	test_pthread_attr_setinheritsched_common(PTHREAD_EXPLICIT_SCHED);
}

ZTEST(xsi_realtime_threads, test_pthread_attr_getinheritsched)
{
	int inheritsched = BIOS_FOOD;

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_getinheritsched(NULL, NULL), EINVAL);
			zassert_equal(pthread_attr_getinheritsched(NULL, &inheritsched), EINVAL);
			zassert_equal(pthread_attr_getinheritsched(&uninit_attr, &inheritsched),
				      EINVAL);
		}
		zassert_equal(pthread_attr_getinheritsched(&attr, NULL), EINVAL);
	}

	zassert_ok(pthread_attr_getinheritsched(&attr, &inheritsched));
	zassert_equal(inheritsched, PTHREAD_INHERIT_SCHED);
}

ZTEST(xsi_realtime_threads, test_pthread_attr_getschedparam)
{
	struct sched_param param = {
		.sched_priority = BIOS_FOOD,
	};

	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_getschedparam(NULL, NULL), EINVAL);
			zassert_equal(pthread_attr_getschedparam(NULL, &param), EINVAL);
			zassert_equal(pthread_attr_getschedparam(&uninit_attr, &param), EINVAL);
		}
		zassert_equal(pthread_attr_getschedparam(&attr, NULL), EINVAL);
	}

	/* only check to see that the function succeeds and sets param */
	zassert_ok(pthread_attr_getschedparam(&attr, &param));
	zassert_not_equal(BIOS_FOOD, param.sched_priority);
}

ZTEST(xsi_realtime_threads, test_pthread_attr_setschedparam)
{
	struct sched_param param = {0};
	/* degenerate cases */
	{
		if (false) {
			/* undefined behaviour */
			zassert_equal(pthread_attr_setschedparam(NULL, NULL), EINVAL);
			zassert_equal(pthread_attr_setschedparam(NULL, &param), EINVAL);
			zassert_equal(
				pthread_attr_setschedparam((pthread_attr_t *)&uninit_attr, &param),
				EINVAL);
		}
		zassert_equal(pthread_attr_setschedparam(&attr, NULL), EINVAL);
	}

	zassert_ok(pthread_attr_setschedparam(&attr, &param));

	can_create_thread(&attr);
}

static void *test_pthread_setschedprio_fn(void *arg)
{
	int policy;
	int prio = 0;
	struct sched_param param;
	pthread_t self = pthread_self();

	zassert_equal(pthread_setschedprio(self, PRIO_INVALID), EINVAL, "EINVAL was expected");
	zassert_equal(pthread_setschedprio(PTHREAD_INVALID, prio), ESRCH, "ESRCH was expected");

	zassert_ok(pthread_setschedprio(self, prio));
	param.sched_priority = ~prio;
	zassert_ok(pthread_getschedparam(self, &policy, &param));
	zassert_equal(param.sched_priority, prio, "Priority unchanged");

	return NULL;
}

ZTEST(xsi_realtime_threads, test_pthread_setschedprio)
{
	pthread_t th;

	zassert_ok(pthread_create(&th, NULL, test_pthread_setschedprio_fn, NULL));
	zassert_ok(pthread_join(th, NULL));
}

ZTEST_SUITE(xsi_realtime_threads, xsi_realtime_threads_predicate, NULL, NULL, NULL,
	    xsi_realtime_threads_teardown);

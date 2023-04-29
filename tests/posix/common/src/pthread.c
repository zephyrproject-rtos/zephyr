/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <pthread.h>
#include <semaphore.h>
#include <zephyr/sys/util.h>

#ifndef min
#define min(a, b) ((a) < (b)) ? (a) : (b)
#endif

#define N_THR_E 3
#define N_THR_T 4
#define BOUNCES 64
#define STACKS (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_PRIORITY 3
#define ONE_SECOND 1

/* Macros to test invalid states */
#define PTHREAD_CANCEL_INVALID -1
#define SCHED_INVALID -1
#define PRIO_INVALID -1
#define PTHREAD_INVALID -1

K_THREAD_STACK_ARRAY_DEFINE(stack_e, N_THR_E, STACKS);
K_THREAD_STACK_ARRAY_DEFINE(stack_t, N_THR_T, STACKS);
K_THREAD_STACK_ARRAY_DEFINE(stack_1, 1, 32);

void *thread_top_exec(void *p1);
void *thread_top_term(void *p1);

PTHREAD_MUTEX_DEFINE(lock);

PTHREAD_COND_DEFINE(cvar0);

PTHREAD_COND_DEFINE(cvar1);

PTHREAD_BARRIER_DEFINE(barrier, N_THR_E);

sem_t main_sem;

static int bounce_failed;
static int bounce_done[N_THR_E];

static int curr_bounce_thread;

static int barrier_failed;
static int barrier_done[N_THR_E];
static int barrier_return[N_THR_E];

/* First phase bounces execution between two threads using a condition
 * variable, continuously testing that no other thread is mucking with
 * the protected state.  This ends with all threads going back to
 * sleep on the condition variable and being woken by main() for the
 * second phase.
 *
 * Second phase simply lines up all the threads on a barrier, verifies
 * that none run until the last one enters, and that all run after the
 * exit.
 *
 * Test success is signaled to main() using a traditional semaphore.
 */

void *thread_top_exec(void *p1)
{
	int i, j, id = (int) POINTER_TO_INT(p1);
	int policy;
	struct sched_param schedparam;

	pthread_getschedparam(pthread_self(), &policy, &schedparam);
	printk("Thread %d starting with scheduling policy %d & priority %d\n",
		 id, policy, schedparam.sched_priority);
	/* Try a double-lock here to exercise the failing case of
	 * trylock.  We don't support RECURSIVE locks, so this is
	 * guaranteed to fail.
	 */
	pthread_mutex_lock(&lock);

	if (!pthread_mutex_trylock(&lock)) {
		printk("pthread_mutex_trylock inexplicably succeeded\n");
		bounce_failed = 1;
	}

	pthread_mutex_unlock(&lock);

	for (i = 0; i < BOUNCES; i++) {

		pthread_mutex_lock(&lock);

		/* Wait for the current owner to signal us, unless we
		 * are the very first thread, in which case we need to
		 * wait a bit to be sure the other threads get
		 * scheduled and wait on cvar0.
		 */
		if (!(id == 0 && i == 0)) {
			zassert_equal(0, pthread_cond_wait(&cvar0, &lock), "");
		} else {
			pthread_mutex_unlock(&lock);
			usleep(USEC_PER_MSEC * 500U);
			pthread_mutex_lock(&lock);
		}

		/* Claim ownership, then try really hard to give someone
		 * else a shot at hitting this if they are racing.
		 */
		curr_bounce_thread = id;
		for (j = 0; j < 1000; j++) {
			if (curr_bounce_thread != id) {
				printk("Racing bounce threads\n");
				bounce_failed = 1;
				sem_post(&main_sem);
				pthread_mutex_unlock(&lock);
				return NULL;
			}
			sched_yield();
		}

		/* Next one's turn, go back to the top and wait.  */
		pthread_cond_signal(&cvar0);
		pthread_mutex_unlock(&lock);
	}

	/* Signal we are complete to main(), then let it wake us up.  Note
	 * that we are using the same mutex with both cvar0 and cvar1,
	 * which is non-standard but kosher per POSIX (and it works fine
	 * in our implementation
	 */
	pthread_mutex_lock(&lock);
	bounce_done[id] = 1;
	sem_post(&main_sem);
	pthread_cond_wait(&cvar1, &lock);
	pthread_mutex_unlock(&lock);

	/* Now just wait on the barrier.  Make sure no one else finished
	 * before we wait on it, then signal that we're done
	 */
	for (i = 0; i < N_THR_E; i++) {
		if (barrier_done[i]) {
			printk("Barrier exited early\n");
			barrier_failed = 1;
			sem_post(&main_sem);
		}
	}
	barrier_return[id] = pthread_barrier_wait(&barrier);
	barrier_done[id] = 1;
	sem_post(&main_sem);
	pthread_exit(p1);

	return NULL;
}

int bounce_test_done(void)
{
	int i;

	if (bounce_failed) {
		return 1;
	}

	for (i = 0; i < N_THR_E; i++) {
		if (!bounce_done[i]) {
			return 0;
		}
	}

	return 1;
}

int barrier_test_done(void)
{
	int i;

	if (barrier_failed) {
		return 1;
	}

	for (i = 0; i < N_THR_E; i++) {
		if (!barrier_done[i]) {
			return 0;
		}
	}

	return 1;
}

void *thread_top_term(void *p1)
{
	pthread_t self;
	int oldstate, policy, ret;
	int id = POINTER_TO_INT(p1);
	struct sched_param param, getschedparam;

	param.sched_priority = N_THR_T - id;

	self = pthread_self();

	/* Change priority of thread */
	zassert_false(pthread_setschedparam(self, SCHED_RR, &param),
		      "Unable to set thread priority!");

	zassert_false(pthread_getschedparam(self, &policy, &getschedparam),
			"Unable to get thread priority!");

	printk("Thread %d starting with a priority of %d\n",
			id,
			getschedparam.sched_priority);

	if (id % 2) {
		ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
		zassert_false(ret, "Unable to set cancel state!");
	}

	if (id >= 2) {
		ret = pthread_detach(self);
		if (id == 2) {
			zassert_equal(ret, EINVAL, "re-detached thread!");
		}
	}

	printk("Cancelling thread %d\n", id);
	pthread_cancel(self);
	printk("Thread %d could not be cancelled\n", id);
	sleep(ONE_SECOND);
	pthread_exit(p1);
	return NULL;
}

ZTEST(posix_apis, test_posix_pthread_execution)
{
	int i, ret, min_prio, max_prio;
	int dstate, policy;
	pthread_attr_t attr[N_THR_E] = {};
	struct sched_param schedparam, getschedparam;
	pthread_t newthread[N_THR_E];
	int schedpolicy = SCHED_FIFO;
	void *retval, *stackaddr;
	size_t stacksize;
	int serial_threads = 0;
	static const char thr_name[] = "thread name";
	char thr_name_buf[CONFIG_THREAD_MAX_NAME_LEN];

	sem_init(&main_sem, 0, 1);
	schedparam.sched_priority = CONFIG_NUM_COOP_PRIORITIES - 1;
	min_prio = sched_get_priority_min(schedpolicy);
	max_prio = sched_get_priority_max(schedpolicy);

	ret = (min_prio < 0 || max_prio < 0 ||
			schedparam.sched_priority < min_prio ||
			schedparam.sched_priority > max_prio);

	/* TESTPOINT: Check if scheduling priority is valid */
	zassert_false(ret,
			"Scheduling priority outside valid priority range");

	/* TESTPOINTS: Try setting attributes before init */
	ret = pthread_attr_setschedparam(&attr[0], &schedparam);
	zassert_equal(ret, EINVAL, "uninitialized attr set!");

	ret = pthread_attr_setdetachstate(&attr[0], PTHREAD_CREATE_JOINABLE);
	zassert_equal(ret, EINVAL, "uninitialized attr set!");

	ret = pthread_attr_setschedpolicy(&attr[0], schedpolicy);
	zassert_equal(ret, EINVAL, "uninitialized attr set!");

	/* TESTPOINT: Try setting attribute with empty stack */
	ret = pthread_attr_setstack(&attr[0], 0, STACKS);
	zassert_equal(ret, EACCES, "empty stack set!");

	/* TESTPOINTS: Try getting attributes before init */
	ret = pthread_attr_getschedparam(&attr[0], &getschedparam);
	zassert_equal(ret, EINVAL, "uninitialized attr retrieved!");

	ret = pthread_attr_getdetachstate(&attr[0], &dstate);
	zassert_equal(ret, EINVAL, "uninitialized attr retrieved!");

	ret = pthread_attr_getschedpolicy(&attr[0], &policy);
	zassert_equal(ret, EINVAL, "uninitialized attr retrieved!");

	ret = pthread_attr_getstack(&attr[0], &stackaddr, &stacksize);
	zassert_equal(ret, EINVAL, "uninitialized attr retrieved!");

	ret = pthread_attr_getstacksize(&attr[0], &stacksize);
	zassert_equal(ret, EINVAL, "uninitialized attr retrieved!");

	/* TESTPOINT: Try destroying attr before init */
	ret = pthread_attr_destroy(&attr[0]);
	zassert_equal(ret, EINVAL, "uninitialized attr destroyed!");

	/* TESTPOINT: Try getting name of NULL thread (aka uninitialized
	 * thread var).
	 */
	ret = pthread_getname_np(PTHREAD_INVALID, thr_name_buf, sizeof(thr_name_buf));
	zassert_equal(ret, ESRCH, "uninitialized getname!");

	/* TESTPOINT: Try setting name of NULL thread (aka uninitialized
	 * thread var).
	 */
	ret = pthread_setname_np(PTHREAD_INVALID, thr_name);
	zassert_equal(ret, ESRCH, "uninitialized setname!");

	/* TESTPOINT: Try creating thread before attr init */
	ret = pthread_create(&newthread[0], &attr[0],
				thread_top_exec, NULL);
	zassert_equal(ret, EINVAL, "thread created before attr init!");

	for (i = 0; i < N_THR_E; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			zassert_false(pthread_attr_destroy(&attr[i]),
				      "Unable to destroy pthread object attrib");
			zassert_false(pthread_attr_init(&attr[i]),
				      "Unable to create pthread object attrib");
		}

		/* TESTPOINTS: Retrieve set stack attributes and compare */
		pthread_attr_setstack(&attr[i], &stack_e[i][0], STACKS);
		stackaddr = NULL;
		pthread_attr_getstack(&attr[i], &stackaddr, &stacksize);
		zassert_equal_ptr(&stack_e[i][0], stackaddr,
				  "stack attribute addresses do not match!");
		zassert_equal(STACKS, stacksize, "stack sizes do not match!");

		pthread_attr_getstacksize(&attr[i], &stacksize);
		zassert_equal(STACKS, stacksize, "stack sizes do not match!");

		pthread_attr_setschedpolicy(&attr[i], schedpolicy);
		pthread_attr_getschedpolicy(&attr[i], &policy);
		zassert_equal(schedpolicy, policy,
				"scheduling policies do not match!");

		pthread_attr_setschedparam(&attr[i], &schedparam);
		pthread_attr_getschedparam(&attr[i], &getschedparam);
		zassert_equal(schedparam.sched_priority,
			      getschedparam.sched_priority,
			      "scheduling priorities do not match!");

		ret = pthread_create(&newthread[i], &attr[i], thread_top_exec,
				INT_TO_POINTER(i));

		/* TESTPOINT: Check if thread is created successfully */
		zassert_false(ret, "Number of threads exceed max limit");
	}

	/* TESTPOINT: Try getting thread name with no buffer */
	ret = pthread_getname_np(newthread[0], NULL, sizeof(thr_name_buf));
	zassert_equal(ret, EINVAL, "uninitialized getname!");

	/* TESTPOINT: Try setting thread name with no buffer */
	ret = pthread_setname_np(newthread[0], NULL);
	zassert_equal(ret, EINVAL, "uninitialized setname!");

	/* TESTPOINT: Try setting thread name */
	ret = pthread_setname_np(newthread[0], thr_name);
	zassert_false(ret, "Set thread name failed!");

	/* TESTPOINT: Try getting thread name */
	ret = pthread_getname_np(newthread[0], thr_name_buf,
				 sizeof(thr_name_buf));
	zassert_false(ret, "Get thread name failed!");

	/* TESTPOINT: Thread names match */
	ret = strncmp(thr_name, thr_name_buf, min(strlen(thr_name),
						  strlen(thr_name_buf)));
	zassert_false(ret, "Thread names don't match!");

	while (!bounce_test_done()) {
		sem_wait(&main_sem);
	}

	/* TESTPOINT: Check if bounce test passes */
	zassert_false(bounce_failed, "Bounce test failed");

	printk("Bounce test OK\n");

	/* Wake up the worker threads */
	pthread_mutex_lock(&lock);
	pthread_cond_broadcast(&cvar1);
	pthread_mutex_unlock(&lock);

	while (!barrier_test_done()) {
		sem_wait(&main_sem);
	}

	/* TESTPOINT: Check if barrier test passes */
	zassert_false(barrier_failed, "Barrier test failed");

	for (i = 0; i < N_THR_E; i++) {
		pthread_join(newthread[i], &retval);
	}

	for (i = 0; i < N_THR_E; i++) {
		if (barrier_return[i] == PTHREAD_BARRIER_SERIAL_THREAD) {
			++serial_threads;
		}
	}

	/* TESTPOINT: Check only one PTHREAD_BARRIER_SERIAL_THREAD returned. */
	zassert_true(serial_threads == 1, "Bungled barrier return value(s)");

	printk("Barrier test OK\n");
}

ZTEST(posix_apis, test_posix_pthread_error_condition)
{
	pthread_attr_t attr;
	struct sched_param param;
	void *stackaddr;
	size_t stacksize;
	int policy, detach;
	static pthread_once_t key;

	/* TESTPOINT: invoke pthread APIs with NULL */
	zassert_equal(pthread_attr_destroy(NULL), EINVAL,
		      "pthread destroy NULL error");
	zassert_equal(pthread_attr_getschedparam(NULL, &param), EINVAL,
		      "get scheduling param error");
	zassert_equal(pthread_attr_getstack(NULL, &stackaddr, &stacksize),
		      EINVAL, "get stack attributes error");
	zassert_equal(pthread_attr_getstacksize(NULL, &stacksize),
		      EINVAL, "get stack size error");
	zassert_equal(pthread_attr_setschedpolicy(NULL, 2),
		      EINVAL, "set scheduling policy error");
	zassert_equal(pthread_attr_getschedpolicy(NULL, &policy),
		      EINVAL, "get scheduling policy error");
	zassert_equal(pthread_attr_setdetachstate(NULL, 0),
		      EINVAL, "pthread set detach state with NULL error");
	zassert_equal(pthread_attr_getdetachstate(NULL, &detach),
		      EINVAL, "get detach state error");
	zassert_equal(pthread_detach(PTHREAD_INVALID), ESRCH, "detach with NULL error");
	zassert_equal(pthread_attr_init(NULL), ENOMEM,
		      "init with NULL error");
	zassert_equal(pthread_attr_setschedparam(NULL, &param), EINVAL,
		      "set sched param with NULL error");
	zassert_equal(pthread_cancel(PTHREAD_INVALID), ESRCH,
		      "cancel NULL error");
	zassert_equal(pthread_join(PTHREAD_INVALID, NULL), ESRCH,
		      "join with NULL has error");
	zassert_false(pthread_once(&key, NULL),
		      "pthread dynamic package initialization error");
	zassert_equal(pthread_getschedparam(PTHREAD_INVALID, &policy, &param), ESRCH,
		      "get schedparam with NULL error");
	zassert_equal(pthread_setschedparam(PTHREAD_INVALID, policy, &param), ESRCH,
		      "set schedparam with NULL error");

	attr = (pthread_attr_t){0};
	zassert_equal(pthread_attr_getdetachstate(&attr, &detach),
		      EINVAL, "get detach state error");

	/* Initialise thread attribute to ensure won't be return with init error */
	zassert_false(pthread_attr_init(&attr),
		      "Unable to create pthread object attr");
	zassert_false(pthread_attr_setschedpolicy(&attr, SCHED_FIFO),
		      "set scheduling policy error");
	zassert_false(pthread_attr_setschedpolicy(&attr, SCHED_RR), "set scheduling policy error");
	zassert_false(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE),
		      "set detach state error");
	zassert_false(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED),
		      "set detach state error");
	zassert_equal(pthread_attr_setdetachstate(&attr, 3),
		      EINVAL, "set detach state error");
	zassert_false(pthread_attr_getdetachstate(&attr, &detach),
		      "get detach state error");
}

ZTEST(posix_apis, test_posix_pthread_termination)
{
	int32_t i, ret;
	int oldstate, policy;
	pthread_attr_t attr[N_THR_T];
	struct sched_param schedparam;
	pthread_t newthread[N_THR_T];
	void *retval;

	/* Creating 4 threads with lowest application priority */
	for (i = 0; i < N_THR_T; i++) {
		ret = pthread_attr_init(&attr[i]);
		if (ret != 0) {
			zassert_false(pthread_attr_destroy(&attr[i]),
				      "Unable to destroy pthread object attrib");
			zassert_false(pthread_attr_init(&attr[i]),
				      "Unable to create pthread object attrib");
		}

		if (i == 2) {
			pthread_attr_setdetachstate(&attr[i],
						    PTHREAD_CREATE_DETACHED);
		}

		schedparam.sched_priority = 2;
		pthread_attr_setschedparam(&attr[i], &schedparam);
		pthread_attr_setstack(&attr[i], &stack_t[i][0], STACKS);
		ret = pthread_create(&newthread[i], &attr[i], thread_top_term,
				     INT_TO_POINTER(i));

		zassert_false(ret, "Not enough space to create new thread");
	}

	/* TESTPOINT: Try setting invalid cancel state to current thread */
	ret = pthread_setcancelstate(PTHREAD_CANCEL_INVALID, &oldstate);
	zassert_equal(ret, EINVAL, "invalid cancel state set!");

	/* TESTPOINT: Try setting invalid policy */
	ret = pthread_setschedparam(newthread[0], SCHED_INVALID, &schedparam);
	zassert_equal(ret, EINVAL, "invalid policy set!");

	/* TESTPOINT: Try setting invalid priority */
	schedparam.sched_priority = PRIO_INVALID;
	ret = pthread_setschedparam(newthread[0], SCHED_RR, &schedparam);
	zassert_equal(ret, EINVAL, "invalid priority set!");

	for (i = 0; i < N_THR_T; i++) {
		pthread_join(newthread[i], &retval);
	}

	/* TESTPOINT: Test for deadlock */
	ret = pthread_join(pthread_self(), &retval);
	zassert_equal(ret, EDEADLK, "thread joined with self inexplicably!");

	/* TESTPOINT: Try canceling a terminated thread */
	ret = pthread_cancel(newthread[N_THR_T/2]);
	zassert_equal(ret, ESRCH, "cancelled a terminated thread!");

	/* TESTPOINT: Try getting scheduling info from terminated thread */
	ret = pthread_getschedparam(newthread[N_THR_T/2], &policy, &schedparam);
	zassert_equal(ret, ESRCH, "got attr from terminated thread!");
}

ZTEST(posix_apis, test_posix_thread_attr_stacksize)
{
	size_t act_size;
	pthread_attr_t attr;
	const size_t exp_size = 0xB105F00D;

	/* TESTPOINT: specify a custom stack size via pthread_attr_t */
	zassert_equal(0, pthread_attr_init(&attr), "pthread_attr_init() failed");

	if (PTHREAD_STACK_MIN > 0) {
		zassert_equal(EINVAL, pthread_attr_setstacksize(&attr, 0),
			      "pthread_attr_setstacksize() did not fail");
	}

	zassert_equal(0, pthread_attr_setstacksize(&attr, exp_size),
		      "pthread_attr_setstacksize() failed");
	zassert_equal(0, pthread_attr_getstacksize(&attr, &act_size),
		      "pthread_attr_getstacksize() failed");
	zassert_equal(exp_size, act_size, "wrong size: act: %zu exp: %zu",
		exp_size, act_size);
}

static void *create_thread1(void *p1)
{
	/* do nothing */
	return NULL;
}

ZTEST(posix_apis, test_posix_pthread_create_negative)
{
	int ret;
	pthread_t pthread1;
	pthread_attr_t attr1;

	/* create pthread without attr initialized */
	ret = pthread_create(&pthread1, NULL, create_thread1, (void *)1);
	zassert_equal(ret, EINVAL, "create thread with NULL successful");

	/* initialized attr without set stack to create thread */
	ret = pthread_attr_init(&attr1);
	zassert_false(ret, "attr1 initialized failed");

	attr1 = (pthread_attr_t){0};
	ret = pthread_create(&pthread1, &attr1, create_thread1, (void *)1);
	zassert_equal(ret, EINVAL, "create successful with NULL attr");

	/* set stack size 0 to create thread */
	pthread_attr_setstack(&attr1, &stack_1, 0);
	ret = pthread_create(&pthread1, &attr1, create_thread1, (void *)1);
	zassert_equal(ret, EINVAL, "create thread with 0 size");
}

ZTEST(posix_apis, test_pthread_descriptor_leak)
{
	void *unused;
	pthread_t pthread1;
	pthread_attr_t attr;

	/* If we are leaking descriptors, then this loop will never complete */
	for (size_t i = 0; i < CONFIG_MAX_PTHREAD_COUNT * 2; ++i) {
		zassert_ok(pthread_attr_init(&attr));
		zassert_ok(pthread_attr_setstack(&attr, &stack_e[0][0], STACKS));
		zassert_ok(pthread_create(&pthread1, &attr, create_thread1, NULL),
			   "unable to create thread %zu", i);
		zassert_ok(pthread_join(pthread1, &unused), "unable to join thread %zu", i);
	}
}

ZTEST(posix_apis, test_sched_policy)
{
	/*
	 * TODO:
	 * 1. assert that _POSIX_PRIORITY_SCHEDULING is defined
	 * 2. if _POSIX_SPORADIC_SERVER or _POSIX_THREAD_SPORADIC_SERVER are defined,
	 *    also check SCHED_SPORADIC
	 * 3. SCHED_OTHER is mandatory (but may be equivalent to SCHED_FIFO or SCHED_RR,
	 *    and is implementation defined)
	 */

	int pmin;
	int pmax;
	pthread_t th;
	pthread_attr_t attr;
	struct sched_param param;
	static const int policies[] = {
		SCHED_FIFO,
		SCHED_RR,
		SCHED_OTHER,
		SCHED_INVALID,
	};
	static const char *const policy_names[] = {
		"SCHED_FIFO",
		"SCHED_RR",
		"SCHED_OTHER",
		"SCHED_INVALID",
	};
	static const bool policy_enabled[] = {
		IS_ENABLED(CONFIG_COOP_ENABLED),
		IS_ENABLED(CONFIG_PREEMPT_ENABLED),
		IS_ENABLED(CONFIG_PREEMPT_ENABLED),
		false,
	};
	static int nprio[] = {
		CONFIG_NUM_COOP_PRIORITIES,
		CONFIG_NUM_PREEMPT_PRIORITIES,
		CONFIG_NUM_PREEMPT_PRIORITIES,
		42,
	};
	const char *const prios[] = {"pmin", "pmax"};

	BUILD_ASSERT(!(SCHED_INVALID == SCHED_FIFO || SCHED_INVALID == SCHED_RR ||
		       SCHED_INVALID == SCHED_OTHER),
		     "SCHED_INVALID is itself invalid");

	for (int policy = 0; policy < ARRAY_SIZE(policies); ++policy) {
		if (!policy_enabled[policy]) {
			/* test degenerate cases */
			errno = 0;
			zassert_equal(-1, sched_get_priority_min(policies[policy]),
				      "expected sched_get_priority_min(%s) to fail",
				      policy_names[policy]);
			zassert_equal(EINVAL, errno, "sched_get_priority_min(%s) did not set errno",
				      policy_names[policy]);

			errno = 0;
			zassert_equal(-1, sched_get_priority_max(policies[policy]),
				      "expected sched_get_priority_max(%s) to fail",
				      policy_names[policy]);
			zassert_equal(EINVAL, errno, "sched_get_priority_max(%s) did not set errno",
				      policy_names[policy]);
			continue;
		}

		/* get pmin and pmax for policies[policy] */
		for (int i = 0; i < 2; ++i) {
			errno = 0;
			if (i == 0) {
				pmin = sched_get_priority_min(policies[policy]);
				param.sched_priority = pmin;
			} else {
				pmax = sched_get_priority_max(policies[policy]);
				param.sched_priority = pmax;
			}

			zassert_not_equal(-1, param.sched_priority,
					  "sched_get_priority_%s(%s) failed: %d",
					  i == 0 ? "min" : "max", policy_names[policy], errno);
			zassert_ok(errno, "sched_get_priority_%s(%s) set errno to %s",
				   i == 0 ? "min" : "max", policy_names[policy], errno);
		}

		/*
		 * IEEE 1003.1-2008 Section 2.8.4
		 * conforming implementations should provide a range of at least 32 priorities
		 *
		 * Note: we relax this requirement
		 */
		zassert_true(pmax > pmin, "pmax (%d) <= pmin (%d)", pmax, pmin,
			     "%s min/max inconsistency: pmin: %d pmax: %d", policy_names[policy],
			     pmin, pmax);

		/*
		 * Getting into the weeds a bit (i.e. whitebox testing), Zephyr
		 * cooperative threads use [-CONFIG_NUM_COOP_PRIORITIES,-1] and
		 * preemptive threads use [0, CONFIG_NUM_PREEMPT_PRIORITIES - 1],
		 * where the more negative thread has the higher priority. Since we
		 * cannot map those directly (a return value of -1 indicates error),
		 * we simply map those to the positive space.
		 */
		zassert_equal(pmin, 0, "unexpected pmin for %s", policy_names[policy]);
		zassert_equal(pmax, nprio[policy] - 1, "unexpected pmax for %s",
			      policy_names[policy]); /* test happy paths */

		for (int i = 0; i < 2; ++i) {
			/* create threads with min and max priority levels */
			zassert_ok(pthread_attr_init(&attr),
				   "pthread_attr_init() failed for %s (%d) of %s", prios[i],
				   param.sched_priority, policy_names[policy]);

			zassert_ok(pthread_attr_setschedpolicy(&attr, policies[policy]),
				   "pthread_attr_setschedpolicy() failed for %s (%d) of %s",
				   prios[i], param.sched_priority, policy_names[policy]);

			zassert_ok(pthread_attr_setschedparam(&attr, &param),
				   "pthread_attr_setschedparam() failed for %s (%d) of %s",
				   prios[i], param.sched_priority, policy_names[policy]);

			zassert_ok(pthread_attr_setstack(&attr, &stack_e[0][0], STACKS),
				   "pthread_attr_setstack() failed for %s (%d) of %s", prios[i],
				   param.sched_priority, policy_names[policy]);

			zassert_ok(pthread_create(&th, &attr, create_thread1, NULL),
				   "pthread_create() failed for %s (%d) of %s", prios[i],
				   param.sched_priority, policy_names[policy]);

			zassert_ok(pthread_join(th, NULL),
				   "pthread_join() failed for %s (%d) of %s", prios[i],
				   param.sched_priority, policy_names[policy]);
		}
	}
}

/*
 * Copyright (c) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <semaphore.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DETACH_THR_ID 2

#define N_THR_E 3
#define N_THR_T 4
#define BOUNCES 64
#define ONE_SECOND 1

/* Macros to test invalid states */
#define PTHREAD_CANCEL_INVALID -1
#define SCHED_INVALID -1
#define PRIO_INVALID -1
#define PTHREAD_INVALID -1

static void *thread_top_exec(void *p1);
static void *thread_top_term(void *p1);

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cvar0 = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cvar1 = PTHREAD_COND_INITIALIZER;
static pthread_barrier_t barrier;

static sem_t main_sem;

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

static void *thread_top_exec(void *p1)
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

static int bounce_test_done(void)
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

static int barrier_test_done(void)
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

static void *thread_top_term(void *p1)
{
	pthread_t self;
	int policy, ret;
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
		ret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		zassert_false(ret, "Unable to set cancel state!");
	}

	if (id >= DETACH_THR_ID) {
		zassert_ok(pthread_detach(self), "failed to set detach state");
		zassert_equal(pthread_detach(self), EINVAL, "re-detached thread!");
	}

	printk("Cancelling thread %d\n", id);
	pthread_cancel(self);
	printk("Thread %d could not be cancelled\n", id);
	sleep(ONE_SECOND);
	pthread_exit(p1);
	return NULL;
}

/* Test the internal priority conversion functions */
int zephyr_to_posix_priority(int z_prio, int *policy);
int posix_to_zephyr_priority(int priority, int policy);
ZTEST(pthread, test_pthread_priority_conversion)
{
	/*
	 *    ZEPHYR [-CONFIG_NUM_COOP_PRIORITIES, -1]
	 *                       TO
	 * POSIX(FIFO) [0, CONFIG_NUM_COOP_PRIORITIES - 1]
	 */
	for (int z_prio = -CONFIG_NUM_COOP_PRIORITIES, prio = CONFIG_NUM_COOP_PRIORITIES - 1,
		 p_prio, policy;
	     z_prio <= -1; z_prio++, prio--) {
		p_prio = zephyr_to_posix_priority(z_prio, &policy);
		zassert_equal(policy, SCHED_FIFO);
		zassert_equal(p_prio, prio, "%d %d\n", p_prio, prio);
		zassert_equal(z_prio, posix_to_zephyr_priority(p_prio, SCHED_FIFO));
	}

	/*
	 *  ZEPHYR [0, CONFIG_NUM_PREEMPT_PRIORITIES - 1]
	 *                      TO
	 * POSIX(RR) [0, CONFIG_NUM_PREEMPT_PRIORITIES - 1]
	 */
	for (int z_prio = 0, prio = CONFIG_NUM_PREEMPT_PRIORITIES - 1, p_prio, policy;
	     z_prio < CONFIG_NUM_PREEMPT_PRIORITIES; z_prio++, prio--) {
		p_prio = zephyr_to_posix_priority(z_prio, &policy);
		zassert_equal(policy, SCHED_RR);
		zassert_equal(p_prio, prio, "%d %d\n", p_prio, prio);
		zassert_equal(z_prio, posix_to_zephyr_priority(p_prio, SCHED_RR));
	}
}

ZTEST(pthread, test_pthread_execution)
{
	int i, ret;
	pthread_t newthread[N_THR_E];
	void *retval;
	int serial_threads = 0;
	static const char thr_name[] = "thread name";
	char thr_name_buf[CONFIG_THREAD_MAX_NAME_LEN];

	/*
	 * initialize barriers the standard way after deprecating
	 * PTHREAD_BARRIER_DEFINE().
	 */
	zassert_ok(pthread_barrier_init(&barrier, NULL, N_THR_E));

	sem_init(&main_sem, 0, 1);

	/* TESTPOINT: Try getting name of NULL thread (aka uninitialized
	 * thread var).
	 */
	ret = pthread_getname_np(PTHREAD_INVALID, thr_name_buf, sizeof(thr_name_buf));
	zassert_equal(ret, ESRCH, "uninitialized getname!");

	for (i = 0; i < N_THR_E; i++) {
		ret = pthread_create(&newthread[i], NULL, thread_top_exec, INT_TO_POINTER(i));
	}

	/* TESTPOINT: Try setting name of NULL thread (aka uninitialized
	 * thread var).
	 */
	ret = pthread_setname_np(PTHREAD_INVALID, thr_name);
	zassert_equal(ret, ESRCH, "uninitialized setname!");

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
	ret = strncmp(thr_name, thr_name_buf, MIN(strlen(thr_name), strlen(thr_name_buf)));
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

ZTEST(pthread, test_pthread_termination)
{
	int32_t i, ret;
	pthread_t newthread[N_THR_T] = {0};
	void *retval;

	/* Creating 4 threads */
	for (i = 0; i < N_THR_T; i++) {
		zassert_ok(pthread_create(&newthread[i], NULL, thread_top_term, INT_TO_POINTER(i)));
	}

	/* TESTPOINT: Try setting invalid cancel state to current thread */
	ret = pthread_setcancelstate(PTHREAD_CANCEL_INVALID, NULL);
	zassert_equal(ret, EINVAL, "invalid cancel state set!");

	for (i = 0; i < N_THR_T; i++) {
		if (i < DETACH_THR_ID) {
			zassert_ok(pthread_join(newthread[i], &retval));
		}
	}

	/* TESTPOINT: Test for deadlock */
	ret = pthread_join(pthread_self(), &retval);
	zassert_equal(ret, EDEADLK, "thread joined with self inexplicably!");

	/* TESTPOINT: Try canceling a terminated thread */
	ret = pthread_cancel(newthread[0]);
	zassert_equal(ret, ESRCH, "cancelled a terminated thread!");
}

static void *create_thread1(void *p1)
{
	/* do nothing */
	return NULL;
}

ZTEST(pthread, test_pthread_descriptor_leak)
{
	pthread_t pthread1;

	/* If we are leaking descriptors, then this loop will never complete */
	for (size_t i = 0; i < CONFIG_MAX_PTHREAD_COUNT * 2; ++i) {
		zassert_ok(pthread_create(&pthread1, NULL, create_thread1, NULL),
			   "unable to create thread %zu", i);
		zassert_ok(pthread_join(pthread1, NULL), "unable to join thread %zu", i);
	}
}

ZTEST(pthread, test_sched_getparam)
{
	struct sched_param param;
	int rc = sched_getparam(0, &param);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(pthread, test_sched_getscheduler)
{
	int rc = sched_getscheduler(0);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}
ZTEST(pthread, test_sched_setparam)
{
	struct sched_param param = {
		.sched_priority = 2,
	};
	int rc = sched_setparam(0, &param);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(pthread, test_sched_setscheduler)
{
	struct sched_param param = {
		.sched_priority = 2,
	};
	int policy = 0;
	int rc = sched_setscheduler(0, policy, &param);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(pthread, test_sched_rr_get_interval)
{
	struct timespec interval = {
		.tv_sec = 0,
		.tv_nsec = 0,
	};
	int rc = sched_rr_get_interval(0, &interval);
	int err = errno;

	zassert_true((rc == -1 && err == ENOSYS));
}

ZTEST(pthread, test_pthread_equal)
{
	zassert_true(pthread_equal(pthread_self(), pthread_self()));
	zassert_false(pthread_equal(pthread_self(), (pthread_t)4242));
}

ZTEST(pthread, test_pthread_set_get_concurrency)
{
	/* EINVAL if the value specified by new_level is negative */
	zassert_equal(EINVAL, pthread_setconcurrency(-42));

	/*
	 * Note: the special value 0 indicates the implementation will
	 * maintain the concurrency level at its own discretion.
	 *
	 * pthread_getconcurrency() should return a value of 0 on init.
	 */
	zassert_equal(0, pthread_getconcurrency());

	for (int i = 0; i <= CONFIG_MP_MAX_NUM_CPUS; ++i) {
		zassert_ok(pthread_setconcurrency(i));
		/* verify parameter is saved */
		zassert_equal(i, pthread_getconcurrency());
	}

	/* EAGAIN if the a system resource to be exceeded */
	zassert_equal(EAGAIN, pthread_setconcurrency(CONFIG_MP_MAX_NUM_CPUS + 1));
}

static void cleanup_handler(void *arg)
{
	bool *boolp = (bool *)arg;

	*boolp = true;
}

static void *test_pthread_cleanup_entry(void *arg)
{
	bool executed[2] = {0};

	pthread_cleanup_push(cleanup_handler, &executed[0]);
	pthread_cleanup_push(cleanup_handler, &executed[1]);
	pthread_cleanup_pop(false);
	pthread_cleanup_pop(true);

	zassert_true(executed[0]);
	zassert_false(executed[1]);

	return NULL;
}

ZTEST(pthread, test_pthread_cleanup)
{
	pthread_t th;

	zassert_ok(pthread_create(&th, NULL, test_pthread_cleanup_entry, NULL));
	zassert_ok(pthread_join(th, NULL));
}

static bool testcancel_ignored;
static bool testcancel_failed;

static void *test_pthread_cancel_fn(void *arg)
{
	zassert_ok(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));

	testcancel_ignored = false;

	/* this should be ignored */
	pthread_testcancel();

	testcancel_ignored = true;

	/* this will mark it pending */
	zassert_ok(pthread_cancel(pthread_self()));

	/* enable the thread to be cancelled */
	zassert_ok(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));

	testcancel_failed = false;

	/* this should terminate the thread */
	pthread_testcancel();

	testcancel_failed = true;

	return NULL;
}

ZTEST(pthread, test_pthread_testcancel)
{
	pthread_t th;

	zassert_ok(pthread_create(&th, NULL, test_pthread_cancel_fn, NULL));
	zassert_ok(pthread_join(th, NULL));
	zassert_true(testcancel_ignored);
	zassert_false(testcancel_failed);
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD)) {
		/* skip redundant testing if there is no thread pool / heap allocation */
		ztest_test_skip();
	}
}

ZTEST_SUITE(pthread, NULL, NULL, before, NULL, NULL);

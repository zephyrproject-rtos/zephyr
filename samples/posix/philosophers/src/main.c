/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_internal.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_THREAD_NAME
#define MAX_NAME_LEN CONFIG_THREAD_MAX_NAME_LEN
#else
#define MAX_NAME_LEN 1
#endif

#define NUM_PHIL      CONFIG_MAX_PTHREAD_COUNT
#define obj_init_type "POSIX"
#define fork_type_str "mutexes"

BUILD_ASSERT(CONFIG_MAX_PTHREAD_COUNT == CONFIG_MAX_PTHREAD_MUTEX_COUNT - 1);
BUILD_ASSERT(CONFIG_DYNAMIC_THREAD_POOL_SIZE == CONFIG_MAX_PTHREAD_COUNT);

typedef pthread_mutex_t *fork_t;

LOG_MODULE_REGISTER(posix_philosophers, LOG_LEVEL_INF);

static pthread_mutex_t forks[NUM_PHIL];
static pthread_t threads[NUM_PHIL];

static inline void fork_init(fork_t frk)
{
	int ret;

	ret = pthread_mutex_init(frk, NULL);
	if (IS_ENABLED(CONFIG_SAMPLE_ERROR_CHECKING) && ret != 0) {
		errno = ret;
		perror("pthread_mutex_init");
		__ASSERT(false, "Failed to initialize fork");
	}
}

static inline fork_t fork(size_t idx)
{
	return &forks[idx];
}

static inline void take(fork_t frk)
{
	int ret;

	ret = pthread_mutex_lock(frk);
	if (IS_ENABLED(CONFIG_SAMPLE_ERROR_CHECKING) && ret != 0) {
		errno = ret;
		perror("pthread_mutex_lock");
		__ASSERT(false, "Failed to lock mutex");
	}
}

static inline void drop(fork_t frk)
{
	int ret;

	ret = pthread_mutex_unlock(frk);
	if (IS_ENABLED(CONFIG_SAMPLE_ERROR_CHECKING) && ret != 0) {
		errno = ret;
		perror("pthread_mutex_unlock");
		__ASSERT(false, "Failed to unlock mutex");
	}
}

static void set_phil_state_pos(int id)
{
	if (IS_ENABLED(CONFIG_SAMPLE_DEBUG_PRINTF)) {
		printk("\x1b[%d;%dH", id + 1, 1);
	}
}

static void print_phil_state(int id, const char *fmt, int32_t delay)
{
	int ret;
	int prio;
	int policy;
	struct sched_param param;

	ret = pthread_getschedparam(pthread_self(), &policy, &param);
	if (IS_ENABLED(CONFIG_SAMPLE_ERROR_CHECKING) && ret != 0) {
		errno = ret;
		perror("pthread_getschedparam");
		__ASSERT(false, "Failed to get scheduler params");
	}

	prio = posix_to_zephyr_priority(param.sched_priority, policy);

	set_phil_state_pos(id);

	printk("Philosopher %d [%s:%s%d] ", id, prio < 0 ? "C" : "P", prio < 0 ? "" : " ", prio);

	if (delay) {
		printk(fmt, delay < 1000 ? " " : "", delay);
	} else {
		printk(fmt, "");
	}

	printk("\n");
}

static int32_t get_random_delay(int id, int period_in_ms)
{
	int32_t ms;
	int32_t delay;
	int32_t uptime;
	struct timespec ts;

	/*
	 * The random delay is unit-less, and is based on the philosopher's ID
	 * and the current uptime to create some pseudo-randomness. It produces
	 * a value between 0 and 31.
	 */
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uptime = ts.tv_sec * MSEC_PER_SEC + (ts.tv_nsec / NSEC_PER_MSEC);
	delay = (uptime / 100 * (id + 1)) & 0x1f;

	/* add 1 to not generate a delay of 0 */
	ms = (delay + 1) * period_in_ms;

	return ms;
}

static inline int is_last_philosopher(int id)
{
	return id == (NUM_PHIL - 1);
}

static void *philosopher(void *arg)
{
	fork_t my_fork1;
	fork_t my_fork2;

	int my_id = POINTER_TO_INT(arg);

	/* Djkstra's solution: always pick up the lowest numbered fork first */
	if (is_last_philosopher(my_id)) {
		my_fork1 = fork(0);
		my_fork2 = fork(my_id);
	} else {
		my_fork1 = fork(my_id);
		my_fork2 = fork(my_id + 1);
	}

	while (1) {
		int32_t delay;

		print_phil_state(my_id, "       STARVING       ", 0);
		take(my_fork1);
		print_phil_state(my_id, "   HOLDING ONE FORK   ", 0);
		take(my_fork2);

		delay = get_random_delay(my_id, 25);
		print_phil_state(my_id, "  EATING  [ %s%d ms ] ", delay);
		usleep(delay * USEC_PER_MSEC);

		drop(my_fork2);
		print_phil_state(my_id, "   DROPPED ONE FORK   ", 0);
		drop(my_fork1);

		delay = get_random_delay(my_id, 25);
		print_phil_state(my_id, " THINKING [ %s%d ms ] ", delay);
		usleep(delay * USEC_PER_MSEC);
	}

	return NULL;
}

static int new_prio(int phil)
{
	if (CONFIG_NUM_COOP_PRIORITIES > 0 && CONFIG_NUM_PREEMPT_PRIORITIES > 0) {
		if (IS_ENABLED(CONFIG_SAMPLE_SAME_PRIO)) {
			return 0;
		}

		return -(phil - (NUM_PHIL / 2));
	}

	if (CONFIG_NUM_COOP_PRIORITIES > 0) {
		return -phil - 2;
	}

	if (CONFIG_NUM_PREEMPT_PRIORITIES > 0) {
		return phil;
	}

	__ASSERT_NO_MSG("Unsupported scheduler configuration");
}

static void init_objects(void)
{
	ARRAY_FOR_EACH(forks, i) {
		LOG_DBG("Initializing fork %zu", i);
		fork_init(fork(i));
	}
}

static void start_threads(void)
{
	int ret;
	int prio;
	int policy;
	struct sched_param param;

	ARRAY_FOR_EACH(forks, i) {
		LOG_DBG("Initializing philosopher %zu", i);
		ret = pthread_create(&threads[i], NULL, philosopher, INT_TO_POINTER(i));
		if (IS_ENABLED(CONFIG_SAMPLE_ERROR_CHECKING) && ret != 0) {
			errno = ret;
			perror("pthread_create");
			__ASSERT(false, "Failed to create thread");
		}

		prio = new_prio(i);
		param.sched_priority = zephyr_to_posix_priority(prio, &policy);
		ret = pthread_setschedparam(threads[i], policy, &param);
		if (IS_ENABLED(CONFIG_SAMPLE_ERROR_CHECKING) && ret != 0) {
			errno = ret;
			perror("pthread_setschedparam");
			__ASSERT(false, "Failed to set scheduler params");
		}

		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			char tname[MAX_NAME_LEN];

			snprintf(tname, sizeof(tname), "Philosopher %zu", i);
			pthread_setname_np(threads[i], tname);
		}
	}
}

#define DEMO_DESCRIPTION                                                                           \
	"\x1b[2J\x1b[15;1H"                                                                        \
	"Demo Description\n"                                                                       \
	"----------------\n"                                                                       \
	"An implementation of a solution to the Dining Philosophers\n"                             \
	"problem (a classic multi-thread synchronization problem).\n"                              \
	"This particular implementation demonstrates the usage of multiple\n"                      \
	"preemptible and cooperative threads of differing priorities, as\n"                        \
	"well as %s %s and thread sleeping.\n",                                                    \
		obj_init_type, fork_type_str

static void display_demo_description(void)
{
	if (IS_ENABLED(CONFIG_SAMPLE_DEBUG_PRINTF)) {
		printk(DEMO_DESCRIPTION);
	}
}

int main(void)
{
	display_demo_description();

	init_objects();
	start_threads();

	if (IS_ENABLED(CONFIG_COVERAGE)) {
		/* Wait a few seconds before main() exit, giving the sample the
		 * opportunity to dump some output before coverage data gets emitted
		 */
		sleep(5);
	}

	return 0;
}

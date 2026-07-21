/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <threads.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/sched.h>
#include <zephyr/sys/clock.h>

struct thrd_trampoline_arg {
	thrd_start_t func;
	void *arg;
	struct k_sem sem;
};

static void *thrd_trampoline(void *arg)
{
	struct thrd_trampoline_arg *ta = arg;
	thrd_start_t func = ta->func;
	void *real_arg = ta->arg;

	k_sem_give(&ta->sem);

	return INT_TO_POINTER(func(real_arg));
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	int ret;
	struct thrd_trampoline_arg ta;

	ta.func = func;
	ta.arg = arg;
	k_sem_init(&ta.sem, 0, 1);

	ret = pthread_create(thr, NULL, thrd_trampoline, &ta);
	if (ret == 0) {
		k_sem_take(&ta.sem, K_FOREVER);
	}

	switch (ret) {
	case 0:
		return thrd_success;
	case EAGAIN:
		return thrd_nomem;
	default:
		return thrd_error;
	}
}

int thrd_equal(thrd_t lhs, thrd_t rhs)
{
	return pthread_equal(lhs, rhs);
}

thrd_t thrd_current(void)
{
	return pthread_self();
}

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
	if (sys_clock_nanosleep(SYS_CLOCK_REALTIME, 0, duration, remaining) != 0) {
		return thrd_error;
	}

	return thrd_success;
}

void thrd_yield(void)
{
	(void)sched_yield();
}

FUNC_NORETURN void thrd_exit(int res)
{
	pthread_exit(INT_TO_POINTER(res));

	CODE_UNREACHABLE;
}

int thrd_detach(thrd_t thr)
{
	switch (pthread_detach(thr)) {
	case 0:
		return thrd_success;
	default:
		return thrd_error;
	}
}

int thrd_join(thrd_t thr, int *res)
{
	void *ret;

	switch (pthread_join(thr, &ret)) {
	case 0:
		if (res != NULL) {
			*res = POINTER_TO_INT(ret);
		}
		return thrd_success;
	default:
		return thrd_error;
	}
}

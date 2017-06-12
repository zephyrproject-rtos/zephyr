/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <tc_util.h>

#define N_THREADS 2

#define STACK_SIZE 384
static K_THREAD_STACK_ARRAY_DEFINE(stacks, N_THREADS, STACK_SIZE);
static struct k_thread threads[N_THREADS];

static int errno_values[N_THREADS + 1] = {
	0xbabef00d,
	0xdeadbeef,
	0xabad1dea,
};

struct result {
	void *q;
	int pass;
};

struct result result[N_THREADS];

struct k_fifo fifo;

static void errno_fiber(int n, int my_errno)
{
	errno = my_errno;

	printk("co-op thread %d, errno before sleep: %x\n", n, errno);

	k_sleep(30 - (n * 10));
	if (errno == my_errno) {
		result[n].pass = 1;
	}

	printk("co-op thread %d, errno after sleep:  %x\n", n, errno);

	k_fifo_put(&fifo, &result[n]);
}

void main(void)
{
	int rv = TC_PASS;

	TC_START("kernel_errno");
	k_fifo_init(&fifo);

	errno = errno_values[N_THREADS];

	printk("main thread, errno before starting co-op threads: %x\n", errno);

	for (int ii = 0; ii < N_THREADS; ii++) {
		result[ii].pass = TC_FAIL;
	}

	for (int ii = 0; ii < N_THREADS; ii++) {
		k_thread_create(&threads[ii], stacks[ii], STACK_SIZE,
				(k_thread_entry_t) errno_fiber,
				(void *) ii, (void *) errno_values[ii], NULL,
				K_PRIO_PREEMPT(ii + 5), 0, K_NO_WAIT);
	}

	for (int ii = 0; ii < N_THREADS; ii++) {
		struct result *p = k_fifo_get(&fifo, 100);

		if (!p || !p->pass) {
			rv = TC_FAIL;
		}
	}

	printk("main thread, errno after running co-op threads: %x\n", errno);

	if (errno != errno_values[N_THREADS]) {
		rv = TC_FAIL;
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}

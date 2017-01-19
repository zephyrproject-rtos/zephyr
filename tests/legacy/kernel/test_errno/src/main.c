/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <tc_util.h>

#define N_FIBERS 2

#define STACK_SIZE 384
static __stack char stacks[N_FIBERS][STACK_SIZE];

static int errno_values[N_FIBERS + 1] = {
	0xbabef00d,
	0xdeadbeef,
	0xabad1dea,
};

struct result {
	void *q;
	int pass;
};

struct result result[N_FIBERS];

struct nano_fifo fifo;

static void errno_fiber(int n, int my_errno)
{
	errno = my_errno;

	printk("fiber %d, errno before sleep: %x\n", n, errno);

	fiber_sleep(3 - n);
	if (errno == my_errno) {
		result[n].pass = 1;
	}

	printk("fiber %d, errno after sleep:  %x\n", n, errno);

	nano_fiber_fifo_put(&fifo, &result[n]);
}

void main(void)
{
	int rv = TC_PASS;

	nano_fifo_init(&fifo);

	errno = errno_values[N_FIBERS];

	printk("task, errno before starting fibers: %x\n", errno);

	for (int ii = 0; ii < N_FIBERS; ii++) {
		result[ii].pass = TC_FAIL;
	}

	for (int ii = 0; ii < N_FIBERS; ii++) {
		task_fiber_start(stacks[ii], STACK_SIZE, errno_fiber,
							ii, errno_values[ii], ii + 5, 0);
	}

	for (int ii = 0; ii < N_FIBERS; ii++) {
		struct result *p = nano_task_fifo_get(&fifo, 10);

		if (!p || !p->pass) {
			rv = TC_FAIL;
		}
	}

	printk("task, errno after running fibers:   %x\n", errno);

	if (errno != errno_values[N_FIBERS]) {
		rv = TC_FAIL;
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}

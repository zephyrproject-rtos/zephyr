/*
 * Copyright (c) 2023 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This sample demonstrates the use of the lightweight scheduler with two
 * lightweight tasks. Each task lasts for a different number of
 * iterations and begins to increment its 'count' after different delays.
 * At the end of its work, each task pauses.
 */

#include <zephyr/kernel.h>
#include <zephyr/lw_sched/lw_sched.h>

static struct lw_scheduler  scheduler;
static struct lw_task       task1;
static struct lw_task      *task2;

struct counter_rules {
	uint32_t count;
	uint32_t delay;
	uint32_t iterations;
};

/* Forward declarations */

static void task_free(void *arg);
static int  task_handler(void *arg);

/* Define information for lightweight task #1 */

static struct counter_rules rules1 = {
	.count = 0,
	.delay = 10,
	.iterations = 70
};
static struct lw_task_ops  ops1 = {
	.execute = task_handler,
	.abort = NULL
};
static struct lw_task_args  args1 = {
	.execute = &rules1,
	.abort = NULL
};

/* Define information for lightweight task #2 */

static struct counter_rules rules2 = {
	.count = 0,
	.delay = 50,
	.iterations = 120,
};
static struct lw_task_ops  ops2 = {
	.execute = task_handler,
	.abort = task_free
};
static struct lw_task_args  args2 = {
	.execute = &rules2,
	.abort = &task2
};

K_THREAD_STACK_DEFINE(sched_stack, 1024);

static int task_handler(void *arg)
{
	struct counter_rules *rules = arg;
	struct lw_task *this_task;

	this_task = lw_task_current_get(&scheduler);

	if (rules->delay != 0) {
		lw_task_delay(this_task, rules->delay);
		rules->delay = 0;
		return LW_TASK_EXECUTE;
	}

	rules->count++;
	rules->iterations--;

	if (rules->iterations == 0) {
		printk("Lightweight task %p is done\n",
		       lw_task_current_get(&scheduler));

		return LW_TASK_PAUSED;
	}

	return LW_TASK_EXECUTE;
}

static void task_free(void *arg)
{
	void **task = (void **)arg;

	printk("Auto-freeing lightweight task %p\n", *task);
	k_free(*task);
}

int main(void)
{
	int  priority;

	priority = k_thread_priority_get(k_current_get());

	printk("Initializing lightweight scheduler ...\n");
	lw_scheduler_init(&scheduler, sched_stack,
			  K_THREAD_STACK_SIZEOF(sched_stack),
			  priority + 1, 0, K_MSEC(50));

	printk("Initializing lightweight task1 (%p)...\n", &task1);
	lw_task_init(&task1, &ops1, &args1, &scheduler, 10);

	task2 = k_malloc(sizeof(struct lw_task));
	printk("Initializing lightweight task2 (%p)...\n", task2);
	lw_task_init(task2, &ops2, &args2, &scheduler, 5);

	lw_task_start(&task1);
	lw_task_start(task2);

	lw_scheduler_start(&scheduler, K_NO_WAIT);

	printk("Sleeping for 10 seconds...\n");

	for (int i = 0; i < 10; i++) {
		k_sleep(K_SECONDS(1));

		printk("Task counts (@ %d sec) : {%u %u}\n",
		       i + 1, rules1.count, rules2.count);
	}

	printk("Shutting down lightweight scheduler ...\n");

	lw_scheduler_abort(&scheduler);

	printk("Final task 1 count: %u\n", rules1.count);
	printk("Final task 2 count: %u\n", rules2.count);

	return 0;
}

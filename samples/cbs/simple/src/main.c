/*
 * Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sched_server/cbs.h>
#include <string.h>
#include "lib/helper.h"

/*
 * Remember that for "pure" EDF results, all user
 * threads must have the same static priority.
 * That happens because EDF will be a tie-breaker
 * among two or more ready tasks of the same static
 * priority. An arbitrary positive number is chosen here.
 */
#define EDF_PRIORITY 5

job_t *create_job(char *msg, int counter)
{
	job_t *job = (job_t *)k_malloc(sizeof(job_t));

	strcpy((char *)job->msg, msg);
	job->counter = counter;
	return job;
}

void destroy_job(job_t *job)
{
	k_free(job);
}

/*
 * The CBS job must return void and receive a void*
 * argument as parameter. The actual argument type is
 * up to you, to be unveiled and used within the
 * function.
 */
void job_function(void *arg)
{
	job_t *job = (job_t *)arg;

	/* prints some info and increments the counter */
	printf("%s %s, %d\n\n", job->msg, CONFIG_BOARD_TARGET, job->counter);
	job->counter++;

	/* busy waits for (approximately) 10 milliseconds */
	k_busy_wait(10000);
}

/*
 * In essence, the CBS is a thread that automatically
 * recalculates its own deadline whenever it executes
 * a job for longer than its allowed time slice (budget).
 * These are the key points of how it works:
 *
 * 1. when a running job consumes its budget entirely,
 * the kernel postpones the deadline (deadline += CBS period)
 * and replenishes the budget.
 *
 * 2. when a job is pushed and the CBS is idle, the
 * kernel checks if the current (budget, deadline) pair
 * yields a higher bandwidth (CPU utilization) than
 * configured. This is not allowed so, if true, the
 * kernel also intervenes. In this case, the budget is
 * replenished and the new deadline is (now + CBS period).

 * The job_function above prints stuff and busy waits
 * for approximately 10 milliseconds, so you can see
 * the CBS in action by tweaking the values below. 3
 * jobs are pushed at once and will execute sequentially,
 * taking roughly 30 milliseconds to complete. So what
 *happens when you drop the budget to lower than K_MSEC(30)?
 */

/* remember: (name, budget, period, static_priority) */
K_CBS_DEFINE(cbs_1, K_MSEC(80), K_MSEC(800), EDF_PRIORITY);

int main(void)
{
	k_sleep(K_SECONDS(2));
	report_cbs_settings();

	job_t *job1 = create_job("[job]\t\tj1 on", 0);

	for (;;) {
		printf("\n");
		k_cbs_push_job(&cbs_1, job_function, job1, K_NO_WAIT);
		k_cbs_push_job(&cbs_1, job_function, job1, K_NO_WAIT);
		k_cbs_push_job(&cbs_1, job_function, job1, K_NO_WAIT);
		k_sleep(K_SECONDS(2));
	}

	destroy_job(job1);
	return 0;
}

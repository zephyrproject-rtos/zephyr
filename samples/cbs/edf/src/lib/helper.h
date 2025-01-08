/*
 * Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

/*
 * Remember that for "pure" EDF results, all
 * user threads must have the same static priority.
 * That happens because EDF will be a tie-breaker
 * among two or more ready tasks of the same static
 * priority. An arbitrary positive number is chosen here.
 */
#define EDF_PRIORITY       5
#define INACTIVE           -1
#define MSEC_TO_CYC(msec)  k_ms_to_cyc_near32(msec)
#define MSEC_TO_USEC(msec) (msec * USEC_PER_MSEC)

typedef struct {
	char id;
	int counter;
	int32_t initial_delay_msec;
	int32_t rel_deadline_msec;
	int32_t period_msec;
	uint32_t wcet_msec;
	k_tid_t thread;
	struct k_msgq queue;
	struct k_timer timer;
} edf_t;

typedef struct {
	char id;
	int counter;
	uint32_t wcet_msec;
} job_t;

job_t *create_job(char id, int counter, uint32_t wcet_msec)
{
	job_t *job = (job_t *)k_malloc(sizeof(job_t));

	job->id = id;
	job->counter = counter;
	job->wcet_msec = wcet_msec;
	return job;
}

void destroy_job(job_t *job)
{
	k_free(job);
}

void report_cbs_settings(void)
{
	printf("\n/////////////////////////////////////////////////////////////////////////////////"
	       "/////\n");
	printf("\nBoard:\t\t%s\n", CONFIG_BOARD_TARGET);
#ifdef CONFIG_CBS
	printf("[CBS]\t\tCBS enabled.\n");
#ifdef CONFIG_CBS_LOG
	printf("[CBS]\t\tCBS events logging: enabled.\n");
#else
	printf("[CBS]\t\tCBS events logging: disabled.\n");
#endif
#ifdef CONFIG_TIMEOUT_64BIT
	printf("[CBS]\t\tSYS 64-bit timeouts: supported.\n");
#else
	printf("[CBS]\t\tSYS 64-bit timeouts: not supported. using 32-bit API instead.\n");
#endif
#else
	printf("[CBS]\t\tCBS disabled.\n");
#endif
	printf("\n/////////////////////////////////////////////////////////////////////////////////"
	       "/////\n\n");
}

#endif

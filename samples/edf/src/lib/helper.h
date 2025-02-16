/*
 * Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

#include <stdio.h>

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
#define CYC_TO_MSEC(cyc)   k_cyc_to_ms_floor32(cyc)

typedef struct {
	int id;
	int32_t initial_delay_msec;
	int32_t rel_deadline_msec;
	int32_t period_msec;
	uint32_t wcet_msec;
	k_tid_t thread;
	struct k_msgq queue;
	struct k_timer timer;
} edf_t;

void cycle(int id, uint32_t wcet, int32_t deadline)
{
	printf("[%d-%d-", id, CYC_TO_MSEC(deadline));
	k_busy_wait(wcet / 5);
	for (int i = 0; i < 4; i++) {
		printf("%d-", id);
		k_busy_wait(wcet / 5);
	}
	printf("%d]-", id);
}

#endif

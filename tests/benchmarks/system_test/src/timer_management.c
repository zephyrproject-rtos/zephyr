/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "timer_management.h"
#include "data_management.h"
#include "log_management.h"

K_MUTEX_DEFINE(tmr_mutex);

static struct tmr_dynamic timers[TMR_COUNT];

void tmr_create(void)
{
	for (int i = 0; i < TMR_COUNT; i++) {
		k_mutex_lock(&tmr_mutex, K_FOREVER);
		timers[i].id = i;
		timers[i].period = ((i + 1) * 10);
		timers[i].wasCreated = true;
		k_mutex_unlock(&tmr_mutex);
		k_timer_init(&timers[i].timer, tmr_expiry_function, NULL);
		k_timer_start(&timers[i].timer, K_MSEC(timers[i].period), K_NO_WAIT);
	}
}

void tmr_expiry_function(struct k_timer *timer_id)
{
	struct tmr_dynamic *t = CONTAINER_OF(timer_id, struct tmr_dynamic, timer);

	k_mutex_lock(&tmr_mutex, K_FOREVER);
	t->wasExpired = true;
	k_mutex_unlock(&tmr_mutex);
}

void tmr_summary(void)
{
	fprintf(output_file, "\n==================================================================="
			     "=============");
	fprintf(output_file, "\nTEST: Timer");
	fprintf(output_file, "\n==================================================================="
			     "=============");
	int created_timers = 0;
	int expired_timers = 0;

	for (int i = 0; i < TMR_COUNT; i++) {
		if (timers[i].wasCreated) {
			created_timers += 1;
		}
		if (timers[i].wasExpired) {
			expired_timers += 1;
		}
	}

	fprintf(output_file, "\n-> Tested %d timers.", TMR_COUNT);
	fprintf(output_file, "\n-> Created %d timers.", created_timers);
	fprintf(output_file, "\n-> Expired %d timers.", expired_timers);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	const char *test_result_message =
		(TMR_COUNT == created_timers && expired_timers == TMR_COUNT)
			? "\nTimer: " GREEN("PASSED")
			: "\nTimer: " RED("FAILED");
	fprintf(output_file, "%s", test_result_message);
}

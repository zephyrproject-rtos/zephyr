/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thread_management.h"
#include "log_management.h"

K_MUTEX_DEFINE(thr_mutex);

volatile int thr_id;

K_THREAD_STACK_ARRAY_DEFINE(thr_stack_area, THR_COUNT, THR_DATA_LEN);

static struct thr_dynamic threads[THR_COUNT];

void thr_entry_point(void *arg1, void *arg2, void *arg3)
{
	k_mutex_lock(&thr_mutex, K_FOREVER);
	threads[thr_id].wasHandled = true;
	thr_id++;
	k_mutex_unlock(&thr_mutex);
}

void thr_create(void)
{
	thr_id = 0;

	for (int i = 0; i < THR_COUNT; i++) {
		k_mutex_lock(&thr_mutex, K_FOREVER);
		threads[i].id = i;
		threads[i].delay = (i + 1) * 20;
		k_thread_create(&threads[i].thread, thr_stack_area[i], THR_DATA_LEN,
				thr_entry_point, (void *)(intptr_t)threads[i].delay, NULL, NULL,
				THR_PRIORITY, 0, K_MSEC(threads[i].delay));
		threads[i].wasCreated = true;
		k_mutex_unlock(&thr_mutex);
	}
}

void thr_join_all(void)
{
	/* Wait for all threads to complete */
	for (int i = 0; i < THR_COUNT; i++) {
		k_thread_join(&threads[i].thread, K_FOREVER);
	}
}

void thr_summary(void)
{
	fprintf(output_file, "\n==================================================================="
			     "=============");
	fprintf(output_file, "\nTEST: Thread");
	fprintf(output_file, "\n==================================================================="
			     "=============");
	int created_threads = 0;
	int hanlded_threads = 0;

	for (int i = 0; i < THR_COUNT; i++) {
		if (threads[i].wasCreated) {
			created_threads += 1;
		}
		if (threads[i].wasHandled) {
			hanlded_threads += 1;
		}
	}

	fprintf(output_file, "\n-> Tested %d threads.", THR_COUNT);
	fprintf(output_file, "\n-> Created %d threads.", created_threads);
	fprintf(output_file, "\n-> Handled %d threads.", hanlded_threads);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	const char *test_result_message =
		(THR_COUNT == created_threads && hanlded_threads == THR_COUNT)
			? "\nThread: " GREEN("PASSED")
			: "\nThread: " RED("FAILED");
	fprintf(output_file, "%s", test_result_message);
}

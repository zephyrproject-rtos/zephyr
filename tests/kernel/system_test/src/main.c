/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include "timer_management.h"
#include "log_management.h"
#include "data_management.h"
#include "thread_management.h"

/* Main application function */
int main(void)
{
	log_init_output();
	log_printDescription();
	fprintf(output_file, "TEST: Start.");
	fprintf(output_file, "\n-> Preparing random data...");
	prepare_data();
	fprintf(output_file, "\n-> Creating threads...");
	thr_create();
	fprintf(output_file, "\n-> Creating timers...");
	tmr_create();
	fprintf(output_file, "\n-> Pushing data to Fifo...");
	fifo_push();
	fprintf(output_file, "\n-> Pushing data to Lifo...");
	lifo_push();
	fprintf(output_file, "\n-> Pushing data to Stack...");
	stack_push();
	fprintf(output_file, "\n-> Pushing data to Ring Buffer...");
	ring_buf_push();
	fprintf(output_file, "\n-> Popping data from Fifo...");
	fifo_pop();
	fprintf(output_file, "\n-> Popping data from Lifo...");
	lifo_pop();
	fprintf(output_file, "\n-> Popping data from Stack...");
	stack_pop();
	fprintf(output_file, "\n-> Popping data from Ring Buffer...");
	ring_buf_pop();
	fprintf(output_file, "\n-> Waiting for join all threads...");
	thr_join_all();
	fprintf(output_file, "\nTEST: End.");

	tmr_summary();
	thr_summary();
	data_summary();

	return 0;
}

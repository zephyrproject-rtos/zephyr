/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log_management.h"
#include "timer_management.h"
#include "thread_management.h"
#include "data_management.h"

FILE *output_file;

void log_print_data(char *title, uint8_t *data, uint8_t size)
{
	fprintf(output_file, "%s", title);
	fprintf(output_file, "-> ");
	for (uint8_t i = 0; i < size; i++) {
		fprintf(output_file, "%d", data[i]);
		if (i < size - 1) {
			fprintf(output_file, ",");
		}
	}
}

void log_printDescription(void)
{
	fprintf(output_file, "\n============================== Kernel System Test "
			     "==============================");
	fprintf(output_file, "\nTEST : System_Test");
	fprintf(output_file, "\n Tested Component   | Covered API");
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	fprintf(output_file, "\n - Fifo:            | k_fifo_put(), k_fifo_get()");
	fprintf(output_file, "\n - Lifo:            | k_lifo_put, k_lifo_get");
	fprintf(output_file, "\n - Stack:           | k_stck_push(), k_stack_pop()");
	fprintf(output_file, "\n - Ring Buffer:     | ring_buf_put(), ring_buf_get()");
	fprintf(output_file,
		"\n - Timer:           | k_timer_init(), k_timer_start(), k_timer_expiry_t");
	fprintf(output_file,
		"\n - Thread:          | k_thread_create(), k_thread_join(), k_thread_entry_t");
	fprintf(output_file,
		"\n - Mutex:           | k_mutex_lock(), k_mutex_unlock(), K_MUTEX_DEFINE");
	fprintf(output_file, "\n - Random Generator:| sys_rand8_get()");
	fprintf(output_file, "\n==================================================================="
			     "=============\n");
}

void log_init_output(void)
{
	output_file = stdout;
}

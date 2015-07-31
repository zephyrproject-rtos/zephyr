/* profiler_collector_sample.c - Profiler sample project */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef CONFIG_NANOKERNEL
#include <nanokernel.h>
#include <arch/cpu.h>
#else
#include <zephyr.h>
#endif

#include "phil.h"
#include <misc/profiler.h>
#include <string.h>

#define RANDOM(x) (((nano_tick_get_32() * ((x) +1)) & 0x2F) + 1)

#define TEST_EVENT_ID 255

extern void philEntry(void);

#define STSIZE 1024
char __stack profiler_stack[2][STSIZE];

struct context_switch_data_t {
	uint32_t context_id;
	uint32_t last_time_executed;
	uint32_t count;
};

int total_dropped_counter=0;

#define MAX_BUFFER_CONTEXT_DATA       20

struct context_switch_data_t
	context_switch_summary_data[MAX_BUFFER_CONTEXT_DATA];

struct event_logger sys_profiler;


void register_context_switch_data(uint32_t timestamp, uint32_t context_id)
{
	int found;
	int i;

	found=0;
	for (i=0; (i<MAX_BUFFER_CONTEXT_DATA) && (found==0); i++) {
		if (context_switch_summary_data[i].context_id == context_id) {
			context_switch_summary_data[i].last_time_executed = timestamp;
			context_switch_summary_data[i].count += 1;
			found=1;
		}
	}

	if (!found) {
		for (i=0; i<MAX_BUFFER_CONTEXT_DATA; i++) {
			if (context_switch_summary_data[i].context_id == 0) {
				context_switch_summary_data[i].context_id = context_id;
				context_switch_summary_data[i].last_time_executed = timestamp;
				context_switch_summary_data[i].count = 1;
				break;
			}
		}
	}
}


void print_context_data(uint32_t context_id, uint32_t count,
	uint32_t last_time_executed, int indice)
{
	PRINTF("\x1b[%d;2H%u    ", 15 + indice, context_id);
	PRINTF("\x1b[%d;14H%u    ", 15 + indice, count);
}


/**
 * @brief Summary data printer fiber
 *
 * @details Print the summary data of the context swith events
 * and the total dropped event ocurred.
 *
 * @return No return value.
 */
void summary_data_printer(void)
{
	int i;

	while(1) {
		/* print dropped event counter */
		PRINTF("\x1b[11;1HDropped events occured: %d   ", total_dropped_counter);

		/* print context switch data */
		PRINTF("\x1b[13;1HContext switch summary");
		PRINTF("\x1b[14;1HContext Id   Amount of context switches");
		for (i=0; i<MAX_BUFFER_CONTEXT_DATA; i++) {
			if (context_switch_summary_data[i].context_id != 0) {
				print_context_data(context_switch_summary_data[i].context_id,
					context_switch_summary_data[i].count,
					context_switch_summary_data[i].last_time_executed, i);
			}
		}

		/* Sleep */
		fiber_sleep(50);
	}
}


/**
 * @brief Profiler data collector fiber
 *
 * @details Collect the profiler messages and process them depending
 * the kind of event received.
 *
 * @return No return value.
 */
void profiling_data_collector(void)
{
	int res;
	uint32_t data[4];
	union event_header *header;

	/* We register the fiber as collector to avoid this fiber generating a
	 * context switch event every time it collects the data
	 */
	sys_profiler_register_as_collector();

	while(1) {
		/* collect the data */
		res = sys_profiler_get_wait(data, 10);
		if (res > 0) {
			header = (union event_header *)&(data[0]);

			/* Register the amount of droppped events occurred */
			if (header->bits.dropped_count) {
				total_dropped_counter += header->bits.dropped_count;
			}

			/* process the data */
			switch (header->bits.event_id) {
			case PROFILER_CONTEXT_SWITCH_EVENT_ID:
				if (header->bits.data_length != 2) {
					PRINTF("\x1b[13;1HError in context switch message. "
						"event_id = %d, Expected %d, received %d\n",
						header->bits.event_id, 2, header->bits.data_length);
				} else {
					register_context_switch_data(data[1], data[2]);
				}
				break;
			}
		} else {
			/* This error should never happen */
			if (res == -EMSGSIZE) {
				PRINTF("FATAL ERROR. The buffer provided to collect the"
					"profiling events is too small\n");
			}
		}
	}
}


/**
 * @brief Start the demo fibers
 *
 * @details Start the profiler data colector fiber and the summary printer
 * fiber that shows the context switch data.
 *
 * @return No return value.
 */
void profiler_fiber_start(void)
{
	PRINTF("\x1b[2J\x1b[15;1H");
	task_fiber_start(&profiler_stack[0][0], STSIZE,
		(nano_fiber_entry_t) profiling_data_collector, 0, 0, 6, 0);
	task_fiber_start(&profiler_stack[1][0], STSIZE,
		(nano_fiber_entry_t) summary_data_printer, 0, 0, 6, 0);
}

#ifdef CONFIG_NANOKERNEL
char __stack philStack[N_PHILOSOPHERS][STSIZE];
struct nano_sem forks[N_PHILOSOPHERS];

/**
 * @brief Manokernel entry point.
 *
 * @details Start the profiler data colector fiber. Then
 * do wait forever.
 * @return No return value.
 */
int main(void)
{
	int i;
	profiler_fiber_start();

	/* initialize philosopher semaphores */
	for (i = 0; i < N_PHILOSOPHERS; i++) {
		nano_sem_init(&forks[i]);
		nano_task_sem_give(&forks[i]);
	}

	/* create philosopher fibers */
	for (i = 0; i < N_PHILOSOPHERS; i++) {
		task_fiber_start(&philStack[i][0], STSIZE,
						(nano_fiber_entry_t) philEntry, 0, 0, 6, 0);
	}

	/* wait forever */
	while (1) {
		extern void nano_cpu_idle(void);
		nano_cpu_idle();
	}
}

#else

/**
 * @brief Microkernel task.
 *
 * @details Start the profiler data colector fiber. Then
 * do wait forever.
 *
 * @return No return value.
 */
void profiler_demo(void)
{
	profiler_fiber_start();

	task_group_start(PHI);
}
#endif

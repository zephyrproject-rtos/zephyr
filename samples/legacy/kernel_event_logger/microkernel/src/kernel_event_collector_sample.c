/* kernel_event_collector_sample.c - Kernel event collector sample project */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "phil.h"
#include <logging/kernel_event_logger.h>
#include <string.h>

#ifdef NANO_APIS_ONLY
  #define TAKE(x) nano_fiber_sem_take(&x, TICKS_UNLIMITED)
  #define GIVE(x) nano_fiber_sem_give(&x)
  #define SLEEP(x) fiber_sleep(x)
#else
  #define TAKE(x) task_mutex_lock(x, TICKS_UNLIMITED)
  #define GIVE(x) task_mutex_unlock(x)
  #define SLEEP(x) task_sleep(x)
#endif

#define RANDDELAY(x) myDelay(((sys_tick_get_32() * ((x) + 1)) & 0x2f) + 1)

#define TEST_EVENT_ID 255

extern void philEntry(void);

#define STSIZE 512
char __stack kernel_event_logger_stack[2][STSIZE];

struct context_switch_data_t {
	uint32_t thread_id;
	uint32_t last_time_executed;
	uint32_t count;
};

int total_dropped_counter;

#define MAX_BUFFER_CONTEXT_DATA       20

struct context_switch_data_t
	context_switch_summary_data[MAX_BUFFER_CONTEXT_DATA];

unsigned int interrupt_counters[255];


struct sleep_data_t {
	uint32_t awake_cause;
	uint32_t last_time_slept;
	uint32_t last_duration;
};

struct sleep_data_t sleep_event_data;

int is_busy_task_awake;
int forks_available = 1;

#ifdef CONFIG_TASK_MONITOR
struct tmon_data_t {
	uint32_t event_type;
	uint32_t timestamp;
	uint32_t task_id;
	uint32_t data;
};

uint32_t tmon_index;

struct tmon_data_t
	tmon_summary_data[MAX_BUFFER_CONTEXT_DATA];
#endif

void register_context_switch_data(uint32_t timestamp, uint32_t thread_id)
{
	int found;
	int i;

	found = 0;
	for (i = 0; (i < MAX_BUFFER_CONTEXT_DATA) && (found == 0); i++) {
		if (context_switch_summary_data[i].thread_id == thread_id) {
			context_switch_summary_data[i].last_time_executed = timestamp;
			context_switch_summary_data[i].count += 1;
			found = 1;
		}
	}

	if (!found) {
		for (i = 0; i < MAX_BUFFER_CONTEXT_DATA; i++) {
			if (context_switch_summary_data[i].thread_id == 0) {
				context_switch_summary_data[i].thread_id = thread_id;
				context_switch_summary_data[i].last_time_executed = timestamp;
				context_switch_summary_data[i].count = 1;
				break;
			}
		}
	}
}

void register_interrupt_event_data(uint32_t timestamp, uint32_t irq)
{
	if (irq < 255) {
		interrupt_counters[irq] += 1;
	}
}


void register_sleep_event_data(uint32_t time_start, uint32_t duration,
	uint32_t cause)
{
	sleep_event_data.awake_cause = cause;
	sleep_event_data.last_time_slept = time_start;
	sleep_event_data.last_duration = duration;
}


void print_context_data(uint32_t thread_id, uint32_t count,
	uint32_t last_time_executed, int indice)
{
	PRINTF("\x1b[%d;1H%u    ", 16 + indice, thread_id);
	PRINTF("\x1b[%d;12H%u    ", 16 + indice, count);
}

#ifdef CONFIG_TASK_MONITOR
void register_tmon_data(uint32_t event_type, uint32_t timestamp,
	uint32_t task_id, uint32_t data)
{
	tmon_summary_data[tmon_index].event_type = event_type;
	tmon_summary_data[tmon_index].timestamp = timestamp;
	tmon_summary_data[tmon_index].task_id = task_id;
	tmon_summary_data[tmon_index].data = data;

	if (++tmon_index == MAX_BUFFER_CONTEXT_DATA) {
		tmon_index = 0;
	}
}

void print_tmon_status_data(int index)
{
	switch (tmon_summary_data[index].event_type) {
	case KERNEL_EVENT_LOGGER_TASK_MON_TASK_STATE_CHANGE_EVENT_ID:
		PRINTF("\x1b[%d;64HEVENT    ", 4 + index);
		break;
	case KERNEL_EVENT_LOGGER_TASK_MON_CMD_PACKET_EVENT_ID:
		PRINTF("\x1b[%d;64HPACKET    ", 4 + index);
		break;
	case KERNEL_EVENT_LOGGER_TASK_MON_KEVENT_EVENT_ID:
		PRINTF("\x1b[%d;64HCOMMAND    ", 4 + index);
		break;
	}
	PRINTF("\x1b[%d;76H%u    ", 4 + index,
		tmon_summary_data[index].timestamp);
	if (tmon_summary_data[index].task_id != -1) {
		PRINTF("\x1b[%d;88H0x%x    ", 4 + index,
			tmon_summary_data[index].task_id);
	} else {
		PRINTF("\x1b[%d;88H----------    ", 4 + index);
	}
	PRINTF("\x1b[%d;100H0x%x    ", 4 + index,
		tmon_summary_data[index].data);
}
#endif

void fork_manager_entry(void)
{
	int i;
#ifdef NANO_APIS_ONLY
	/* externs */
	extern struct nano_sem forks[N_PHILOSOPHERS];
#else
	kmutex_t forks[] = {forkMutex0, forkMutex1, forkMutex2, forkMutex3, forkMutex4, forkMutex5};
#endif

	SLEEP(2000);
	while (1) {
		if (forks_available) {
			/* take all forks */
			for (i = 0; i < N_PHILOSOPHERS; i++) {
				TAKE(forks[i]);
			}

			/* Philosophers won't be able to take any fork for 2000 ticks */
			forks_available = 0;
			SLEEP(2000);
		} else {
			/* give back all forks */
			for (i = 0; i < N_PHILOSOPHERS; i++) {
				GIVE(forks[i]);
			}

			/* Philosophers will be able to take forks for 2000 ticks */
			forks_available = 1;
			SLEEP(2000);
		}
	}
}


void busy_task_entry(void)
{
	int ticks_when_awake;
	int i = 0;

	while (1) {
		/*
		 * go to sleep for 1000 ticks allowing the system entering to sleep
		 * mode if required.
		 */
		is_busy_task_awake = 0;
		SLEEP(1000);
		ticks_when_awake = sys_tick_get_32();

		/*
		 * keep the cpu busy for 1000 ticks preventing the system entering
		 * to sleep mode.
		 */
		is_busy_task_awake = 1;
		while (sys_tick_get_32() - ticks_when_awake < 1000) {
			i++;
		}
	}
}


/**
 * @brief Summary data printer fiber
 *
 * @details Print the summary data of the context switch events
 * and the total dropped event ocurred.
 *
 * @return No return value.
 */
void summary_data_printer(void)
{
	int i;

	while (1) {
		/* print task data */
		PRINTF("\x1b[1;32HFork manager task");
		if (forks_available) {
			PRINTF("\x1b[2;32HForks : free to use");
		} else {
			PRINTF("\x1b[2;32HForks : all taken  ");
		}

#ifndef NANO_APIS_ONLY
		/* Due to fiber are not pre-emptive, the busy_task_entry thread won't
		 * run as a fiber in nanokernel-only system, because it would affect
		 * the visualization of the sample and the collection of the data
		 * while running busy.
		 */
		PRINTF("\x1b[4;32HWorker task");
		if (is_busy_task_awake) {
			PRINTF("\x1b[5;32HState : BUSY");
			PRINTF("\x1b[6;32H(Prevent the system going idle)");
		} else {
			PRINTF("\x1b[5;32HState : IDLE");
			PRINTF("\x1b[6;32H                               ");
		}
#endif

		/* print general data */
		PRINTF("\x1b[8;1HGENERAL DATA");
		PRINTF("\x1b[9;1H------------");

		PRINTF("\x1b[10;1HSystem tick count : %d    ", sys_tick_get_32());

		/* print dropped event counter */
		PRINTF("\x1b[11;1HDropped events #  : %d   ", total_dropped_counter);

		/* Print context switch event data */
		PRINTF("\x1b[13;1HCONTEXT SWITCH EVENT DATA");
		PRINTF("\x1b[14;1H-------------------------");
		PRINTF("\x1b[15;1HThread ID   Switches");
		for (i = 0; i < MAX_BUFFER_CONTEXT_DATA; i++) {
			if (context_switch_summary_data[i].thread_id != 0) {
				print_context_data(context_switch_summary_data[i].thread_id,
					context_switch_summary_data[i].count,
					context_switch_summary_data[i].last_time_executed, i);
			}
		}

		/* Print sleep event data */
		PRINTF("\x1b[8;32HSLEEP EVENT DATA");
		PRINTF("\x1b[9;32H----------------");
		PRINTF("\x1b[10;32HLast sleep event received");
		if (sleep_event_data.last_time_slept > 0) {
			PRINTF("\x1b[11;32HExit cause : irq #%u   ",
				sleep_event_data.awake_cause);
			PRINTF("\x1b[12;32HAt tick    : %u        ",
				sleep_event_data.last_time_slept);
			PRINTF("\x1b[13;32HDuration   : %u ticks     ",
				sleep_event_data.last_duration);
		}

		/* Print interrupt event data */
		PRINTF("\x1b[15;32HINTERRUPT EVENT DATA");
		PRINTF("\x1b[16;32H--------------------");
		PRINTF("\x1b[17;32HInterrupt counters");

		int line = 0;

		for (i = 0; i < 255; i++) {
			if (interrupt_counters[i] > 0) {
				PRINTF("\x1b[%d;%dHirq #%d : %d times", 18 + line, 32, i,
					interrupt_counters[i]);
				line++;
			}
		}

#ifdef CONFIG_TASK_MONITOR
		/* Print task monitor status data */
		PRINTF("\x1b[1;64HTASK MONITOR STATUS DATA");
		PRINTF("\x1b[2;64H-------------------------");
		PRINTF("\x1b[3;64HEvento\tTimestamp\tTaskId\tData");
		for (i = 0; i < MAX_BUFFER_CONTEXT_DATA; i++) {
			if (tmon_summary_data[i].timestamp != 0) {
				print_tmon_status_data(i);
			}
		}
#endif

		/* Sleep */
		fiber_sleep(50);
	}
}


/**
 * @brief Kernel event data collector fiber
 *
 * @details Collect the kernel event messages and process them depending
 * the kind of event received.
 *
 * @return No return value.
 */
void profiling_data_collector(void)
{
	int res;
	uint32_t data[4];
	uint8_t dropped_count;
	uint16_t event_id;

	/* We register the fiber as collector to avoid this fiber generating a
	 * context switch event every time it collects the data
	 */
	sys_k_event_logger_register_as_collector();

	while (1) {
		/* collect the data */
		uint8_t data_length = SIZE32_OF(data);

		res = sys_k_event_logger_get_wait(&event_id, &dropped_count, data,
					    &data_length);
		if (res > 0) {
			/* Register the amount of droppped events occurred */
			if (dropped_count) {
				total_dropped_counter += dropped_count;
			}

			/* process the data */
			switch (event_id) {
#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
			case KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID:
				if (data_length != 2) {
					PRINTF("\x1b[13;1HError in context switch message. "
						"event_id = %d, Expected %d, received %d\n",
						event_id, 2, data_length);
				} else {
					register_context_switch_data(data[0], data[1]);
				}
				break;
#endif
#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
			case KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID:
				if (data_length != 2) {
					PRINTF("\x1b[13;1HError in interrupt message. "
						"event_id = %d, Expected %d, received %d\n",
						event_id, 2, data_length);
				} else {
					register_interrupt_event_data(data[0], data[1]);
				}
				break;
#endif
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
			case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
				if (data_length != 3) {
					PRINTF("\x1b[13;1HError in sleep message. "
						"event_id = %d, Expected %d, received %d\n",
						event_id, 3, data_length);
				} else {
					register_sleep_event_data(data[0], data[1], data[2]);
				}
				break;
#endif
#ifdef CONFIG_TASK_MONITOR
			case KERNEL_EVENT_LOGGER_TASK_MON_TASK_STATE_CHANGE_EVENT_ID:
			case KERNEL_EVENT_LOGGER_TASK_MON_CMD_PACKET_EVENT_ID:
				if (data_length != 3) {
					PRINTF("\x1b[13;1HError in task monitor message. "
						"event_id = %d, Expected 3, received %d\n",
						event_id, data_length);
				} else {
					register_tmon_data(event_id, data[0], data[1], data[2]);
				}
				break;

			case KERNEL_EVENT_LOGGER_TASK_MON_KEVENT_EVENT_ID:
				if (data_length != 2) {
					PRINTF("\x1b[13;1HError in task monitor message. "
						"event_id = %d, Expected 2, received %d\n",
						event_id, data_length);
				} else {
					register_tmon_data(event_id, data[0], -1, data[1]);
				}
				break;
#endif
			default:
				PRINTF("unrecognized event id %d", event_id);
			}
		} else {
			/* This error should never happen */
			if (res == -EMSGSIZE) {
				PRINTF("FATAL ERROR. The buffer provided to collect the "
					"profiling events is too small\n");
			}
		}
	}
}


/**
 * @brief Start the demo fibers
 *
 * @details Start the kernel event data colector fiber and the summary printer
 * fiber that shows the context switch data.
 *
 * @return No return value.
 */
void kernel_event_logger_fiber_start(void)
{
	PRINTF("\x1b[2J\x1b[15;1H");
	task_fiber_start(&kernel_event_logger_stack[0][0], STSIZE,
		(nano_fiber_entry_t) profiling_data_collector, 0, 0, 6, 0);
	task_fiber_start(&kernel_event_logger_stack[1][0], STSIZE,
		(nano_fiber_entry_t) summary_data_printer, 0, 0, 6, 0);
}

#ifdef NANO_APIS_ONLY
char __stack philStack[N_PHILOSOPHERS+1][STSIZE];
struct nano_sem forks[N_PHILOSOPHERS];

/**
 * @brief Manokernel entry point.
 *
 * @details Start the kernel event data colector fiber. Then
 * do wait forever.
 * @return No return value.
 */
int main(void)
{
	int i;

#ifdef CONFIG_TASK_MONITOR
	tmon_index = 0;
#endif
	kernel_event_logger_fiber_start();

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

	task_fiber_start(&philStack[N_PHILOSOPHERS][0], STSIZE,
		(nano_fiber_entry_t) fork_manager_entry, 0, 0, 6, 0);

	/* wait forever */
	while (1) {
		nano_cpu_idle();
	}
}

#else

/**
 * @brief Microkernel task.
 *
 * @details Start the kernel event data colector fiber. Then
 * do wait forever.
 *
 * @return No return value.
 */
void k_event_logger_demo(void)
{
	kernel_event_logger_fiber_start();

	task_group_start(PHI);
}
#endif

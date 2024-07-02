/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/instrumentation/instrumentation.h>

/*
 * This sample shows the instrumentation subsystem tracing and profiling
 * features. It basically consists in two threads in a ping-pong mode, taking
 * turns to execute loops that spend some CPU cycles.
 *
 * After one flashes this sample to the target, it must be possible to
 * collect and visualize traces and profiling info using the instrumentation CLI
 * tool, zaru. Please _note_ that this subsystem uses the retained_mem driver,
 * hence it's necessary to add the proper devicetree overlay for the target
 * board (see './boards/b_u585i_iot02a.overlay' in this dir. as a reference).
 *
 * Example:
 *
 * $ export ZEPHYR_BASE=~/zephyrproject/zephyr
 *
 * $ west build -p always -b b_u585i_iot02a samples/subsys/instrumentation -t flash
 * (wait ~2 seconds so the sample finishes 2 rounds of ping-pong between 'main'
 * and 'thread_A')
 *
 * (check instrumentation status)
 * $ scripts/zaru.py status
 *
 * (set the tracing / profiling location; in this sample the function
 * 'get_sem_and_exec_function' is the one interesting to allow the observation
 * of context switches)
 * $ scripts/zaru.py trace -v -c get_sem_and_exec_function
 *
 * (reboot target so tracing/profiling at the location is effective)
 * $ scripts/zaru.py reboot
 * (wait ~2 seconds so the sample finishes 2 rounds of ping-pong between 'main'
 * and 'thread_A')
 *
 * (get traces)
 * $ scripts/zaru.py trace -v
 *
 * (.. and get profile)
 * $ scripts/zaru.py profile -v -n 10
 *
 * Or alternatively, export the traces to Perfetto:
 *
 * (it's necessary to reboot because 'zaru.py trace' dumped the buffer and it's
 * now empty)
 * $ scripts/zaru.py reboot
 * (wait ~2 seconds so the sample finishes 2 rounds of ping-pong between 'main'
 * and 'thread_A')
 *
 * $ scripts/zaru.py trace -v --perfetto --output perfetto_zephyr.json
 * (then go to http://perfetto.dev, Trace Viewer, and load perfetto_zephyr.json)
 *
 */

#define SLEEPTIME 10
#define STACKSIZE 1024
#define PRIORITY  7

void __no_optimization loop_0(void)
{
	/* Just loop to spend some cycles */
	for (int i = 0; i < 1024; i++) {
		/* Empty */
	};
}

void __no_optimization loop_1(void)
{
	/* Just loop to spend some cycles */
	for (int i = 0; i < 1024*512; i++) {
		/* Empty */
	};
}

/*
 * 'main' thread can take its mutex promptly and run. 'thread_A' needs to wait 'main' give its
 * (thread_A) mutex to run.
 */
K_SEM_DEFINE(main_sem, 1, 1);		/* Initialized as ready to be taken */
K_SEM_DEFINE(thread_a_sem, 0, 1);	/* Initialized as already taken (blocked) */

K_THREAD_STACK_DEFINE(thread_a_stack, STACKSIZE);
static struct k_thread thread_a_data;

static int counter;

void get_sem_and_exec_function(struct k_sem *my_sem, struct k_sem *other_sem, void (*func)(void))
{
	while (counter < 4) {
		k_sem_take(my_sem, K_FOREVER);

		func();
		k_msleep(SLEEPTIME);

		counter++;
		k_sem_give(other_sem);
	}
}

void thread_A(void *notused0, void *notused1, void *notused2)
{
	ARG_UNUSED(notused0);
	ARG_UNUSED(notused1);
	ARG_UNUSED(notused2);

	get_sem_and_exec_function(&thread_a_sem, &main_sem, loop_0);
}

int main(void)
{
	k_tid_t thread_a;

	/* Create Thread A */
	thread_a = k_thread_create(&thread_a_data, thread_a_stack, STACKSIZE, thread_A, NULL, NULL,
			NULL, PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(thread_a, "thread_A");

	/* Start ping-pong between 'main' and 'thread_A' */
	get_sem_and_exec_function(&main_sem, &thread_a_sem, loop_1);
}

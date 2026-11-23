/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <sample_usbd.h>
#include <zephyr/tracing/tracing.h>

/*
 * Tracing demonstration.
 *
 * This sample exercises a representative cross-section of the kernel so that,
 * regardless of which tracing format and backend are selected at build time, a
 * rich and continuous stream of trace events is produced for visualization on
 * the host:
 *
 *   - Two threads (one static, one dynamic) ping-pong "Hello World" greetings,
 *     synchronising with a pair of semaphores. This keeps generating thread
 *     switch, sleep and semaphore give/take events for as long as the sample
 *     runs.
 *   - On start-up, a one-shot workload drives a mutex, the system work queue
 *     and a kernel timer so that those objects' lifecycle events also appear in
 *     the trace.
 *   - sys_trace_named_event() is used throughout to emit custom, application
 *     defined events (a short name plus two 32-bit arguments). Named events are
 *     the primary way to annotate a trace from application code. Note that some
 *     backends truncate the name (CTF keeps the first 19 characters).
 *
 * The README lists the supported formats (CTF, SystemView, Percepio and the
 * "test" and "user" formats) and backends (UART, USB, POSIX/native, RAM, ...)
 * and explains how to capture and decode the output for each.
 */

/* size of stack area used by each thread */
#define STACKSIZE 2048

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 500

static uint32_t counter;

/*
 * Objects exercised once at start-up by run_tracing_workload(). Each kernel
 * primitive emits its own sys_port_trace_* events automatically; defining a
 * spread of object types here means more of those event families show up in
 * the captured trace.
 */

K_MUTEX_DEFINE(demo_mutex);

K_SEM_DEFINE(work_done, 0, 1);

static void demo_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	/* Runs in the system work queue thread context. */
	sys_trace_named_event("work_run", 0, 0);
	k_sem_give(&work_done);
}
K_WORK_DEFINE(demo_work, demo_work_handler);

static void demo_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	/* Runs in the system clock (timer) interrupt context. */
	sys_trace_named_event("timer_fired", 0, 0);
}
K_TIMER_DEFINE(demo_timer, demo_timer_handler, NULL);

/*
 * Drive a handful of kernel primitives once at start-up. The named events
 * bracket each phase so the activity is easy to locate in the captured trace,
 * while the underlying mutex/work/timer calls generate the corresponding
 * kernel object trace events.
 */
static void run_tracing_workload(void)
{
	sys_trace_named_event("workload_start", 0, 0);

	/* Mutex: lock then unlock. */
	k_mutex_lock(&demo_mutex, K_FOREVER);
	sys_trace_named_event("mutex_locked", 0, 0);
	k_mutex_unlock(&demo_mutex);

	/* System work queue: submit a work item and wait for it to run. */
	k_work_submit(&demo_work);
	k_sem_take(&work_done, K_FOREVER);

	/* Timer: arm a one-shot timer and wait for it to expire. */
	k_timer_start(&demo_timer, K_MSEC(10), K_NO_WAIT);
	k_timer_status_sync(&demo_timer);

	sys_trace_named_event("workload_done", 0, 0);
}

/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void helloLoop(const char *my_name,
	       struct k_sem *my_sem, struct k_sem *other_sem)
{
	const char *tname;

	while (1) {
		/* take my semaphore */
		k_sem_take(my_sem, K_FOREVER);

		/* Emit a named event carrying the running counter value. */
		sys_trace_named_event("counter_value", counter, 0);
		counter++;

		/* say "hello" */
		tname = k_thread_name_get(k_current_get());
		if (tname == NULL) {
			printk("%s: Hello World from %s!\n",
				my_name, CONFIG_BOARD);
		} else {
			printk("%s: Hello World from %s!\n",
				tname, CONFIG_BOARD);
		}

		/* wait a while, then let other thread have a turn */
		k_msleep(SLEEPTIME);
		k_sem_give(other_sem);
	}
}

/* define semaphores */

K_SEM_DEFINE(threadA_sem, 1, 1);	/* starts off "available" */
K_SEM_DEFINE(threadB_sem, 0, 1);	/* starts off "not available" */


/* threadB is a dynamic thread that is spawned by threadA */

void threadB(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	/* invoke routine to ping-pong hello messages with threadA */
	helloLoop(__func__, &threadB_sem, &threadA_sem);
}

K_THREAD_STACK_DEFINE(threadB_stack_area, STACKSIZE);
static struct k_thread threadB_data;

/* threadA is a static thread that is spawned automatically */

void threadA(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	struct usbd_context *sample_usbd = sample_usbd_init_device(NULL);

	if (sample_usbd == NULL) {
		printk("Failed to initialize USB device");
		return;
	}

	if (usbd_enable(sample_usbd)) {
		printk("usb backend enable failed");
		return;
	}
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

	/* exercise a spread of kernel primitives once, for the trace */
	run_tracing_workload();

	/* spawn threadB */
	k_tid_t tid = k_thread_create(&threadB_data, threadB_stack_area,
			STACKSIZE, threadB, NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(tid, "thread_b");

	/* invoke routine to ping-pong hello messages with threadB */
	helloLoop(__func__, &threadA_sem, &threadB_sem);
}

K_THREAD_DEFINE(thread_a, STACKSIZE, threadA, NULL, NULL, NULL,
		PRIORITY, 0, 0);

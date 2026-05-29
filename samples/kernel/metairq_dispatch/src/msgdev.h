/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MSGDEV_H__
#define __MSGDEV_H__

/* Define this to enable logging of every event as it is processed to
 * carefully inspect behavior.  Note that at high event rates (see
 * MAX_EVENT_DELAY_TICKS) the log backend won't be able to keep up and
 * that the log processing will add CPU load that is not accounted for
 * in AVERAGE_LOAD_TARGET_PCT.  Disable for more accurate statistics
 * measurement.  Note that if you don't want the log subsystem
 * throttling to drop messages, you'll probably want to increase
 * MAX_EVENT_DELAY_TICKS too, to slow down the reporting.
 */
#define LOG_EVERY_EVENT 1

/* Number of worker threads, each at a separate priority, split evenly
 * between cooperative and preemptible priorities.
 */
#define NUM_THREADS 4

/* The proc_cyc duty cycle parameters are chosen to use approximately
 * this fraction of one CPU's available cycles.  Default 60%
 */
#define AVERAGE_LOAD_TARGET_PCT 60

/* "Hardware interrupts" for our fake device will arrive after a
 * random delay of between zero and this number of ticks.  The event
 * rate should be high enough to collect enough data but low enough
 * that it is not regular.
 */
#define MAX_EVENT_DELAY_TICKS 8

/* How many events will be sent before the test completes?  Note that
 * we keep a naive array of latencies to compute variance, so this
 * value has memory costs.
 */
#define MAX_EVENTS 40

/* The "messages" dispatched by our MetaIRQ thread */
struct msg {
	/* Sequence number */
	uint32_t seq;

	/* Cycle time when the message was "received" */
	uint32_t timestamp;

	/* Thread to which the message is targeted */
	uint32_t target;

	/* Cycles of processing the message requires */
	uint32_t proc_cyc;

	/* Cycles of latency before the MetaIRQ thread received the message */
	uint32_t metairq_latency;
};

/* Initialize the fake "message" device, after this messages will
 * begin arriving via message_dev_fetch().
 */
void message_dev_init(void);

/* Retrieve the next message from the "device", blocks until
 * delivery
 */
void message_dev_fetch(struct msg *m);

#endif /* __MSGDEV_H__ */

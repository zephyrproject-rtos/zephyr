/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/timeout_q.h>
#include "msgdev.h"

/* This file implements a fake device that creates and enqueues
 * "struct msg" messages for handling by the rest of the test.  It's
 * based on Zephyr kernel timeouts only.
 */

/* Note: we use the internal timeout API to get tick precision,
 * k_timer limits us to milliseconds.
 */
static struct _timeout timeout;

/* The "proc_cyc" parameter in the message, indicating how many cycles
 * the target thread should delay while "processing" the message, will
 * be a random number between zero and this value.
 */
uint32_t max_duty_cyc;

uint32_t msg_seq;

K_MSGQ_DEFINE(hw_msgs, sizeof(struct msg), 2, sizeof(uint32_t));

static void timeout_reset(void);

/* Use a custom RNG for good statistics, sys_rand32_get() is just a
 * timer counter on some platforms.  Note that this is used only
 * inside the ISR and needs no locking for the otherwise non-atomic
 * state.
 */
static uint32_t rand32(void)
{
	static uint64_t state;

	if (!state) {
		state = ((uint64_t)k_cycle_get_32()) << 16;
	}

	/* MMIX LCRNG parameters */
	state = state * 6364136223846793005ULL + 1442695040888963407ULL;
	return (uint32_t)(state >> 32);
}

/* This acts as the "ISR" for our fake device.  It "reads from the
 * hardware" a single timestamped message which needs to be dispatched
 * (by the MetaIRQ) to a random thread, with a random argument
 * indicating how long the thread should "process" the message.
 */
static void dev_timer_expired(struct _timeout *t)
{
	__ASSERT_NO_MSG(t == &timeout);
	uint32_t timestamp = k_cycle_get_32();
	struct msg m;

	m.seq = msg_seq++;
	m.timestamp = timestamp;
	m.target = rand32() % NUM_THREADS;
	m.proc_cyc = rand32() % max_duty_cyc;

	int ret = k_msgq_put(&hw_msgs, &m, K_NO_WAIT);

	if (ret != 0) {
		printk("ERROR: Queue full, event dropped!\n");
	}

	if (m.seq < MAX_EVENTS) {
		timeout_reset();
	}
}

static void timeout_reset(void)
{
	uint32_t ticks = rand32() % MAX_EVENT_DELAY_TICKS;

	z_add_timeout(&timeout, dev_timer_expired, Z_TIMEOUT_TICKS(ticks));
}

void message_dev_init(void)
{
	/* Compute a bound for the proc_cyc message parameter such
	 * that on average we request a known percent of available
	 * CPU.  We want the load to sometimes back up and require
	 * queueing, but to be achievable over time.
	 */
	uint64_t cyc_per_tick = k_ticks_to_cyc_near64(1);
	uint64_t avg_ticks_per_event = MAX_EVENT_DELAY_TICKS / 2;
	uint64_t avg_cyc_per_event = cyc_per_tick * avg_ticks_per_event;

	max_duty_cyc = (2 * avg_cyc_per_event * AVERAGE_LOAD_TARGET_PCT) / 100;

	z_add_timeout(&timeout, dev_timer_expired, K_NO_WAIT);
}

void message_dev_fetch(struct msg *m)
{
	int ret = k_msgq_get(&hw_msgs, m, K_FOREVER);

	__ASSERT_NO_MSG(ret == 0);
}

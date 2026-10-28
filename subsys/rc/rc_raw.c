/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/rc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/spsc_lockfree.h>

#include "rc_internal.h"

#define RC_FLAG_OVERFLOW 0

BUILD_ASSERT(CONFIG_RC_INIT_PRIORITY > CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	     "rc devices must init after the rc receive work queue");

static K_THREAD_STACK_DEFINE(rc_rx_stack, CONFIG_RC_RX_WORKQ_STACK_SIZE);
static struct k_work_q rc_rx_wq;

static int rc_raw_push(struct rc_raw_ring *ring, uint32_t word)
{
	uint32_t *slot = spsc_acquire(ring);

	if (slot == NULL) {
		return -ENOMEM;
	}

	*slot = word;
	spsc_produce(ring);

	return 0;
}

int rc_raw_store(const struct device *dev, uint32_t word)
{
	struct rc_dev_data *data = rc_data(dev);
	int ret = 0;

	if (atomic_test_bit(&data->flags, RC_FLAG_OVERFLOW)) {
		ret = rc_raw_push(data->ring, RC_RAW_OVERFLOW);
		if (ret == 0) {
			atomic_clear_bit(&data->flags, RC_FLAG_OVERFLOW);
		}
	}

	if (ret == 0) {
		ret = rc_raw_push(data->ring, word);
		if (ret != 0) {
			atomic_set_bit(&data->flags, RC_FLAG_OVERFLOW);
		}
	}

	(void)k_work_submit_to_queue(&rc_rx_wq, &data->rx_work);

	return ret;
}

static uint64_t rc_cycles(void)
{
	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return k_cycle_get_64();
	}

	return k_cycle_get_32();
}

int rc_raw_store_edge(const struct device *dev, bool pulse_start)
{
	struct rc_dev_data *data = rc_data(dev);
	uint64_t now = rc_cycles();
	uint64_t delta;
	uint32_t us;

	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		delta = now - data->last_edge;
	} else {
		delta = (uint32_t)now - (uint32_t)data->last_edge;
	}
	data->last_edge = now;
	us = (uint32_t)MIN(k_cyc_to_us_floor64(delta), RC_RAW_DURATION_MASK);

	return rc_raw_store(dev, pulse_start ? RC_RAW_SPACE_US(us) : RC_RAW_PULSE_US(us));
}

void rc_raw_overflow(const struct device *dev)
{
	struct rc_dev_data *data = rc_data(dev);

	atomic_set_bit(&data->flags, RC_FLAG_OVERFLOW);
}

static void rc_rx_work_handler(struct k_work *work)
{
	struct rc_dev_data *data = CONTAINER_OF(work, struct rc_dev_data, rx_work);
	uint32_t *slot;

	while ((slot = spsc_consume(data->ring)) != NULL) {
		uint32_t word = *slot;

		spsc_release(data->ring);

		if ((word & RC_RAW_OVERFLOW) != 0U) {
			rc_protocol_reset_all(data->dev);
		} else {
			rc_protocol_decode(data->dev, word);
		}
	}
}

void rc_raw_init(const struct device *dev)
{
	struct rc_dev_data *data = rc_data(dev);

	data->last_edge = rc_cycles();
	k_work_init(&data->rx_work, rc_rx_work_handler);
}

static int rc_rx_wq_init(void)
{
	const struct k_work_queue_config cfg = {.name = "rc_rx"};

	k_work_queue_init(&rc_rx_wq);
	k_work_queue_start(&rc_rx_wq, rc_rx_stack, K_THREAD_STACK_SIZEOF(rc_rx_stack),
			   K_PRIO_PREEMPT(CONFIG_RC_RX_WORKQ_PRIORITY), &cfg);

	return 0;
}

SYS_INIT(rc_rx_wq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

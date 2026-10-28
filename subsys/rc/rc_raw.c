/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/rc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/spsc_lockfree.h>

#include "rc_internal.h"

BUILD_ASSERT(CONFIG_RC_INIT_PRIORITY > CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	     "rc devices must init after the rc receive work queue");

static K_THREAD_STACK_DEFINE(rc_rx_stack, CONFIG_RC_RX_WORKQ_STACK_SIZE);
static struct k_work_q rc_rx_wq;

static uint64_t rc_cycles(void)
{
	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return k_cycle_get_64();
	}

	return k_cycle_get_32();
}

static uint64_t rc_cycles_since(uint64_t then)
{
	uint64_t now = rc_cycles();

	if (IS_ENABLED(CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER)) {
		return now - then;
	}

	return (uint32_t)now - (uint32_t)then;
}

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

/* Serializes the driver and the timeout work: one ring producer at a time */
static int rc_raw_store_locked(struct rc_dev_data *data, uint32_t word)
{
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

	if ((word & RC_RAW_TIMEOUT) != 0U) {
		atomic_set_bit(&data->flags, RC_FLAG_IDLE);
		(void)k_work_cancel_delayable(&data->timeout_work);
	} else if (atomic_test_bit(&data->flags, RC_FLAG_CORE_TIMEOUT)) {
		/* No-op if already scheduled: expiry re-arms for the remainder */
		data->last_store = rc_cycles();
		(void)k_work_schedule_for_queue(&rc_rx_wq, &data->timeout_work,
						K_USEC(atomic_get(&data->rx_timeout_us)));
	}

	(void)k_work_submit_to_queue(&rc_rx_wq, &data->rx_work);

	return ret;
}

int rc_raw_store(const struct device *dev, uint32_t word)
{
	struct rc_dev_data *data = rc_data(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = rc_raw_store_locked(data, word);

	k_spin_unlock(&data->lock, key);

	return ret;
}

int rc_raw_store_edge(const struct device *dev, bool pulse_start)
{
	struct rc_dev_data *data = rc_data(dev);
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint64_t delta = rc_cycles_since(data->last_edge);
	uint32_t us;
	int ret = 0;

	data->last_edge += delta;

	if (!atomic_test_and_clear_bit(&data->flags, RC_FLAG_IDLE)) {
		us = (uint32_t)MIN(k_cyc_to_us_floor64(delta), RC_RAW_DURATION_MASK);
		ret = rc_raw_store_locked(data,
					  pulse_start ? RC_RAW_SPACE_US(us) : RC_RAW_PULSE_US(us));
	}

	k_spin_unlock(&data->lock, key);

	return ret;
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

static void rc_timeout_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct rc_dev_data *data = CONTAINER_OF(dwork, struct rc_dev_data, timeout_work);
	uint32_t timeout = (uint32_t)atomic_get(&data->rx_timeout_us);
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t idle = (uint32_t)MIN(k_cyc_to_us_floor64(rc_cycles_since(data->last_store)),
				      RC_RAW_DURATION_MASK);

	if (idle >= timeout) {
		(void)rc_raw_store_locked(data, RC_RAW_TIMEOUT | RC_RAW_SPACE_US(idle));
	} else {
		(void)k_work_reschedule_for_queue(&rc_rx_wq, &data->timeout_work,
						  K_USEC(timeout - idle));
	}

	k_spin_unlock(&data->lock, key);
}

void rc_raw_init(const struct device *dev, bool core_timeout)
{
	struct rc_dev_data *data = rc_data(dev);

	data->last_edge = rc_cycles();
	atomic_set_bit(&data->flags, RC_FLAG_IDLE);
	atomic_set_bit_to(&data->flags, RC_FLAG_CORE_TIMEOUT, core_timeout);
	k_work_init(&data->rx_work, rc_rx_work_handler);
	k_work_init_delayable(&data->timeout_work, rc_timeout_work_handler);
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

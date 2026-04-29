/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/workq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(worq_sample, LOG_LEVEL_DBG);

#define PRIORITY 0

WORKQ_DEFINE(my_workq);
WORKQ_THREAD_DEFINE(thread1, my_workq, 1024, PRIORITY);
WORKQ_THREAD_DEFINE(thread2, my_workq, 1024, PRIORITY);

struct container {
	struct work_item item;
	bool delayable;
	size_t number;
};

struct sync {
	struct work_item item;
	struct k_sem sem;
};

static void work_fn(struct work_item *item)
{
	struct container *c = CONTAINER_OF(item, struct container, item);

	if (c->delayable) {
		LOG_WRN("[%p] Delayed work(%p) executing:%d", k_current_get(), c, c->number);
	} else {
		LOG_INF("[%p] Work(%p) executing:%d", k_current_get(), c, c->number);
	}
	k_msleep(100);
	if (c->delayable) {
		LOG_WRN("[%p] Delayed work(%p) executed:%d", k_current_get(), c, c->number);
	} else {
		LOG_INF("[%p] Work(%p) executed :%d", k_current_get(), c, c->number);
	}
	k_free(c);
}


void sample_multiple_workers(void)
{

	struct container *c;
	uint32_t delay;

	for (size_t i = 0; i < 5; i++) {
		c = k_malloc(sizeof(struct container));
		__ASSERT(c != NULL, "Memory allocation failed");
		work_init(&c->item, work_fn);
		c->delayable = true;
		delay = 5 + i * 5;
		c->number = i;
		LOG_DBG("[%p] Submitting delayed(%d ms) work(%p) item:%d",
				k_current_get(), delay, c, c->number);
		workq_delayed_submit(&my_workq, &c->item, K_MSEC(delay));
	}

	for (size_t i = 0; i < 5; i++) {
		c = k_malloc(sizeof(struct container));
		__ASSERT(c != NULL, "Memory allocation failed");
		work_init(&c->item, work_fn);
		c->delayable = false;
		c->number = i+5;
		LOG_DBG("[%p] Submitting work(%p) item:%d", k_current_get(), c, c->number);
		workq_submit(&my_workq, &c->item);
		k_msleep(10);
	}

	workq_drain(&my_workq, K_FOREVER);
	LOG_INF("All work items have been processed");
}

static void sync_fn(struct work_item *item)
{
	struct sync *s = CONTAINER_OF(item, struct sync, item);

	LOG_DBG("[%p] Work(%p) executing with synchronization", k_current_get(), s);
	k_msleep(100);
	LOG_DBG("[%p] Work(%p) executed with synchronization", k_current_get(), s);
	k_sem_give(&s->sem);
}

void sample_synchronization(void)
{
	struct sync *s;

	s = k_malloc(sizeof(*s));
	__ASSERT(s != NULL, "Memory allocation failed");
	work_init(&s->item, sync_fn);
	k_sem_init(&s->sem, 0, 1);
	LOG_DBG("[%p] Submitting work(%p) item with synchronization", k_current_get(), s);
	workq_submit(&my_workq, &s->item);
	k_sem_take(&s->sem, K_FOREVER);
	LOG_INF("Work item with synchronization has completed");
	/* Note that the work item is freed here and not in the work function, as the work function
	 * is responsible for giving the semaphore to signal completion. This ensures that the work
	 * item is not freed before the semaphore goes out of scope, which could lead to undefined
	 * behavior.
	 */
	k_free(s);

	workq_drain(&my_workq, K_FOREVER);
	LOG_INF("All work items have been processed");
}

int main(void)
{
	LOG_INF("Hello from Zephyr Work Queue example!");
	sample_multiple_workers();
	sample_synchronization();
	return 0;
}

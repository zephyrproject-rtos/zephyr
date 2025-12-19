/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <CANopen.h>

static struct k_timer sync_timer;
static struct k_sem sync_sem;
static uint32_t last_sync_time; /* cycles */

/**
 * @brief SYNC Timer expiry callback.
 */
static void sync_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&sync_sem);
}

/**
 * @brief CANopen sync thread.
 *
 * The CANopen real-time sync thread processes SYNC RPDOs and TPDOs
 * through the CANopenNode stack with an interval of 1 millisecond.
 * Uses a k_timer for deterministic timing.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
static void canopen_sync_thread(void *p1, void *p2, void *p3)
{
	uint32_t current_time; /* cycles */
	uint32_t delta;        /* cycles */
	uint32_t elapsed = 0;  /* microseconds */
	bool sync;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Initialize binary semaphore and timer */
	k_sem_init(&sync_sem, 0, 1);
	k_timer_init(&sync_timer, sync_timer_expiry, NULL);

	/* Start the periodic timer */
	k_timer_start(&sync_timer, K_USEC(CONFIG_CANOPENNODE_SYNC_THREAD_PERIOD_US),
		      K_USEC(CONFIG_CANOPENNODE_SYNC_THREAD_PERIOD_US));
	last_sync_time = k_cycle_get_32();

	while (true) {
		/* Wait for timer to fire - deterministic wake-up */
		k_sem_take(&sync_sem, K_FOREVER);

		current_time = k_cycle_get_32();
		delta = current_time - last_sync_time;
		elapsed = (uint32_t)k_cyc_to_ns_floor64(delta) / NSEC_PER_USEC;
		last_sync_time = current_time;

		if (CO && CO->CANmodule[0] && CO->CANmodule[0]->CANnormal) {
			CO_LOCK_OD();
			sync = CO_process_SYNC(CO, elapsed);
			CO_process_RPDO(CO, sync);
			CO_process_TPDO(CO, sync, elapsed);
			CO_UNLOCK_OD();
		}
	}
}

K_THREAD_DEFINE(canopen_sync, CONFIG_CANOPENNODE_SYNC_THREAD_STACK_SIZE, canopen_sync_thread, NULL,
		NULL, NULL, CONFIG_CANOPENNODE_SYNC_THREAD_PRIORITY, 0, 1);

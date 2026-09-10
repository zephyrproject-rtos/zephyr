/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>

#include <platform/argus_irq.h>

/* We have an atomic lock count in order to only lock once and store the lock key. */
static atomic_val_t lock_count;
static uint32_t lock;

void IRQ_UNLOCK(void)
{
	if (atomic_dec(&lock_count) == 1) {
		irq_unlock(lock);
	}
}

void IRQ_LOCK(void)
{
	if (atomic_inc(&lock_count) == 0) {
		lock = irq_lock();
	}
}

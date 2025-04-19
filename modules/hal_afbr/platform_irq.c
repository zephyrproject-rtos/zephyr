/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>

#include <platform/argus_irq.h>

static uint32_t lock;

void IRQ_UNLOCK(void)
{
	irq_unlock(lock);
}

void IRQ_LOCK(void)
{
	lock = irq_lock();
}

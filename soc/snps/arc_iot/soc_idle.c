/*
 * Copyright (c) 2026 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom CPU idle implementation for ARC IoT SoC
 *
 * The ARC IoT SoC has a hardware limitation where entering sleep mode
 * causes peripherals (e.g., UART) to lose power. This prevents wake-up
 * from peripheral interrupts. These custom implementations skip the sleep
 * instruction and only enable interrupts.
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

void arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(0);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
}

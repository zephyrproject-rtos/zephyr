/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-R interrupt management
 *
 *
 * Interrupt management: enabling/disabling and dynamic ISR
 * connecting/replacing.  SW_ISR_TABLE_DYNAMIC has to be enabled for
 * connecting ISRs at runtime.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <irq_nextlevel.h>
#include <sys/__assert.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <irq.h>
#include <kernel_structs.h>
#include <debug/tracing.h>

void z_arch_irq_enable(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_enable_next_level(dev, (irq >> 8) - 1);
}

void z_arch_irq_disable(unsigned int irq)
{
	struct device *dev = _sw_isr_table[0].arg;

	irq_disable_next_level(dev, (irq >> 8) - 1);
}

int z_arch_irq_is_enabled(unsigned int irq)
{
    struct device *dev = _sw_isr_table[0].arg;

	return irq_is_enabled_next_level(dev);
}

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number
 * of priority levels is a little complex, as there are some hardware
 * priority levels which are reserved.
 *
 * @return N/A
 */
void z_arm_irq_priority_set(unsigned int irq, unsigned int prio, u32_t flags)
{
	struct device *dev = _sw_isr_table[0].arg;

	if (irq == 0)
		return 0;

	irq_set_priority_next_level(dev, (irq >> 8) - 1, prio, flags);
}
